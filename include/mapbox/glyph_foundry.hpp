#ifndef SDF_GLYPH_FOUNDRY_HPP
#define SDF_GLYPH_FOUNDRY_HPP

#include <string>

// freetype2
extern "C"
{
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H
}


namespace sdf_glyph_foundry
{

    struct glyph_info;
    void RenderSDF(glyph_info &glyph,
                   int size,
                   int buffer,
                   float cutoff,
                   FT_Face ft_face);

    struct glyph_info
    {
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

} // ns sdf_glyph_foundry

#endif // SDF_GLYPH_FOUNDRY_HPP
