#include <node.h>
#include "tile.hpp"

void RegisterModule(v8::Handle<v8::Object> target) {
    Tile::Init(target);
}

NODE_MODULE(fontserver, RegisterModule);
