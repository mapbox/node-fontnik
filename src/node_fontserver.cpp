// node-mapnik
#include <node_mapnik/mapnik_fonts.hpp>

// fontserver
#include <node_fontserver/glyphs.hpp>

// node
#include <node.h>
#include <nan.h>

namespace node_fontserver
{

void RegisterModule(v8::Handle<v8::Object> target) {
    NODE_SET_METHOD(target, "register_fonts", node_mapnik::register_fonts);
    NODE_SET_METHOD(target, "faces", node_mapnik::available_font_faces);
    NODE_SET_METHOD(target, "files", node_mapnik::available_font_files);

    Glyphs::Init(target);
}

NODE_MODULE(fontserver, RegisterModule);

} // ns node_fontserver
