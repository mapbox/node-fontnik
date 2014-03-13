#include <node.h>
#include "pango_engine.hpp"
#include "pango_shaper.hpp"
#include "tile.hpp"

using namespace v8;

typedef PangoEngine Font;

void RegisterModule(Handle<Object> target) {
    InitShaping(target);
    Font::Init(target);
    Tile::Init(target);
}

NODE_MODULE(fontserver, RegisterModule);
