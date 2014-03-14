#include <node.h>
#include <node_buffer.h>

#include "freetype_engine.hpp"
#include "freetype_font.hpp"
#include "distmap.h"
#include "globals.hpp"

#include "metrics.pb.h"

// stl
#include <iostream>
#include <fstream>
#include <memory>


// freetype2
extern "C"
{
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_STROKER_H
}


v8::Persistent<v8::FunctionTemplate> FreetypeEngine::constructor;

void FreetypeEngine::Init(v8::Handle<v8::Object> target) {
    v8::HandleScope scope;

    v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(New);
    v8::Local<v8::String> name = v8::String::NewSymbol("FreetypeEngine");

    constructor = v8::Persistent<v8::FunctionTemplate>::New(tpl);

    // node::ObjectWrap uses the first internal field to store the wrapped pointer.
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(name);

    // Add all prototype methods, getters and setters here.
    constructor->PrototypeTemplate()->SetAccessor(v8::String::NewSymbol("metrics"), Metrics);
    constructor->PrototypeTemplate()->SetIndexedPropertyHandler(GetGlyph);

    // This has to be last, otherwise the properties won't show up on the
    // object in JavaScript.
    target->Set(name, constructor->GetFunction());
}

const int FreetypeEngine::size = 24;
const int FreetypeEngine::buffer = 3;

FreetypeEngine::FreetypeEngine()
    : node::ObjectWrap(),
    library_(nullptr) {
    FT_Error error = FT_Init_FreeType( &library_ );
    if (error)
    {
        throw std::runtime_error("can not load FreeType2 library");
    }
}

FreetypeEngine::~FreetypeEngine() {
    FT_Done_FreeType(library_);
}

v8::Handle<v8::Value> FreetypeEngine::New(const v8::Arguments& args) {
    if (!args.IsConstructCall()) {
        return ThrowException(v8::Exception::TypeError(v8::String::New("Constructor must be called with new keyword")));
    }
    if (args.Length() != 1) {
        return ThrowException(v8::Exception::TypeError(v8::String::New("Constructor must be called with new keyword")));
    }

    FT_Face ft_face = nullptr;
    if (args[0]->IsExternal()) {
        ft_face = (FT_Face)v8::External::Cast(*args[0])->Value();
    } else {
        v8::String::Utf8Value font_name(args[0]->ToString());
        /*
        PangoFontDescription *desc = pango_font_description_from_string(*font_name);
        pango_font = pango_font_map_load_font(pango_fontmap(), pango_context(), desc);
        */
    }

    if (ft_face) {
        FT_Font* font = FT_Font::New(ft_face);
        // v8::Local<v8::Object> font = v8::Object::New();
        font->Wrap(args.This());
        args.This()->Set(v8::String::NewSymbol("family"), v8::String::New(ft_face->family_name), v8::ReadOnly);
        args.This()->Set(v8::String::NewSymbol("style"), v8::String::New(ft_face->style_name), v8::ReadOnly);
        args.This()->Set(v8::String::NewSymbol("length"), v8::Number::New(ft_face->num_glyphs), v8::ReadOnly);

        return args.This();
    } else {
        return ThrowException(v8::Exception::Error(v8::String::New("No matching font found")));
    }
}

v8::Handle<v8::Value> FreetypeEngine::New(std::string const& family_name) {
    v8::HandleScope scope;

    std::map<std::string, std::pair<int,std::string> >::const_iterator itr;
    itr = name2file_.find(family_name);
    if (itr != name2file_.end())
    {
        FT_Face face;

        std::map<std::string,std::string>::const_iterator mem_font_itr = memory_fonts_.find(itr->second.second);

        if (mem_font_itr != memory_fonts_.end()) // memory font
        {
            FT_Error error = FT_New_Memory_Face(library_,
                                                reinterpret_cast<FT_Byte const*>(mem_font_itr->second.c_str()),
                                                static_cast<FT_Long>(mem_font_itr->second.size()), // size
                                                itr->second.first, // face index
                                                &face);

            // if (!error) return std::make_shared<font_face>(face);
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
                                                 &face);
            /*
            if (!error) return std::make_shared<font_face>(face);
            else
            {
                // we can't load font, erase it.
                memory_fonts_.erase(result.first);
            }
            */
        }
        // return face_ptr();
    }

    // v8::Local<v8::Value> value = v8::External::New(family_name);
    // v8::Local<v8::Object> object = constructor->GetFunction()->NewInstance(1, &value);

    // return scope.Close(object);
}

std::map<std::string,std::pair<int,std::string> > FreetypeEngine::name2file_;
std::map<std::string,std::string> FreetypeEngine::memory_fonts_;
