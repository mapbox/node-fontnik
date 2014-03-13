#pragma once

#include <node.h>
#include <string>

struct FT_LibraryRec_;

class FreetypeEngine : public node::ObjectWrap {
public:
    static v8::Persistent<v8::FunctionTemplate> constructor;
    static void Init(v8::Handle<v8::Object> target);
    static bool HasInstance(v8::Handle<v8::Value> val);

    static v8::Handle<v8::Value> New(std::string const& family_name);
protected:
    FreetypeEngine();
    ~FreetypeEngine();

    static v8::Handle<v8::Value> New(const v8::Arguments& args);
    static v8::Handle<v8::Value> GetGlyph(uint32_t, const v8::AccessorInfo& info);
    static v8::Handle<v8::Value> Metrics(v8::Local<v8::String> property, const v8::AccessorInfo &info);

    static const int size;
    static const int buffer;
private:
    FT_LibraryRec_ *library;
};
