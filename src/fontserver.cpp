#include <node.h>
#include "font.hpp"
#include "shaping.hpp"

using namespace v8;

void RegisterModule(Handle<Object> target) {
    InitShaping(target);
    Font::Init(target);
}

NODE_MODULE(fontserver, RegisterModule);
