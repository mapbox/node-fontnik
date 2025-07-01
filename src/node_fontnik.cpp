// fontnik
#include "glyphs.hpp"
#include <napi.h>
//#include <uv.h>

namespace node_fontnik {

Napi::Object init(Napi::Env env, Napi::Object exports) {
    exports.Set("load", Napi::Function::New(env, Load));
    exports.Set("range", Napi::Function::New(env, Range));
    exports.Set("composite", Napi::Function::New(env, Composite));
    return exports;
}

// We mark this NOLINT to avoid the clang-tidy checks
// warning about code inside nodejs that we don't control and can't
// directly change to avoid the warning.
NODE_API_MODULE(fontnik, init) // NOLINT

} // namespace node_fontnik
