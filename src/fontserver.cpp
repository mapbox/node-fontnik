#include <node.h>

#include "mapnik_fonts.hpp"
#include "tile.hpp"

void RegisterModule(v8::Handle<v8::Object> target) {
    NODE_SET_METHOD(target, "register_fonts", node_mapnik::register_fonts);
    Tile::Init(target);
}

NODE_MODULE(fontserver, RegisterModule);
