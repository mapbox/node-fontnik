#include "harfbuzz_glyph.hpp"

HarfBuzzGlyph::HarfBuzzGlyph(FT_Glyph glyph,
                             FT_Glyph_Metrics metrics,
                             FT_Face face)
    : glyph(glyph),
    metrics(metrics),
    face(face) {};

HarfBuzzGlyph::~HarfBuzzGlyph() {
    FT_Done_Glyph(glyph);
    FT_Done_Face(face);
};

HarfBuzzGlyphInfo HarfBuzzGlyph::get() {
    HarfBuzzGlyphInfo info;
    FT_BitmapGlyph glyph_bitmap;
    FT_Error error;

    // error = FT_Load_Glyph(face, glyph_index, FT_LOAD_NORMAL);
    // error = FT_Get_Glyph(face->glyph, &glyph);

    // info.metrics = face->glyph->metrics;
    // info.face = face;

    if (glyph->format != FT_GLYPH_FORMAT_BITMAP) {
        error = FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, 0, 1);
    }

    glyph_bitmap = (FT_BitmapGlyph)glyph;
    info.bitmap = glyph_bitmap->bitmap;
    info.top = glyph_bitmap->top;
    info.left = glyph_bitmap->left;

    return info;
}
