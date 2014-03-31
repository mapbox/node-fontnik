#pragma once

#include "face.hpp"
#include "glyph_info.hpp"

// freetype2
extern "C"
{
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
}

struct HarfBuzzGlyphInfo {
    FT_Bitmap bitmap;
    FT_Int top;
    FT_Int left;
    FT_Glyph_Metrics metrics;
    FT_Face face;
};

class HarfBuzzGlyph {
public:
    HarfBuzzGlyph(FT_Glyph glyph,
                  FT_Glyph_Metrics metrics,
                  FT_Face face);
    ~HarfBuzzGlyph();

    HarfBuzzGlyphInfo get();
private:
    FT_Glyph glyph;
    FT_Glyph_Metrics metrics;
    FT_Face face;
};
