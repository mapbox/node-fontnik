#pragma once

#include "glyphs.pb.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#include <node.h>
#pragma GCC diagnostic pop

#include <nan.h>

// freetype2
extern "C" {
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H
}

namespace node_fontnik {

struct glyph_info {
   glyph_info()
       : glyph_index(0),
         codepoint(0),
         width(0),
         height(0),
         advance(0.0),
         left(0),
         top(0),
         bitmap("") {}

   uint32_t glyph_index;
   uint32_t codepoint;

   uint32_t width;
   uint32_t height;

   double advance;

   int32_t left;
   int32_t top;

   std::string bitmap;
};

NAN_METHOD(Load);
void LoadAsync(uv_work_t* req);
void AfterLoad(uv_work_t* req);

NAN_METHOD(Range);
void RangeAsync(uv_work_t* req);
void AfterRange(uv_work_t* req);

NAN_METHOD(Shape);
void ShapeAsync(uv_work_t* req);
void AfterShape(uv_work_t* req);

void RenderSDF(FT_Face ft_face,
               glyph_info &glyph);

const static int char_size = 24;
const static float scale_factor = 1.0;

const static int buffer_size = 3;

// dpp = 256 / radius_size
const static int radius_size = 8;

// shift = cutoff_size * 256
const static float cutoff_size = 0.25;

const static int granularity = 1;
const static float offset_size = 0.5;

} // ns node_fontnik
