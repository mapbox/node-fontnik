#pragma once

// stl
#include <string>
#include <set>

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

    inline void add_glyph(uint32_t glyph_id) {
        glyphs.insert(glyph_id);
    }

    void set_face(FT_Face face) {
        face_ = face;
        family_name = face_->family_name;
        style_name = face_->style_name;
    }

    std::string name() {
        return family_name + ' ' + style_name;
    }

    std::string family_name;
    std::string style_name;
    std::set<uint32_t> glyphs;

private:
    FT_Face face_;
};
