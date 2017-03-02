#ifndef NODE_FONTNIK_GLYPHS_HPP
#define NODE_FONTNIK_GLYPHS_HPP

#include "glyphs.pb.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#include <node.h>
#pragma GCC diagnostic pop

#include <nan.h>

namespace node_fontnik
{

NAN_METHOD(Load);
void LoadAsync(uv_work_t* req);
void AfterLoad(uv_work_t* req);
NAN_METHOD(Range);
void RangeAsync(uv_work_t* req);
void AfterRange(uv_work_t* req);

} // ns node_fontnik

#endif // NODE_FONTNIK_GLYPHS_HPP
