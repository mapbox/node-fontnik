#include <node.h>
#include "freetype_font.h"

v8::Persistent<v8::Function> FT_Font::constructor;

FT_Font::FT_Font(FT_Face face) {
}

FT_Font::~FT_Font() {
    FT_Done_Face(ft_face);
}

void FT_Font::Init() {
    // Prepare constructor template
    v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(New);
    Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
    tpl->SetClassName(String::NewSymbol("FT_Font"));
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    // Prototype
    tpl->PrototypeTemplate()->Set(String::NewSymbol("HasInstance"),
        FunctionTemplate::New(HasInstance)->GetFunction());
    constructor = v8::Persistent<v8::Function>::New(tpl->GetFunction());
}

v8::Handle<v8::Value> FT_Font::New(FT_Face ft_face) {
    v8::HandleScope scope;

    v8::Local<v8::Value> value = v8::External::New(ft_face);
    v8::Local<v8::Object> object = constructor->GetFunction()->NewInstance(1, &value);

    return scope.Close(object);
}

v8::Handle<v8::Value> FT_Font::New(const v8::Arguments& args) {
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

v8::Handle<v8::Value> FT_Font::NewInstance(const v8::Arguments& args) {
  v8::HandleScope scope;

  const unsigned argc = 1;
  v8::Handle<v8::Value> argv[argc] = { args[0] };
  v8::Local<v8::Object> instance = constructor->NewInstance(argc, argv);

  return scope.Close(instance);
}

bool FT_Font::HasInstance(v8::Handle<v8::Value> val) {
    if (!val->IsObject()) return false;
    return constructor->HasInstance(val->ToObject());
}
