#pragma once

#include <node.h>

#include "font_engine_freetype.hpp"

#include "vector_tile.pb.h"

class Tile : public node::ObjectWrap {
public:
    static v8::Persistent<v8::FunctionTemplate> constructor;
    static void Init(v8::Handle<v8::Object> target);
    static bool HasInstance(v8::Handle<v8::Value> val);
    void AsyncShape(uv_work_t* req);
protected:
    Tile(const char *data, size_t length);
    ~Tile();

    static v8::Handle<v8::Value> New(const v8::Arguments& args);
    static v8::Handle<v8::Value> Length(v8::Local<v8::String> property, const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> Serialize(const v8::Arguments& args);

    static v8::Handle<v8::Value> Simplify(const v8::Arguments& args);
    static void AsyncSimplify(uv_work_t* req);
    static void SimplifyAfter(uv_work_t* req);

    static v8::Handle<v8::Value> Shape(const v8::Arguments& args);
    static void AsyncShapeWrapper(uv_work_t* req);
    static void ShapeAfter(uv_work_t* req);

    freetype_engine font_engine_;
    face_manager_freetype font_manager;

public:
    llmr::vector::tile tile;
    pthread_mutex_t mutex;
};
