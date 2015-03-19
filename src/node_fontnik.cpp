// node-mapnik
#include "mapnik_fonts.hpp"

// fontnik
#include <node_fontnik/glyphs.hpp>

// node
#include <node.h>
#include <nan.h>

namespace node_fontnik
{

void RegisterModule(v8::Handle<v8::Object> target) {
    NODE_SET_METHOD(target, "register_fonts", node_mapnik_fontnik::register_fonts);
    NODE_SET_METHOD(target, "faces", node_mapnik_fontnik::available_font_faces);
    NODE_SET_METHOD(target, "files", node_mapnik_fontnik::available_font_files);
    NODE_SET_METHOD(target, "load", node_fontnik::Load);
    // NODE_SET_METHOD(target, "range", node_fontnik::Range);

    Glyphs::Init(target);
}

NODE_MODULE(fontnik, RegisterModule);

} // ns node_fontnik
