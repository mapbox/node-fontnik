#include "glyphs.hpp"
#include "font_face_set.hpp"
#include "harfbuzz_shaper.hpp"

// node
#include <node_buffer.h>

// stl
#include <algorithm>
#include <memory>
#include <sstream>
#include <iostream>

// freetype2
extern "C"
{
#include <ft2build.h>
#include FT_FREETYPE_H
// #include FT_STROKER_H
}

struct RangeBaton {
    v8::Persistent<v8::Function> callback;
    Glyphs *glyphs;
    std::string fontstack;
    bool error;
    std::string error_name;
    unsigned long start;
    unsigned long end;
};

v8::Persistent<v8::FunctionTemplate> Glyphs::constructor;

Glyphs::Glyphs() : node::ObjectWrap() {}

Glyphs::Glyphs(const char *data, size_t length) : node::ObjectWrap() {
    glyphs.ParseFromArray(data, length);
}

Glyphs::~Glyphs() {}

void Glyphs::Init(v8::Handle<v8::Object> target) {
    v8::HandleScope scope;

    v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(New);
    v8::Local<v8::String> name = v8::String::NewSymbol("Glyphs");

    constructor = v8::Persistent<v8::FunctionTemplate>::New(tpl);

    // node::ObjectWrap uses the first internal field to store the wrapped pointer.
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(name);

    // Add all prototype methods, getters and setters here.
    NODE_SET_PROTOTYPE_METHOD(constructor, "serialize", Serialize);
    NODE_SET_PROTOTYPE_METHOD(constructor, "range", Range);

    // This has to be last, otherwise the properties won't show up on the
    // object in JavaScript.
    target->Set(name, constructor->GetFunction());
}

v8::Handle<v8::Value> Glyphs::New(const v8::Arguments& args) {
    if (!args.IsConstructCall()) {
        return ThrowException(v8::Exception::TypeError(v8::String::New("Constructor must be called with new keyword")));
    }
    if (args.Length() > 0 && !node::Buffer::HasInstance(args[0])) {
        return ThrowException(v8::Exception::TypeError(v8::String::New("First argument may only be a buffer")));
    }

    Glyphs* glyphs;

    if (args.Length() < 1) {
        glyphs = new Glyphs();
    } else {
        v8::Local<v8::Object> buffer = args[0]->ToObject();
        glyphs = new Glyphs(node::Buffer::Data(buffer), node::Buffer::Length(buffer));
    }
    
    glyphs->Wrap(args.This());

    return args.This();
}

bool Glyphs::HasInstance(v8::Handle<v8::Value> val) {
    if (!val->IsObject()) return false;
    return constructor->HasInstance(val->ToObject());
}

v8::Handle<v8::Value> Glyphs::Serialize(const v8::Arguments& args) {
    v8::HandleScope scope;
    llmr::glyphs::glyphs& glyphs = node::ObjectWrap::Unwrap<Glyphs>(args.This())->glyphs;
    std::string serialized = glyphs.SerializeAsString();
    return scope.Close(node::Buffer::New(serialized.data(), serialized.length())->handle_);
}

v8::Handle<v8::Value> Glyphs::Range(const v8::Arguments& args) {
    v8::HandleScope scope;

    // Validate arguments.
    if (args.Length() < 1 || !args[0]->IsString()) {
        return ThrowException(v8::Exception::TypeError(
            v8::String::New("fontstack must be a string")));
    }

    if (args.Length() < 2 || !args[1]->IsNumber()) {
        return ThrowException(v8::Exception::TypeError(
            v8::String::New("start must be a number")));
    }

    if (args.Length() < 3 || !args[2]->IsNumber()) {
        return ThrowException(v8::Exception::TypeError(
            v8::String::New("end must be a number")));
    }

    if (args.Length() < 4 || !args[3]->IsFunction()) {
        return ThrowException(v8::Exception::TypeError(
            v8::String::New("callback must be a function")));
    }

    v8::Local<v8::Function> callback = v8::Local<v8::Function>::Cast(args[3]);

    if (args[1]->NumberValue() < 0 || args[1]->NumberValue() > 65533) {
        v8::Local<v8::Value> argv[1] = { v8::Exception::Error(v8::String::New("start must be a number from 0-65533")) };
        callback->Call(v8::Context::GetCurrent()->Global(), 1, argv);
        return v8::Undefined();
    }

    if (args[2]->NumberValue() < 0 || args[2]->NumberValue() > 65533) {
        v8::Local<v8::Value> argv[1] = { v8::Exception::Error(v8::String::New("end must be a number from 0-65533")) };
        callback->Call(v8::Context::GetCurrent()->Global(), 1, argv);
        return v8::Undefined();
    }

    if (args[1]->NumberValue() > args[2]->NumberValue()) {
        v8::Local<v8::Value> argv[1] = { v8::Exception::Error(v8::String::New("start must be less than or equal to end")) };
        callback->Call(v8::Context::GetCurrent()->Global(), 1, argv);
        return v8::Undefined();
    }

    v8::String::Utf8Value fontstack(args[0]->ToString());
    unsigned long start = args[1]->NumberValue();
    unsigned long end = args[2]->NumberValue();

    Glyphs *glyphs = node::ObjectWrap::Unwrap<Glyphs>(args.This());

    RangeBaton* baton = new RangeBaton();
    baton->callback = v8::Persistent<v8::Function>::New(callback);
    baton->glyphs = glyphs;
    baton->fontstack = *fontstack;
    baton->start = start;
    baton->end = end;

    uv_work_t *req = new uv_work_t();
    req->data = baton;

    int status = uv_queue_work(uv_default_loop(), req, AsyncRange, (uv_after_work_cb)RangeAfter);
    assert(status == 0);

    return v8::Undefined();
}

