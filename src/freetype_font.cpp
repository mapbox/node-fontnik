#include <node.h>
#include "freetype_font.h"

v8::Persistent<v8::Function> FT_Font::constructor;

FT_Font::FT_Font(FT_Face face) : node::ObjectWrap() {
}

FT_Font::~FT_Font() {
    FT_Done_Face(ft_face);
}

void FT_Font::Init() {
    v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(New);
    v8::Local<v8::String> name = v8::String::NewSymbol("FT_Font");

    constructor = v8::Persistent<v8::Function>::New(tpl->GetFunction());

    constructor->SetClassName(v8::String::NewSymbol("FT_Font"));
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
}

v8::Handle<v8::Value> FT_Font::New(FT_Face ft_face) {
    v8::HandleScope scope;

    v8::Local<v8::Value> value = v8::External::New(ft_face);
    v8::Local<v8::Object> object = constructor->GetFunction()->NewInstance(1, &value);

    return scope.Close(object);
}

bool FreetypeEngine::HasInstance(v8::Handle<v8::Value> val) {
    if (!val->IsObject()) return false;
    return constructor->HasInstance(val->ToObject());
}
