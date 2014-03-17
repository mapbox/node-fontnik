#include "freetype_font.h"

v8::Persistent<v8::Function> FT_Font::constructor;

FT_Font::FT_Font() {
}

FT_Font::~FT_Font() {
    FT_Done_Face(face);
}

void FT_Font::Init() {
    // Prepare constructor template
    v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(New);
    Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
    tpl->SetClassName(String::NewSymbol("FT_Font"));
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    // Prototype
    tpl->PrototypeTemplate()->Set(v8::String::NewSymbol("hasInstance"),
        FunctionTemplate::New(HasInstance)->GetFunction());
    tpl->PrototypeTemplate()->Set(v8::String::NewSymbol("getFamilyName"),
        FunctionTemplate::New(HasInstance)->GetFunction());
    tpl->PrototypeTemplate()->SetAccessor(v8::String::NewSymbol("metrics"), FunctionTemplate::New(Metrics)->GetFunction());
    tpl->PrototypeTemplate()->SetIndexedPropertyHandler(GetGlyph);


    constructor = v8::Persistent<v8::Function>::New(tpl->GetFunction());
}

v8::Handle<v8::Value> FT_Font::New(const v8::Arguments& args) {
    if (!args.IsConstructCall()) {
        return ThrowException(v8::Exception::TypeError(v8::String::New("Constructor must be called with new keyword")));
    }
    if (args.Length() != 1) {
        return ThrowException(v8::Exception::TypeError(v8::String::New("Constructor must be called with new keyword")));
    }

    if (args[0]->IsExternal()) {
        face = (FT_Face)v8::External::Cast(*args[0])->Value();
    } else {
        v8::String::Utf8Value font_name(args[0]->ToString());
        /*
        PangoFontDescription *desc = pango_font_description_from_string(*font_name);
        pango_font = pango_font_map_load_font(pango_fontmap(), pango_context(), desc);
        */
    }

    if (face) {
        FT_Font* font = new FT_Font(face);
        font->Wrap(args.This());
        args.This()->Set(v8::String::NewSymbol("family"), v8::String::New(face->family_name), v8::ReadOnly);
        args.This()->Set(v8::String::NewSymbol("style"), v8::String::New(face->style_name), v8::ReadOnly);
        args.This()->Set(v8::String::NewSymbol("length"), v8::Number::New(face->num_glyphs), v8::ReadOnly);

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

bool FT_Font::HasInstance(v8::Handle<v8::Value> value) {
    if (!value->IsObject()) return false;
    return constructor->HasInstance(value->ToObject());
}

v8::Handle<v8::Value> FT_Font::GetFamilyName(const v8::Arguments& args) {
    v8::HandleScope scope;

    FT_Font font = node::ObjectWrap::Unwrap<FT_Font>(args.This());

    return scope.Close(v8::String::New(font->face->family_name)); 
}
