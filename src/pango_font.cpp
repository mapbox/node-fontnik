#include <node.h>
#include <node_buffer.h>

#include "pango_font.hpp"
#include "distmap.h"
#include "globals.hpp"

#include "metrics.pb.h"

using namespace v8;


Persistent<FunctionTemplate> Pango_Font::constructor;

void Pango_Font::Init(Handle<Object> target) {
    HandleScope scope;

    Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
    Local<String> name = String::NewSymbol("Pango_Font");

    constructor = Persistent<FunctionTemplate>::New(tpl);

    // ObjectWrap uses the first internal field to store the wrapped pointer.
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(name);

    // Add all prototype methods, getters and setters here.
    constructor->PrototypeTemplate()->SetAccessor(v8::String::NewSymbol("metrics"), Metrics);
    constructor->PrototypeTemplate()->SetIndexedPropertyHandler(GetGlyph);

    // This has to be last, otherwise the properties won't show up on the
    // object in JavaScript.
    target->Set(name, constructor->GetFunction());
}

const int Pango_Font::size = 24;
const int Pango_Font::buffer = 3;

Pango_Font::Pango_Font(PangoFont *pango_font)
    : ObjectWrap(),
    font(pango_font) {
    g_object_ref(font);
}

Pango_Font::~Pango_Font() {
    g_object_unref(font);
}

Handle<Value> Pango_Font::New(const v8::Arguments& args) {
    if (!args.IsConstructCall()) {
        return ThrowException(Exception::TypeError(String::New("Constructor must be called with new keyword")));
    }
    if (args.Length() != 1) {
        return ThrowException(Exception::TypeError(String::New("Constructor must be called with new keyword")));
    }

    PangoFont *pango_font = NULL;
    if (args[0]->IsExternal()) {
        pango_font = (PangoFont *)External::Cast(*args[0])->Value();
    } else {
        String::Utf8Value font_name(args[0]->ToString());
        PangoFontDescription *desc = pango_font_description_from_string(*font_name);
        pango_font = pango_font_map_load_font(pango_fontmap(), pango_context(), desc);
    }

    if (pango_font) {
        PangoFcFont *fc_font = PANGO_FC_FONT(pango_font);
        Pango_Font* font = new Pango_Font(pango_font);
        font->Wrap(args.This());
        FT_Face face = pango_fc_font_lock_face(fc_font);
        args.This()->Set(String::NewSymbol("family"), String::New(face->family_name), ReadOnly);
        args.This()->Set(String::NewSymbol("style"), String::New(face->style_name), ReadOnly);
        args.This()->Set(String::NewSymbol("length"), Number::New(face->num_glyphs), ReadOnly);
        pango_fc_font_unlock_face(fc_font);

        return args.This();
    } else {
        return ThrowException(Exception::Error(String::New("No matching font found")));
    }
}

Handle<Value> Pango_Font::New(PangoFont* pango_font) {
    HandleScope scope;

    Local<Value> value = External::New(pango_font);
    Local<Object> object = constructor->GetFunction()->NewInstance(1, &value);

    return scope.Close(object);
}

bool Pango_Font::HasInstance(Handle<Value> val) {
    if (!val->IsObject()) return false;
    return constructor->HasInstance(val->ToObject());
}

Handle<Value> Pango_Font::GetGlyph(uint32_t glyph_index, const v8::AccessorInfo& info)
{
    HandleScope scope;
    Pango_Font* font = ObjectWrap::Unwrap<Pango_Font>(info.This());
    PangoFcFont *fc_font = PANGO_FC_FONT(font->font);
    FT_Face face = pango_fc_font_lock_face(fc_font);
    // fprintf(stderr, "x/y/h: %ld/%ld/%ld\n", face->size->metrics.x_scale, face->size->metrics.y_scale / 2048, face->size->metrics.height);
    FT_Set_Char_Size(face, 0, size * 64, 72, 72);
    FT_Error error = FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_HINTING | FT_LOAD_RENDER);
    if (error) {
        pango_fc_font_unlock_face(fc_font);
        return Undefined();
    } else {
        Local<Object> result = Object::New();
        result->Set(String::NewSymbol("id"), Uint32::New(glyph_index), ReadOnly);
        result->Set(String::NewSymbol("width"), Number::New(face->glyph->bitmap.width), ReadOnly);
        result->Set(String::NewSymbol("height"), Number::New(face->glyph->bitmap.rows), ReadOnly);
        result->Set(String::NewSymbol("left"), Number::New(face->glyph->bitmap_left), ReadOnly);
        result->Set(String::NewSymbol("top"), Number::New(face->glyph->bitmap_top), ReadOnly);
        result->Set(String::NewSymbol("advance"), Number::New(face->glyph->metrics.horiAdvance), ReadOnly);

        FT_GlyphSlot slot = face->glyph;
        int width = slot->bitmap.width;
        int height = slot->bitmap.rows;

        // Create a signed distance field for the glyph bitmap.
        if (width > 0) {
            unsigned int buffered_width = width + 2 * buffer;
            unsigned int buffered_height = height + 2 * buffer;

            unsigned char *distance = make_distance_map((unsigned char *)slot->bitmap.buffer, width, height, buffer);

            unsigned char *map = (unsigned char *)malloc(buffered_width * buffered_height);
            for (unsigned int y = 0; y < buffered_height; y++) {
                memcpy(map + buffered_width * y, distance + y * distmap_size, buffered_width);
            }
            free(distance);

            result->Set(String::NewSymbol("bitmap"), node::Buffer::New((const char *)map, buffered_width * buffered_height)->handle_);
            free(map);
        }

        pango_fc_font_unlock_face(fc_font);
        return scope.Close(result);
    }
}

Handle<Value> Pango_Font::Metrics(Local<String> property, const AccessorInfo &info)
{
    HandleScope scope;

    Pango_Font* font = ObjectWrap::Unwrap<Pango_Font>(info.This());
    PangoFcFont *fc_font = PANGO_FC_FONT(font->font);
    FT_Face face = pango_fc_font_lock_face(fc_font);

    FT_Error error = FT_Set_Char_Size(face, 0, size * 64, 72, 72);
    if (error) {
        return ThrowException(Exception::Error(String::New("Could not set char size")));
    }

    // Generate a list of all glyphs
    llmr::face metrics;
    metrics.set_family(face->family_name);
    metrics.set_style(face->style_name);

    for (int glyph_index = 0; glyph_index < face->num_glyphs; glyph_index++) {
        FT_Error error = FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_HINTING | FT_LOAD_RENDER);
        if (error) {
            return ThrowException(Exception::Error(String::New("Could not load glyph")));
        }

        llmr::glyph *glyph = metrics.add_glyphs();
        glyph->set_id(glyph_index);
        glyph->set_width((unsigned int)face->glyph->bitmap.width);
        glyph->set_height((unsigned int)face->glyph->bitmap.rows);
        glyph->set_left((unsigned int)face->glyph->bitmap_left);
        glyph->set_top((unsigned int)face->glyph->bitmap_top);
        glyph->set_advance(face->glyph->metrics.horiAdvance / 64);
    }

    pango_fc_font_unlock_face(fc_font);

    std::string serialized = metrics.SerializeAsString();
    return scope.Close(node::Buffer::New(serialized.data(), serialized.length())->handle_);
}
