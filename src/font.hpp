#ifndef FONT_HPP
#define FONT_HPP

#include <node.h>

#include <ft2build.h>
#include FT_FREETYPE_H

class Font : public node::ObjectWrap {
public:
    static v8::Persistent<v8::FunctionTemplate> constructor;
    static void Init(v8::Handle<v8::Object> target);
    static bool HasInstance(v8::Handle<v8::Value> val);

protected:
    Font(const char *data, size_t length, int index = 0);
    ~Font();

    static v8::Handle<v8::Value> New(const v8::Arguments& args);
    static v8::Handle<v8::Value> GetGlyph(uint32_t, const v8::AccessorInfo& info);

    static const int size;
    static const int buffer;

public:
    FT_Error error;
    FT_Face face;
    FT_GlyphSlot slot;
};

#endif
