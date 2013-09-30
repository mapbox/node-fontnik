#include <node.h>
#include <node_buffer.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include "font.hpp"

using namespace v8;



FT_Library library() {
    static FT_Library library = NULL;
    if (library == NULL) {
        FT_Error error = FT_Init_FreeType(&library);
        assert(error == 0 && "failed to initialize freetype");
    }
    return library;
}


Persistent<FunctionTemplate> Font::constructor;

void Font::Init(Handle<Object> target) {
    HandleScope scope;

    Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
    Local<String> name = String::NewSymbol("Font");

    constructor = Persistent<FunctionTemplate>::New(tpl);
    // ObjectWrap uses the first internal field to store the wrapped pointer.
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(name);

    // Add all prototype methods, getters and setters here.
    // NODE_SET_PROTOTYPE_METHOD(constructor, "value", Value);
    constructor->InstanceTemplate()->SetIndexedPropertyHandler(GetGlyph);

    // This has to be last, otherwise the properties won't show up on the
    // object in JavaScript.
    target->Set(name, constructor->GetFunction());
}

const int Font::size = 24;
const int Font::buffer = 3;

Font::Font(const char *data, size_t length, int index)
    : ObjectWrap(),
    face(NULL) {
    error = FT_New_Memory_Face(library(), (const FT_Byte *)data, length, index, &face);
    if (error) return;

    error = FT_Set_Char_Size(face, 0, size * 64, 72, 72);
    if (error) return;

    slot = face->glyph;
}

Font::~Font() {
    if (face) {
        FT_Done_Face(face);
    }
}

Handle<Value> Font::New(const Arguments& args) {
    HandleScope scope;

    if (!args.IsConstructCall()) {
        return ThrowException(Exception::TypeError(
            String::New("Use the new operator to create instances of this object."))
        );
    }

    if (args.Length() < 1 || !node::Buffer::HasInstance(args[0])) {
        return ThrowException(Exception::TypeError(
            String::New("First argument must be a font blob")));
    }

    Local<Object> buffer = args[0]->ToObject();
    Font* font = new Font(node::Buffer::Data(buffer), node::Buffer::Length(buffer));
    if (font->error != 0) {
        return ThrowException(Exception::Error(
            String::New("Loading the font failed")));
    } else {
        font->Wrap(args.This());
        args.This()->Set(String::NewSymbol("family"), String::New(font->face->family_name), ReadOnly);
        args.This()->Set(String::NewSymbol("style"), String::New(font->face->style_name), ReadOnly);
        args.This()->Set(String::NewSymbol("length"), Number::New(font->face->num_glyphs), ReadOnly);
        return args.This();
    }
}

bool Font::HasInstance(Handle<Value> val) {
    if (!val->IsObject()) return false;
    return constructor->HasInstance(val->ToObject());
}

Handle<Value> Font::GetGlyph(uint32_t glyph_index, const v8::AccessorInfo& info)
{
    HandleScope scope;
    Font* font = ObjectWrap::Unwrap<Font>(info.This());
    FT_Error error = FT_Load_Glyph(font->face, glyph_index, FT_LOAD_NO_HINTING | FT_LOAD_RENDER);
    if (error) {
        return Undefined();
    } else {
        Local<Object> result = Object::New();
        result->Set(String::NewSymbol("width"), Number::New(font->slot->bitmap.width), ReadOnly);
        result->Set(String::NewSymbol("height"), Number::New(font->slot->bitmap.rows), ReadOnly);
        result->Set(String::NewSymbol("left"), Number::New(font->slot->bitmap_left), ReadOnly);
        result->Set(String::NewSymbol("top"), Number::New(font->slot->bitmap_top), ReadOnly);
        result->Set(String::NewSymbol("advance"), Number::New(font->slot->metrics.horiAdvance), ReadOnly);
        return scope.Close(result);
    }
}
