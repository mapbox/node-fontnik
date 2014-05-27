#pragma once

#include "tile_face.hpp"

#include <node.h>

#include "glyphs.pb.h"

class Tile : public node::ObjectWrap {
public:
    static v8::Persistent<v8::FunctionTemplate> constructor;
    static void Init(v8::Handle<v8::Object> target);
    static bool HasInstance(v8::Handle<v8::Value> val);
protected:
    Tile();
    Tile(const char *data, size_t length);
    ~Tile();

    static v8::Handle<v8::Value> New(const v8::Arguments& args);
    static v8::Handle<v8::Value> Serialize(const v8::Arguments& args);
    static v8::Handle<v8::Value> Range(const v8::Arguments& args);
    static void AsyncRange(uv_work_t* req);
    static void RangeAfter(uv_work_t* req);
public:
    llmr::glyphs::glyphs glyphs;
};
