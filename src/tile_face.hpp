#pragma once

// stl
#include <string>

// freetype2
extern "C"
{
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_ERRORS_H
// #include FT_STROKER_H
}

class TileFace {
public:
    TileFace() {}
    ~TileFace() {}

    /*
    inline void add_glyph(uint32_t glyph_id) {
        glyphs.insert(glyph_id);
    }
    */

    void set_face(FT_Face face) {
        face_ = face;
        family = face_->family_name;
        style = face_->style_name;
    }

    std::string name() {
        return family + ' ' + style;
    }

    std::string family;
    std::string style;
    // std::set<uint32_t> glyphs;

private:
    FT_Face face_;
};
