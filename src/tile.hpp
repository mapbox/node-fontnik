#pragma once

#include <node.h>

#include <pthread.h>
#include "harfbuzz_shaper.hpp"

#include "vector_tile.pb.h"

// freetype2
extern "C"
{
#include <ft2build.h>
#include FT_FREETYPE_H
// #include FT_STROKER_H
}

class Tile : public node::ObjectWrap {
public:
    static v8::Persistent<v8::FunctionTemplate> constructor;
    static void Init(v8::Handle<v8::Object> target);
    static bool HasInstance(v8::Handle<v8::Value> val);
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
    static void AsyncShape(uv_work_t* req);
    static void ShapeAfter(uv_work_t* req);
private:
    static HarfbuzzShaper *shaper;
    HarfbuzzShaper *shaper_;

    static pthread_once_t init;

    static pthread_key_t library_key;
    static void InitLibrary() {
        pthread_key_create(&library_key, DestroyLibrary);
    }
    static void DestroyLibrary(void *key) {
        FT_Library library = static_cast<FT_Library>(key);
        FT_Done_FreeType(library);
    }

    static pthread_key_t shaper_key;
    static void InitShaper() {
        pthread_key_create(&shaper_key, nullptr);
    }

public:
    llmr::vector::tile tile;
    pthread_mutex_t mutex;
};
