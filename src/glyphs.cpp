// fontnik
#include <node_fontnik/glyphs.hpp>

// node
#include <node_buffer.h>
#include <nan.h>

// freetype2
extern "C"
{
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H
}

namespace node_fontnik
{

struct RangeBaton {
    v8::Persistent<v8::Function> callback;
    Glyphs *glyphs;
    std::string fontstack;
    std::string range;
    std::vector<std::uint32_t> chars;
    bool error;
    std::string error_name;
};

v8::Persistent<v8::FunctionTemplate> Glyphs::constructor;

Glyphs::Glyphs() : node::ObjectWrap() {
    glyphs = fontnik::Glyphs();
}

Glyphs::Glyphs(const char *data, size_t length) : node::ObjectWrap() {
    glyphs = fontnik::Glyphs(data, length);
}

Glyphs::~Glyphs() {}

void Glyphs::Init(v8::Handle<v8::Object> target) {
    NanScope();

    v8::Local<v8::FunctionTemplate> tpl = NanNew<v8::FunctionTemplate>(New);
    v8::Local<v8::String> name = NanNew<v8::String>("Glyphs");

    NanAssignPersistent(Glyphs::constructor, tpl);

    // node::ObjectWrap uses the first internal field to store the wrapped pointer.
    tpl->InstanceTemplate()->SetInternalFieldCount(1);
    tpl->SetClassName(name);

    // Add all prototype methods, getters and setters here.
    NODE_SET_PROTOTYPE_METHOD(tpl, "serialize", Serialize);
    NODE_SET_PROTOTYPE_METHOD(tpl, "range", Range);
    NODE_SET_PROTOTYPE_METHOD(tpl, "codepoints", Codepoints);

    // This has to be last, otherwise the properties won't show up on the
    // object in JavaScript.
    target->Set(name, constructor->GetFunction());
}

NAN_METHOD(Glyphs::New) {
    NanScope();

    if (!args.IsConstructCall()) {
        return NanThrowTypeError("Constructor must be called with new keyword");
    }
    if (args.Length() > 0 && !node::Buffer::HasInstance(args[0])) {
        return NanThrowTypeError("First argument may only be a buffer");
    }

    Glyphs* glyphs;

    if (args.Length() < 1) {
        glyphs = new Glyphs();
    } else {
        v8::Local<v8::Object> buffer = args[0]->ToObject();
        glyphs = new Glyphs(node::Buffer::Data(buffer), node::Buffer::Length(buffer));
    }

    glyphs->Wrap(args.This());

    NanReturnValue(args.This());
}

bool Glyphs::HasInstance(v8::Handle<v8::Value> val) {
    if (!val->IsObject()) return false;
    return constructor->HasInstance(val->ToObject());
}

NAN_METHOD(Glyphs::Serialize) {
    NanScope();
    std::string serialized = node::ObjectWrap::Unwrap<Glyphs>(args.This())->glyphs.Serialize();
    NanReturnValue(NanNewBufferHandle(serialized.data(), serialized.length()));
}

NAN_METHOD(Glyphs::Range) {
    NanScope();

    // Validate arguments.
    if (args.Length() < 1 || !args[0]->IsString()) {
        return NanThrowTypeError("fontstack must be a string");
    }

    if (args.Length() < 2 || !args[1]->IsString()) {
        return NanThrowTypeError("range must be a string");
    }

    if (args.Length() < 3 || !args[2]->IsArray()) {
        return NanThrowTypeError("chars must be an array");
    }

    if (args.Length() < 4 || !args[3]->IsFunction()) {
        return NanThrowTypeError("callback must be a function");
    }

    v8::String::Utf8Value fontstack(args[0]->ToString());
    v8::String::Utf8Value range(args[1]->ToString());
    v8::Local<v8::Array> charsArray = v8::Local<v8::Array>::Cast(args[2]);
    v8::Local<v8::Function> callback = v8::Local<v8::Function>::Cast(args[3]);

    unsigned array_size = charsArray->Length();
    std::vector<std::uint32_t> chars;
    for (unsigned i=0; i < array_size; i++) {
        chars.push_back(charsArray->Get(i)->IntegerValue());
    }

    Glyphs *glyphs = node::ObjectWrap::Unwrap<Glyphs>(args.This());

    RangeBaton* baton = new RangeBaton();
    baton->callback = v8::Persistent<v8::Function>::New(callback);
    baton->glyphs = glyphs;
    baton->fontstack = *fontstack;
    baton->range = *range;
    baton->chars = chars;

    uv_work_t *req = new uv_work_t();
    req->data = baton;

    int status = uv_queue_work(uv_default_loop(), req, AsyncRange, (uv_after_work_cb)RangeAfter);
    assert(status == 0);

    NanReturnUndefined();
}

NAN_METHOD(Glyphs::Codepoints) {
    NanScope();

    // Validate arguments.
    if (args.Length() < 1 || !args[0]->IsString()) {
        return NanThrowTypeError("fontstack must be a string");
    }

    v8::String::Utf8Value param1(args[0]->ToString());
    std::string from = std::string(*param1);
    try {
        std::vector<int> points = fontnik::Glyphs::Codepoints(from);

        v8::Handle<v8::Array> result = v8::Array::New(points.size());

        for (size_t i = 0; i < points.size(); i++) {
            result->Set(i, NanNew<v8::Number>(points[i]));
        }
        NanReturnValue(result);
    } catch (std::exception const& ex) {
        return NanThrowTypeError(ex.what());
    }
    NanReturnUndefined();
}

struct FaceMetadata {
    std::string family_name;
    std::string style_name;
    std::vector<int> points;
    FaceMetadata() :
        family_name(),
        style_name(),
        points() {}
};

struct LoadBaton {
    v8::Persistent<v8::Function> callback;
    std::string file_name;
    std::string error_name;
    bool error;
    std::vector<FaceMetadata> faces;
    uv_work_t request;
    LoadBaton() :
        file_name(),
        error_name(),
        faces() {}
};

NAN_METHOD(Load) {
    NanScope();

    // Validate arguments.
    if (!args[0]->IsString()) {
        return NanThrowTypeError("First argument must be a path to a font");
    }
    if (args.Length() < 2 || !args[1]->IsFunction()) {
        return NanThrowTypeError("callback must be a function");
    }

    v8::Local<v8::Function> callback = args[1].As<v8::Function>();

    LoadBaton* baton = new LoadBaton();
    baton->file_name = *NanUtf8String(args[0]);

    baton->request.data = baton;
    NanAssignPersistent(baton->callback, callback.As<v8::Function>());

    uv_queue_work(uv_default_loop(), &baton->request, LoadAsync, (uv_after_work_cb)AfterLoad);
    NanReturnUndefined();
}

void LoadAsync(uv_work_t* req) {
    LoadBaton* baton = static_cast<LoadBaton*>(req->data);

    FT_Library library = nullptr;
    FT_Error error = FT_Init_FreeType(&library);
    if (error) {
        baton->error = true;
        baton->error_name = std::string("could not open FreeType library");
        return;
    }

    std::vector<FaceMetadata> faces;
    FaceMetadata face;
    std::vector<int> points;

    FT_Face ft_face = nullptr;

    FT_New_Face(library, baton->file_name.c_str(), 0, &ft_face);

    for ( int i = 0; ft_face == 0 || i < ft_face->num_faces; ++i )
    {
        FT_ULong charcode;
        FT_UInt gindex;
        charcode = FT_Get_First_Char(ft_face, &gindex);
        while (gindex != 0) {
            charcode = FT_Get_Next_Char(ft_face, charcode, &gindex);
            if (charcode != 0) points.push_back(charcode);
        }
        std::sort(points.begin(), points.end());
        auto last = std::unique(points.begin(), points.end());
        points.erase(last, points.end());

        face.points = std::move(points);
        face.family_name = ft_face->family_name;
        face.style_name = ft_face->style_name;

        faces.push_back(std::move(face));
    }

    baton->faces = std::move(faces);

    FT_Done_Face(ft_face);
};

void AfterLoad(uv_work_t* req) {
    NanScope();
    LoadBaton* baton = static_cast<LoadBaton*>(req->data);

    const unsigned argc = 1;

    v8::TryCatch try_catch;
    v8::Local<v8::Context> ctx = NanGetCurrentContext();

    if (baton->error) {
        v8::Local<v8::Value> argv[argc] = {
            v8::Exception::Error(NanNew<v8::String>(baton->error_name.c_str()))
        };
        baton->callback->Call(ctx->Global(), argc, argv);
    } else {
        v8::Local<v8::Value> argv[argc] = {
            NanNull()
        };
        baton->callback->Call(ctx->Global(), argc, argv);
    }

    if (try_catch.HasCaught()) {
        node::FatalException(try_catch);
    }

    baton->callback.Dispose();

    delete baton;
};

void Glyphs::AsyncRange(uv_work_t* req) {
    RangeBaton* baton = static_cast<RangeBaton*>(req->data);

    try {
        baton->glyphs->glyphs.Range(baton->fontstack, baton->range, baton->chars);
    } catch(const std::runtime_error &e) {
        baton->error = true;
        baton->error_name = e.what();
        return;
    }
}

void Glyphs::RangeAfter(uv_work_t* req) {
    NanScope();
    RangeBaton* baton = static_cast<RangeBaton*>(req->data);

    const unsigned argc = 1;

    v8::TryCatch try_catch;
    v8::Local<v8::Context> ctx = NanGetCurrentContext();

    if (baton->error) {
        v8::Local<v8::Value> argv[argc] = {
            v8::Exception::Error(NanNew<v8::String>(baton->error_name.c_str()))
        };
        baton->callback->Call(ctx->Global(), argc, argv);
    } else {
        v8::Local<v8::Value> argv[argc] = {
            NanNull()
        };
        baton->callback->Call(ctx->Global(), argc, argv);
    }

    if (try_catch.HasCaught()) {
        node::FatalException(try_catch);
    }

    baton->callback.Dispose();

    delete baton;
    delete req;
}

} // ns node_fontnik
