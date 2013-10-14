#pragma once

#include <node.h>

#include <pango/pangoft2.h>

class Font : public node::ObjectWrap {
public:
    static v8::Persistent<v8::FunctionTemplate> constructor;
    static void Init(v8::Handle<v8::Object> target);
    static bool HasInstance(v8::Handle<v8::Value> val);

    static v8::Handle<v8::Value> New(PangoFont *pango_font);
protected:
    Font(PangoFont *pango_font);
    ~Font();

    static v8::Handle<v8::Value> New(const v8::Arguments& args);
    static v8::Handle<v8::Value> GetGlyph(uint32_t, const v8::AccessorInfo& info);
    static v8::Handle<v8::Value> Metrics(v8::Local<v8::String> property, const v8::AccessorInfo &info);

    static const int size;
    static const int buffer;

public:
    PangoFont *font;
};
