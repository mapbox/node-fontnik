#ifndef NODE_FONTNIK_GLYPHS_HPP
#define NODE_FONTNIK_GLYPHS_HPP

#include <fontnik/glyphs.hpp>

#include <node.h>
#include <nan.h>

namespace node_fontnik
{

class Glyphs : public node::ObjectWrap {
public:
    static v8::Persistent<v8::FunctionTemplate> constructor;
    static void Init(v8::Handle<v8::Object> target);
    static bool HasInstance(v8::Handle<v8::Value> val);
protected:
    Glyphs();
    Glyphs(const char *data, size_t length);
    ~Glyphs();

    static NAN_METHOD(New);
    static NAN_METHOD(Serialize);
    static NAN_METHOD(Range);
    static NAN_METHOD(Codepoints);
    static void AsyncRange(uv_work_t* req);
    static void RangeAfter(uv_work_t* req);

    fontnik::Glyphs glyphs;
};

} // ns node_fontnik

#endif // NODE_FONTNIK_GLYPHS_HPP
