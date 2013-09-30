#include <node.h>
#include "font.hpp"
#include "shaping.hpp"

using namespace v8;

void RegisterModule(Handle<Object> target) {
    Font::Init(target);
    target->Set(String::NewSymbol("shape"), FunctionTemplate::New(Shaping)->GetFunction());
}

NODE_MODULE(fontserver, RegisterModule);
