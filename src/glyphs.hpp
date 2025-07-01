#pragma once

#include <napi.h>
#include <uv.h>

namespace node_fontnik {

Napi::Value Load(Napi::CallbackInfo const& info);
Napi::Value Range(Napi::CallbackInfo const& info);
Napi::Value Composite(Napi::CallbackInfo const& info);

} // namespace node_fontnik
