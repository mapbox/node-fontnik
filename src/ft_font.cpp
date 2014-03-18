#include <node_buffer.h>

#include "ft_font.hpp"
#include "distmap.h"
#include "globals.hpp"

#include "metrics.pb.h"

// stl
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <memory>

const int FT_Font::size = 24;
const int FT_Font::buffer = 3;

v8::Persistent<v8::FunctionTemplate> FT_Font::constructor;

FT_Font::FT_Font(FT_Face ft_face)
    : node::ObjectWrap(),
    library_(nullptr) {
    FT_Error error = FT_Init_FreeType(&library_);
    if (error)
    {
        throw std::runtime_error("can not load FreeType2 library");
    }
    FT_Reference_Face(ft_face);
}

FT_Font::~FT_Font() {
    FT_Done_Face(ft_face);
    FT_Done_FreeType(library_);
}

void FT_Font::Init(v8::Handle<v8::Object> target) {
    // Prepare constructor template
    v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(New);
    v8::Local<v8::String> name = v8::String::NewSymbol("FT_Font");

    constructor = v8::Persistent<v8::FunctionTemplate>::New(tpl);

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

v8::Handle<v8::Value> FT_Font::New(const v8::Arguments& args) {
    if (!args.IsConstructCall()) {
        return ThrowException(v8::Exception::TypeError(v8::String::New("Constructor must be called with new keyword")));
    }
    if (args.Length() != 1) {
        return ThrowException(v8::Exception::TypeError(v8::String::New("Constructor must be called with new keyword")));
    }

    FT_Face ft_face = NULL;
    if (args[0]->IsExternal()) {
        ft_face = (FT_Face)v8::External::Cast(*args[0])->Value();
    } else {
        v8::String::Utf8Value font_name(args[0]->ToString());
        /*
        PangoFontDescription *desc = pango_font_description_from_string(*font_name);
        pango_font = pango_font_map_load_font(pango_fontmap(), pango_context(), desc);

        std::map<std::string, std::pair<int,std::string> >::const_iterator itr;
        itr = name2file_.find(family_name);
        if (itr != name2file_.end()) {
            FT_Face ft_face;

            std::map<std::string,std::string>::const_iterator mem_font_itr = memory_fonts_.find(itr->second.second);

            if (mem_font_itr != memory_fonts_.end()) // memory font
            {
                FT_Error error = FT_New_Memory_Face(library_,
                                                    reinterpret_cast<FT_Byte const*>(mem_font_itr->second.c_str()),
                                                    static_cast<FT_Long>(mem_font_itr->second.size()), // size
                                                    itr->second.first, // face index
                                                    &ft_face);

                if (!error) return std::make_shared<font_face>(ft_face);
            }
            else
            {
                // load font into memory
#ifdef MAPNIK_THREADSAFE
                mapnik::scoped_lock lock(mutex_);
#endif
                std::ifstream is(itr->second.second.c_str() , std::ios::binary);
                std::string buffer((std::istreambuf_iterator<char>(is)),
                                   std::istreambuf_iterator<char>());
                std::pair<std::map<std::string,std::string>::iterator,bool> result
                    = memory_fonts_.insert(std::make_pair(itr->second.second, buffer));

                FT_Error error = FT_New_Memory_Face (library_,
                                                     reinterpret_cast<FT_Byte const*>(result.first->second.c_str()),
                                                     static_cast<FT_Long>(buffer.size()),
                                                     itr->second.first,
                                                     &ft_face);
                if (!error) return std::make_shared<font_face>(ft_face);
                else
                {
                    // we can't load font, erase it.
                    memory_fonts_.erase(result.first);
                }
            }
            return face_ptr();
        }
        */
    }

    if (ft_face) {
        FT_Font* font = new FT_Font(ft_face);
        font->Wrap(args.This());
        args.This()->Set(v8::String::NewSymbol("family"), v8::String::New(ft_face->family_name), v8::ReadOnly);
        args.This()->Set(v8::String::NewSymbol("style"), v8::String::New(ft_face->style_name), v8::ReadOnly);
        args.This()->Set(v8::String::NewSymbol("length"), v8::Number::New(ft_face->num_glyphs), v8::ReadOnly);

        return args.This();
    } else {
        return ThrowException(v8::Exception::Error(v8::String::New("No matching font found")));
    }
}

v8::Handle<v8::Value> FT_Font::New(FT_Face ft_face) {
    v8::HandleScope scope;

    v8::Local<v8::Value> value = v8::External::New(ft_face);
    v8::Local<v8::Object> object = constructor->GetFunction()->NewInstance(1, &value);

    return scope.Close(object);
}

bool FT_Font::HasInstance(v8::Handle<v8::Value> value) {
    if (!value->IsObject()) return false;
    return constructor->HasInstance(value->ToObject());
}

v8::Handle<v8::Value> FT_Font::GetGlyph(uint32_t glyph_index, const v8::AccessorInfo& info)
{
    v8::HandleScope scope;
    FT_Font* font = node::ObjectWrap::Unwrap<FT_Font>(info.This());
    FT_Face ft_face = font->ft_face;
    // fprintf(stderr, "x/y/h: %ld/%ld/%ld\n", ft_face->size->metrics.x_scale, ft_face->size->metrics.y_scale / 2048, ft_face->size->metrics.height);
    FT_Set_Char_Size(ft_face, 0, size * 64, 72, 72);
    FT_Error error = FT_Load_Glyph(ft_face, glyph_index, FT_LOAD_NO_HINTING | FT_LOAD_RENDER);
    if (error) {
        FT_Done_Face(ft_face);
        return v8::Undefined();
    } else {
        v8::Local<v8::Object> result = v8::Object::New();
        result->Set(v8::String::NewSymbol("id"), v8::Uint32::New(glyph_index), v8::ReadOnly);
        result->Set(v8::String::NewSymbol("width"), v8::Number::New(ft_face->glyph->bitmap.width), v8::ReadOnly);
        result->Set(v8::String::NewSymbol("height"), v8::Number::New(ft_face->glyph->bitmap.rows), v8::ReadOnly);
        result->Set(v8::String::NewSymbol("left"), v8::Number::New(ft_face->glyph->bitmap_left), v8::ReadOnly);
        result->Set(v8::String::NewSymbol("top"), v8::Number::New(ft_face->glyph->bitmap_top), v8::ReadOnly);
        result->Set(v8::String::NewSymbol("advance"), v8::Number::New(ft_face->glyph->metrics.horiAdvance), v8::ReadOnly);

        FT_GlyphSlot slot = ft_face->glyph;
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

            result->Set(v8::String::NewSymbol("bitmap"), node::Buffer::New((const char *)map, buffered_width * buffered_height)->handle_);
            free(map);
        }

        FT_Done_Face(ft_face);
        return scope.Close(result);
    }
}

