#ifndef NODE_FONTNIK_GLYPHS_HPP
#define NODE_FONTNIK_GLYPHS_HPP

#include <nan.h>

namespace node_fontnik {

NAN_METHOD(Load);
void LoadAsync(uv_work_t* req);
void AfterLoad(uv_work_t* req);
NAN_METHOD(Range);
void RangeAsync(uv_work_t* req);
void AfterRange(uv_work_t* req);
NAN_METHOD(Composite);
void CompositeAsync(uv_work_t* req);
void AfterComposite(uv_work_t* req);

} // namespace node_fontnik

#endif // NODE_FONTNIK_GLYPHS_HPP
