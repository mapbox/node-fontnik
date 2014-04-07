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

#include "pixel_position.hpp"

// stl
#include <memory>

class font_face;
typedef std::shared_ptr<font_face> face_ptr;

struct glyph_info {
    glyph_info()
        : glyph_index(0),
          face(nullptr),
          char_index(0),
          width(0.0),
          x(0.0),
          y(0.0),
          ymin(0.0),
          ymax(0.0),
          line_height(0.0),
          offset() {}
    uint32_t glyph_index;
    face_ptr face;

    uint32_t id;
    std::string bitmap;

    // Position in the string of all characters i.e. before itemizing
    unsigned char_index;

    uint32_t width;
    uint32_t height;
    int32_t left;
    int32_t top;
    uint32_t advance;

    double x;
    double y;
    double ymin;
    double ymax;

    // Line height returned by freetype, includes normal font
    // line spacing, but not additional user defined spacing
    double line_height;
    pixel_position offset;
    // double height() const { return ymax-ymin; }
};
