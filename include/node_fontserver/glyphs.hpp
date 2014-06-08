#ifndef NODE_FONTSERVER_GLYPHS_HPP
#define NODE_FONTSERVER_GLYPHS_HPP

#include <fontserver/glyphs.hpp>

#include <node.h>

namespace node_fontserver
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

    static v8::Handle<v8::Value> New(const v8::Arguments& args);
    static v8::Handle<v8::Value> Serialize(const v8::Arguments& args);
    static v8::Handle<v8::Value> Range(const v8::Arguments& args);
    static void AsyncRange(uv_work_t* req);
    static void RangeAfter(uv_work_t* req);

    fontserver::Glyphs glyphs;
};

} // ns node_fontserver

#endif // NODE_FONTSERVER_GLYPHS_HPP
