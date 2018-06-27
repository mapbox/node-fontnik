// fontnik
#include "glyphs.hpp"
#include <nan.h>

namespace node_fontnik {

NAN_MODULE_INIT(RegisterModule) {
    target->Set(Nan::New("load").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(Load)->GetFunction());
    target->Set(Nan::New("range").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(Range)->GetFunction());
}

// We mark this NOLINT to avoid the clang-tidy checks
// warning about code inside nodejs that we don't control and can't
// directly change to avoid the warning.
NODE_MODULE(fontnik, RegisterModule) // NOLINT

} // namespace node_fontnik
