/*****************************************************************************
 *
 * This file is part of Mapnik (c++ mapping toolkit)
 *
 * Copyright (C) 2013 Artem Pavlenko
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *****************************************************************************/

#pragma once

#include "glyph_info.hpp"
#include "guarded_map.hpp"

// stl
#include <memory>
#include <string>

// freetype2
extern "C"
{
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
}

namespace fontserver {

typedef guarded_map<uint32_t, glyph_info> glyph_cache_type;
typedef std::shared_ptr<glyph_cache_type> glyph_cache_ptr;

class font_face {
public:
    typedef glyph_cache_type::const_iterator iterator;

    font_face(FT_Face face);
    font_face(FT_Face face, glyph_cache_ptr glyphs);
    ~font_face();

    std::string family_name() const {
        return std::string(face_->family_name);
    }

    std::string style_name() const {
        return std::string(face_->style_name);
    }

    FT_Face get_face() const {
        return face_;
    }

    double get_char_height() const;
    bool set_character_sizes(double size);
    void glyph_dimensions(glyph_info &glyph) const;

    unsigned size() const { return glyphs_->size(); }
    iterator begin() { return glyphs_->cbegin(); }
    iterator end() { return glyphs_->cend(); }
private:
    FT_Face face_;
    glyph_cache_ptr glyphs_;
    mutable double char_height_;
};

typedef std::shared_ptr<font_face> face_ptr;

inline bool operator==(face_ptr const& lhs, face_ptr const& rhs) {
    return lhs->get_face() == rhs->get_face();
}

}
