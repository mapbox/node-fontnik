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
#ifndef MAPNIK_GLYPH_INFO_HPP
#define MAPNIK_GLYPH_INFO_HPP

//mapnik
#include <mapnik/pixel_position.hpp>

// stl
#include <memory>

namespace mapnik
{

class font_face;
typedef std::shared_ptr<font_face> face_ptr;

typedef uint32_t glyph_index_t;

struct glyph_info
{
    glyph_info()
        : glyph_index(0),
          face(nullptr),
          bitmap(""),
          char_index(0),
          width(0),
          height(0),
          left(0),
          top(0),
          advance(0.0),
          line_height(0.0),
          ascender(0.0),
          descender(0.0),
          offset() {}
    glyph_index_t glyph_index;
    face_ptr face;
    std::string bitmap;
    // Position in the string of all characters i.e. before itemizing
    unsigned char_index;
    uint32_t width;
    uint32_t height;
    int32_t left;
    int32_t top;
    double advance;
    // Line height returned by FreeType, includes normal font
    // line spacing, but not additional user defined spacing
    double line_height;
    // Ascender and descender from baseline returned by FreeType
    double ascender;
    double descender;
    pixel_position offset;
};

inline bool operator<(glyph_info const& lhs, glyph_info const& rhs) {
    return lhs.glyph_index < rhs.glyph_index;
}

} //ns mapnik

#endif // GLYPH_INFO_HPP
