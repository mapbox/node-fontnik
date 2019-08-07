#pragma once

#include "glyphs.pb.h"
#include <nan.h>

namespace node_fontnik {

NAN_METHOD(Load);
void LoadAsync(uv_work_t* req);
void AfterLoad(uv_work_t* req);
NAN_METHOD(Range);
void RangeAsync(uv_work_t* req);
void AfterRange(uv_work_t* req);

const static int char_size = 24;
const static int buffer_size = 3;
const static float cutoff_size = 0.25;
const static float scale_factor = 1.0;

} // namespace node_fontnik
