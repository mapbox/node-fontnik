// fontnik
#include "glyphs.hpp"

// node
#include <node.h>
#include <nan.h>

namespace node_fontnik
{

NAN_MODULE_INIT(RegisterModule) {
    target->Set(Nan::New("load").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(Load)->GetFunction());
    target->Set(Nan::New("range").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(Range)->GetFunction());
}

NODE_MODULE(fontnik, RegisterModule);

} // ns node_fontnik
