#include <node.h>
#include "pango_shaper.hpp"
#include "ft_font.hpp"
#include "tile.hpp"

typedef FT_Font Font;

void RegisterModule(v8::Handle<v8::Object> target) {
    InitShaping(target);
    Font::Init(target);
    Tile::Init(target);
}

NODE_MODULE(fontserver, RegisterModule);
