#pragma once

#include <node.h>

// freetype2
extern "C"
{
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_STROKER_H
}

struct FT_LibraryRec_;

class FreetypeEngine : public node::ObjectWrap {
public:
    static v8::Persistent<v8::FunctionTemplate> constructor;
    static void Init(v8::Handle<v8::Object> target);

    static v8::Handle<v8::Value> New(FT_Face face);
protected:
    explicit FreetypeEngine();
    ~FreetypeEngine();

    static v8::Handle<v8::Value> New(const v8::Arguments& args);
private:
    FT_LibraryRec_ *library_;
};
