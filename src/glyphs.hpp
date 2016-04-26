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

NAN_METHOD(Load);
void LoadAsync(uv_work_t* req);
void AfterLoad(uv_work_t* req);

NAN_METHOD(Range);
void RangeAsync(uv_work_t* req);
void AfterRange(uv_work_t* req);

NAN_METHOD(Shape);
void ShapeAsync(uv_work_t* req);
void AfterShape(uv_work_t* req);

struct glyph_info;
void RenderSDF(glyph_info &glyph,
               int size,
               int buffer,
               float cutoff,
               FT_Face ft_face);

struct glyph_info {
   glyph_info()
       : glyph_index(0),
         bitmap(""),
         char_index(0),
         left(0),
         top(0),
         width(0),
         height(0),
         advance(0.0),
         line_height(0.0),
         ascender(0.0),
         descender(0.0) {}
   unsigned glyph_index;
   std::string bitmap;

   // Position in the string of all characters i.e. before itemizing
   unsigned char_index;

   int32_t left;
   int32_t top;
   uint32_t width;
   uint32_t height;
   double advance;

   // Line height returned by FreeType, includes normal font
   // line spacing, but not additional user defined spacing
   double line_height;

   // Ascender and descender from baseline returned by FreeType
   double ascender;
   double descender;
};

NAN_METHOD(Load);
void LoadAsync(uv_work_t* req);
void AfterLoad(uv_work_t* req);

NAN_METHOD(Range);
void RangeAsync(uv_work_t* req);
void AfterRange(uv_work_t* req);

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
