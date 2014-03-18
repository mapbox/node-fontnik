#include <node.h>
#include "pango_engine.hpp"
#include "pango_shaper.hpp"
#include "tile.hpp"

typedef PangoEngine Font;

void RegisterModule(v8::Handle<v8::Object> target) {
    InitShaping(target);
    Font::Init(target);
    Tile::Init(target);
}

NODE_MODULE(fontserver, RegisterModule);
