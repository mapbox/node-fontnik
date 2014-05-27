#include "tile.hpp"
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
    Tile *tile;
    std::string fontstack;
    unsigned long start;
    unsigned long end;
};

v8::Persistent<v8::FunctionTemplate> Tile::constructor;

Tile::Tile() : node::ObjectWrap() {}

Tile::Tile(const char *data, size_t length) : node::ObjectWrap() {
    glyphs.ParseFromArray(data, length);
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
    NODE_SET_PROTOTYPE_METHOD(constructor, "serialize", Serialize);
    NODE_SET_PROTOTYPE_METHOD(constructor, "range", Range);

    // This has to be last, otherwise the properties won't show up on the
    // object in JavaScript.
    target->Set(name, constructor->GetFunction());
}

v8::Handle<v8::Value> Tile::New(const v8::Arguments& args) {
    if (!args.IsConstructCall()) {
        return ThrowException(v8::Exception::TypeError(v8::String::New("Constructor must be called with new keyword")));
    }
    if (args.Length() > 0 && !node::Buffer::HasInstance(args[0])) {
        return ThrowException(v8::Exception::TypeError(v8::String::New("First argument may only be a buffer")));
    }

    Tile* tile;

    if (args.Length() < 1) {
        tile = new Tile();
    } else {
        v8::Local<v8::Object> buffer = args[0]->ToObject();
        tile = new Tile(node::Buffer::Data(buffer), node::Buffer::Length(buffer));
    }
    
    tile->Wrap(args.This());

    return args.This();
}

bool Tile::HasInstance(v8::Handle<v8::Value> val) {
    if (!val->IsObject()) return false;
    return constructor->HasInstance(val->ToObject());
}

v8::Handle<v8::Value> Tile::Serialize(const v8::Arguments& args) {
    v8::HandleScope scope;
    llmr::glyphs::glyphs& tile = node::ObjectWrap::Unwrap<Tile>(args.This())->glyphs;
    std::string serialized = tile.SerializeAsString();
    return scope.Close(node::Buffer::New(serialized.data(), serialized.length())->handle_);
}

v8::Handle<v8::Value> Tile::Range(const v8::Arguments& args) {
    v8::HandleScope scope;

    // Validate arguments.
    if (args.Length() < 1 || !args[0]->IsString()) {
        return ThrowException(v8::Exception::TypeError(
            v8::String::New("First argument must be a font stack")));
    }

    if (args.Length() < 2 || !args[1]->IsNumber()) {
        return ThrowException(v8::Exception::TypeError(
            v8::String::New("Second argument must be a number")));
    }

    if (args.Length() < 3 || !args[2]->IsNumber()) {
        return ThrowException(v8::Exception::TypeError(
            v8::String::New("Third argument must be a number")));
    }

    if (args.Length() < 4 || !args[3]->IsFunction()) {
        return ThrowException(v8::Exception::TypeError(
            v8::String::New("Fourth argument must be a callback function")));
    }

    v8::Local<v8::Function> callback = v8::Local<v8::Function>::Cast(args[3]);
    v8::String::Utf8Value fontstack(args[0]->ToString());
    unsigned long start = args[1]->NumberValue();
    unsigned long end = args[2]->NumberValue();

    Tile *tile = node::ObjectWrap::Unwrap<Tile>(args.This());

    RangeBaton* baton = new RangeBaton();
    baton->callback = v8::Persistent<v8::Function>::New(callback);
    baton->tile = tile;
    baton->fontstack = *fontstack;
    baton->start = start;
    baton->end = end;

    uv_work_t *req = new uv_work_t();
    req->data = baton;

    int status = uv_queue_work(uv_default_loop(), req, AsyncRange, (uv_after_work_cb)RangeAfter);
    assert(status == 0);

    return v8::Undefined();
}

void Tile::AsyncRange(uv_work_t* req) {
    RangeBaton* baton = static_cast<RangeBaton*>(req->data);

    fontserver::freetype_engine font_engine_;
    fontserver::face_manager_freetype font_manager(font_engine_);

    fontserver::font_set fset(baton->fontstack);
    fset.add_fontstack(baton->fontstack, ',');

    fontserver::face_set_ptr face_set = font_manager.get_face_set(fset);
    if (!face_set->size()) return;

    FT_ULong char_code = baton->start;
    FT_ULong char_end = baton->end + 1;

    llmr::glyphs::glyphs& tile = baton->tile->glyphs;

    llmr::glyphs::fontstack *mutable_fontstack = tile.add_stacks();
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

void Tile::RangeAfter(uv_work_t* req) {
    v8::HandleScope scope;
    RangeBaton* baton = static_cast<RangeBaton*>(req->data);

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