void Glyphs::AsyncRange(uv_work_t* req) {
    RangeBaton* baton = static_cast<RangeBaton*>(req->data);

    fontserver::freetype_engine font_engine_;
    fontserver::face_manager_freetype font_manager(font_engine_);

    fontserver::font_set fset(baton->fontstack);
    fset.add_fontstack(baton->fontstack, ',');

    fontserver::face_set_ptr face_set;

    try {
        face_set = font_manager.get_face_set(fset);
    } catch(const std::runtime_error &e) {
        baton->error = true;
        baton->error_name = e.what();
        return;
    }

    FT_ULong char_code = baton->start;
    FT_ULong char_end = baton->end + 1;

    llmr::glyphs::glyphs& glyphs = baton->glyphs->glyphs;

    llmr::glyphs::fontstack *mutable_fontstack = glyphs.add_stacks();
    mutable_fontstack->set_name(baton->fontstack);

    std::stringstream range;
    range << baton->start << "-" << baton->end;
    mutable_fontstack->set_range(range.str());

    fontserver::text_format format(baton->fontstack, 24);
    const double scale_factor = 1.0;

    // Set character sizes.
    double size = format.text_size * scale_factor;
    face_set->set_character_sizes(size);

    for (; char_code < char_end; char_code++) {
        fontserver::glyph_info glyph;

        for (auto const& face : *face_set) {
            // Get FreeType face from face_ptr.
            FT_Face ft_face = face->get_face();
            FT_UInt char_index = FT_Get_Char_Index(ft_face, char_code);

            // Try next font in fontset.
            if (!char_index) continue;

            glyph.glyph_index = char_index;
            face->glyph_dimensions(glyph);

            // Add glyph to fontstack.
            llmr::glyphs::glyph *mutable_glyph = mutable_fontstack->add_glyphs();
            mutable_glyph->set_id(char_code);
            mutable_glyph->set_width(glyph.width);
            mutable_glyph->set_height(glyph.height);
            mutable_glyph->set_left(glyph.left);
            mutable_glyph->set_top(glyph.top - glyph.ascender);
            mutable_glyph->set_advance(glyph.advance);

            if (glyph.width > 0) {
                mutable_glyph->set_bitmap(glyph.bitmap);
            }

            // Glyph added, continue to next char_code.
            break;
        }
    }
}

void Glyphs::RangeAfter(uv_work_t* req) {
    v8::HandleScope scope;
    RangeBaton* baton = static_cast<RangeBaton*>(req->data);

    const unsigned argc = 1;

    v8::TryCatch try_catch;

    if (baton->error) {
        v8::Local<v8::Value> argv[argc] = { v8::Exception::Error(v8::String::New(baton->error_name.c_str())) };
        baton->callback->Call(v8::Context::GetCurrent()->Global(), argc, argv);
    } else {
        v8::Local<v8::Value> argv[argc] = { v8::Local<v8::Value>::New(v8::Null()) };
        baton->callback->Call(v8::Context::GetCurrent()->Global(), argc, argv);
    }

    if (try_catch.HasCaught()) {
        node::FatalException(try_catch);
    }

    baton->callback.Dispose();

    delete baton;
    delete req;
}
