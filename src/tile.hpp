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

    // Width of a certain glyph cluster (in pixels).
    inline double cluster_width(unsigned cluster) const
    {
        std::map<unsigned, double>::const_iterator width_itr = width_map_.find(cluster);
        if (width_itr != width_map_.end()) return width_itr->second;
        return 0;
    }

    // Maps char index (UTF-16) to width. If multiple glyphs map to the
    // same char the sum of all widths is used.
    // Note: this probably isn't the best solution. it would be better
    // to have an object for each cluster, but it needs to be
    // implemented with no overhead.
    std::map<unsigned, double> width_map_;

public:
    llmr::vector::tile tile;
};
