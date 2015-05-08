// fontnik
#include "glyphs.hpp"

// node
#include <node.h>
#include <nan.h>

namespace node_fontnik
{

void RegisterModule(v8::Handle<v8::Object> target) {
    NODE_SET_METHOD(target, "load", node_fontnik::Load);
    NODE_SET_METHOD(target, "table", node_fontnik::Table);
    NODE_SET_METHOD(target, "range", node_fontnik::Range);

}

NODE_MODULE(fontnik, RegisterModule);

} // ns node_fontnik
