#pragma once

#include "font_face.hpp"

namespace fontserver {

class tile_face {
public:
    tile_face(face_ptr face) : face_(face) {
        family = face->family_name();
        style = face->style_name();
    }
    ~tile_face();

    inline void add_glyph(uint32_t glyph_index, glyph_info glyph) {
        glyphs.emplace(glyph_index, glyph);
    }

    std::string family;
    std::string style;
    std::map<uint32_t, glyph_info> glyphs;

private:
    face_ptr face_;
};

}
