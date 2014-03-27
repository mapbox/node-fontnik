#include <node.h>

#include "mapnik_fonts.hpp"
#include "tile.hpp"

void RegisterModule(v8::Handle<v8::Object> target) {
    NODE_SET_METHOD(target, "register_fonts", node_mapnik::register_fonts);
    NODE_SET_METHOD(target, "faces", node_mapnik::available_font_faces);
    NODE_SET_METHOD(target, "files", node_mapnik::available_font_files);
    Tile::Init(target);
}

NODE_MODULE(fontserver, RegisterModule);