v8::Handle<v8::Value> FT_Font::Metrics(v8::Local<v8::String> property, const v8::AccessorInfo &info) {
    v8::HandleScope scope;

    FT_Font* font = node::ObjectWrap::Unwrap<FT_Font>(info.This());
    FT_Face ft_face = font->ft_face;

    FT_Error error = FT_Set_Char_Size(ft_face, 0, size * 64, 72, 72);
    if (error) {
        return ThrowException(v8::Exception::Error(v8::String::New("Could not set char size")));
    }

    // Generate a list of all glyphs
    llmr::face metrics;
    metrics.set_family(ft_face->family_name);
    metrics.set_style(ft_face->style_name);

    for (int glyph_index = 0; glyph_index < ft_face->num_glyphs; glyph_index++) {
        FT_Error error = FT_Load_Glyph(ft_face, glyph_index, FT_LOAD_NO_HINTING | FT_LOAD_RENDER);
        if (error) {
            return ThrowException(v8::Exception::Error(v8::String::New("Could not load glyph")));
        }

        llmr::glyph *glyph = metrics.add_glyphs();
        glyph->set_id(glyph_index);
        glyph->set_width((unsigned int)ft_face->glyph->bitmap.width);
        glyph->set_height((unsigned int)ft_face->glyph->bitmap.rows);
        glyph->set_left((unsigned int)ft_face->glyph->bitmap_left);
        glyph->set_top((unsigned int)ft_face->glyph->bitmap_top);
        glyph->set_advance(ft_face->glyph->metrics.horiAdvance / 64);
    }

    FT_Done_Face(ft_face);

    std::string serialized = metrics.SerializeAsString();
    return scope.Close(node::Buffer::New(serialized.data(), serialized.length())->handle_);
}

std::map<std::string,std::pair<int,std::string> > FT_Font::name2file_;
std::map<std::string,std::string> FT_Font::memory_fonts_;
