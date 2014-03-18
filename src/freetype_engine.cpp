#include <node.h>

#include "freetype_engine.hpp"
#include "freetype_font.hpp"

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

    // This has to be last, otherwise the properties won't show up on the
    // object in JavaScript.
    target->Set(name, constructor->GetFunction());
}

FreetypeEngine::FreetypeEngine()
    : node::ObjectWrap(),
    library_(nullptr) {
    FT_Error error = FT_Init_FreeType(&library_);
    if (error)
    {
        throw std::runtime_error("can not load FreeType2 library");
    }
}

FreetypeEngine::~FreetypeEngine() {
    FT_Done_FreeType(library_);
}

v8::Handle<v8::Value> FreetypeEngine::New(const v8::Arguments& args) {
    v8::HandleScope scope;
    return scope.Close(FT_Font::NewInstance(args));
}

v8::Handle<v8::Value> FreetypeEngine::New(FT_Face face) {
    v8::HandleScope scope;

    v8::Local<v8::Value> value = v8::External::New(face);
    v8::Local<v8::Object> object = constructor->GetFunction()->NewInstance(1, &value);

    return scope.Close(object);
}
