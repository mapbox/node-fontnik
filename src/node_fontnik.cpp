// fontnik
#include "glyphs.hpp"
#include <nan.h>

namespace node_fontnik {

static void init(v8::Local<v8::Object> target) {
    Nan::SetMethod(target, "load", Load);
    Nan::SetMethod(target, "range", Range);
    Nan::SetMethod(target, "composite", Composite);
}

// We mark this NOLINT to avoid the clang-tidy checks
// warning about code inside nodejs that we don't control and can't
// directly change to avoid the warning.
NODE_MODULE(fontnik, init) // NOLINT

} // namespace node_fontnik
