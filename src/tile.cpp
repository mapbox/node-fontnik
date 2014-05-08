#include "tile.hpp"
#include "font_face_set.hpp"
#include "harfbuzz_shaper.hpp"
#include "tile_face.hpp"

// node
#include <node_buffer.h>

// stl
#include <algorithm>
#include <memory>
#include <iostream>

// freetype2
extern "C"
{
#include <ft2build.h>
#include FT_FREETYPE_H
// #include FT_STROKER_H
}

struct ShapeBaton {
    v8::Persistent<v8::Function> callback;
    Tile *tile;
    std::string fontstack;
};

v8::Persistent<v8::FunctionTemplate> Tile::constructor;

Tile::Tile(const char *data, size_t length) : node::ObjectWrap() {
    tile.ParseFromArray(data, length);
}

Tile::~Tile() {}

void Tile::Init(v8::Handle<v8::Object> target) {
    v8::HandleScope scope;

    v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(New);
    v8::Local<v8::String> name = v8::String::NewSymbol("Tile");

    constructor = v8::Persistent<v8::FunctionTemplate>::New(tpl);

    // node::ObjectWrap uses the first internal field to store the wrapped pointer.
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(name);

    // Add all prototype methods, getters and setters here.
    constructor->PrototypeTemplate()->SetAccessor(v8::String::NewSymbol("length"), Length);
    NODE_SET_PROTOTYPE_METHOD(constructor, "serialize", Serialize);
    NODE_SET_PROTOTYPE_METHOD(constructor, "shape", Shape);

    // This has to be last, otherwise the properties won't show up on the
    // object in JavaScript.
    target->Set(name, constructor->GetFunction());
}

v8::Handle<v8::Value> Tile::New(const v8::Arguments& args) {
    if (!args.IsConstructCall()) {
        return ThrowException(v8::Exception::TypeError(v8::String::New("Constructor must be called with new keyword")));
    }
    if (args.Length() < 1 || !node::Buffer::HasInstance(args[0])) {
        return ThrowException(v8::Exception::TypeError(v8::String::New("First argument must be a buffer")));
    }

    v8::Local<v8::Object> buffer = args[0]->ToObject();

    Tile* tile = new Tile(node::Buffer::Data(buffer), node::Buffer::Length(buffer));
    tile->Wrap(args.This());

    return args.This();
}

bool Tile::HasInstance(v8::Handle<v8::Value> val) {
    if (!val->IsObject()) return false;
    return constructor->HasInstance(val->ToObject());
}

v8::Handle<v8::Value> Tile::Length(v8::Local<v8::String> property, const v8::AccessorInfo &info) {
    v8::HandleScope scope;
    Tile* tile = node::ObjectWrap::Unwrap<Tile>(info.This());
    v8::Local<v8::Number> length = v8::Uint32::New(tile->tile.layers_size());
    return scope.Close(length);
}

v8::Handle<v8::Value> Tile::Serialize(const v8::Arguments& args) {
    v8::HandleScope scope;
    llmr::vector::tile& tile = node::ObjectWrap::Unwrap<Tile>(args.This())->tile;
    std::string serialized = tile.SerializeAsString();
    return scope.Close(node::Buffer::New(serialized.data(), serialized.length())->handle_);
}

v8::Handle<v8::Value> Tile::Shape(const v8::Arguments& args) {
    v8::HandleScope scope;

    if (args.Length() < 1) {
        return ThrowException(v8::Exception::TypeError(
            v8::String::New("First argument must be a font stack")));
    }

    if (args.Length() < 2 || !args[1]->IsFunction()) {
        return ThrowException(v8::Exception::TypeError(
            v8::String::New("Second argument must be a callback function")));
    }
    v8::Local<v8::Function> callback = v8::Local<v8::Function>::Cast(args[1]);
    // TODO: validate this is a string
    v8::String::Utf8Value fontstack(args[0]->ToString());

    Tile *tile = node::ObjectWrap::Unwrap<Tile>(args.This());

    ShapeBaton* baton = new ShapeBaton();
    baton->callback = v8::Persistent<v8::Function>::New(callback);
    baton->tile = tile;
    baton->fontstack = *fontstack;

    uv_work_t *req = new uv_work_t();
    req->data = baton;

    int status = uv_queue_work(uv_default_loop(), req, AsyncShape, (uv_after_work_cb)ShapeAfter);
    assert(status == 0);

    return v8::Undefined();
}

void Tile::AsyncShape(uv_work_t* req) {
    ShapeBaton* baton = static_cast<ShapeBaton*>(req->data);

    fontserver::freetype_engine font_engine_;
    fontserver::face_manager_freetype font_manager(font_engine_);

    fontserver::font_set fset(baton->fontstack);
    fset.add_fontstack(baton->fontstack, ',');

    fontserver::face_set_ptr face_set = font_manager.get_face_set(fset);
    if (!face_set->size()) return;

    std::map<fontserver::face_ptr, fontserver::tile_face *> face_map;
    std::vector<fontserver::tile_face *> tile_faces;

    llmr::vector::tile& tile = baton->tile->tile;

    fontserver::text_format format(baton->fontstack, 24);
    fontserver::text_format_ptr format_ptr = 
        std::make_shared<fontserver::text_format>(format);

    const double scale_factor = 1.0;

    FT_UInt glyph_index = 0;
    FT_UInt glyph_end = 128;

    for (auto const& face : *face_set) {
        // Create tile_face, add to face_map and tile_faces.
        fontserver::tile_face *t_face = new fontserver::tile_face(face);
        face_map.emplace(face, t_face);
        tile_faces.push_back(t_face);

        // Get FreeType face from face_ptr.
        FT_Face ft_face = face->get_face();

        // Add all glyphs for this labels and add new font
        // faces as they appear.
        FT_ULong char_code = FT_Get_First_Char(ft_face, &glyph_index);
        while (glyph_index != 0 && char_code < glyph_end) {
            fontserver::glyph_info glyph;
            glyph.glyph_index = char_code;

            face->glyph_dimensions(glyph);

            // Add glyph to t_face.
            t_face->add_glyph(glyph);

            // Advance char_code to next glyph index.
            char_code = FT_Get_Next_Char(ft_face, char_code, &glyph_index);
        }
    }

    // Insert SDF glyphs + bitmaps
    for (auto const& face : tile_faces) {
        llmr::vector::face *mutable_face = tile.add_faces();
        mutable_face->set_family(face->family);
        mutable_face->set_style(face->style);

        for (auto const& glyph : face->glyphs) {
            llmr::vector::glyph *mutable_glyph = mutable_face->add_glyphs();
            mutable_glyph->set_id(glyph.glyph_index);
            mutable_glyph->set_width(glyph.width);
            mutable_glyph->set_height(glyph.height);
            mutable_glyph->set_left(glyph.left);
            mutable_glyph->set_top(glyph.top);
            mutable_glyph->set_advance(glyph.advance);
            if (glyph.width > 0) {
                mutable_glyph->set_bitmap(glyph.bitmap);
            }
        }

        delete face;
    }
}

void Tile::ShapeAfter(uv_work_t* req) {
    v8::HandleScope scope;
    ShapeBaton* baton = static_cast<ShapeBaton*>(req->data);

    const unsigned argc = 1;
    v8::Local<v8::Value> argv[argc] = { v8::Local<v8::Value>::New(v8::Null()) };

    v8::TryCatch try_catch;
    baton->callback->Call(v8::Context::GetCurrent()->Global(), argc, argv);
    if (try_catch.HasCaught()) {
        node::FatalException(try_catch);
    }

    baton->callback.Dispose();

    delete baton;
    delete req;
}
