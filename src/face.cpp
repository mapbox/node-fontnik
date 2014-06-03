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

// mapnik
#include <mapnik/text/face.hpp>

#include "util/distmap.h"

namespace mapnik {

face::face(FT_Face ft_face)
    : ft_face_(ft_face),
      glyphs_(std::make_shared<glyph_cache_type>()),
      char_height_(0.0) {}

face::face(FT_Face ft_face, glyph_cache_ptr glyphs)
    : ft_face_(ft_face),
      glyphs_(glyphs),
      char_height_(0.0) {}

face::~face() {}

double face::get_char_height() const {
    if (char_height_ != 0.0) return char_height_;

    glyph_info tmp;
    tmp.glyph_index = FT_Get_Char_Index(ft_face_, 'X');
    glyph_dimensions(tmp);
    char_height_ = tmp.height;

    return char_height_;
}

bool face::set_character_sizes(double size) {
    char_height_ = 0.0;
    return !FT_Set_Char_Size(ft_face_, 0, (FT_F26Dot6)(size * (1<<6)), 0, 0);
}

void face::glyph_dimensions(glyph_info &glyph) const {
    // Check if char is already in cache.
    iterator itr;
    itr = glyphs_.get()->find(glyph.glyph_index);
    if (itr != glyphs_.get()->cend()) {
        glyph = itr->second;
        return;
    }

    // TODO: remove hardcoded buffer size?
    int buffer = 3;

    if (FT_Load_Glyph(ft_face_, glyph.glyph_index, FT_LOAD_NO_HINTING)) return;

    FT_Glyph ft_glyph;
    if (FT_Get_Glyph(ft_face_->glyph, &ft_glyph)) return;

    FT_BBox bbox;
    FT_Glyph_Get_CBox(ft_glyph, FT_GLYPH_BBOX_PIXELS, &bbox);

    glyph.ymin = bbox.yMin;
    glyph.ymax = bbox.yMax;

    glyph.line_height = ft_face_->size->metrics.height / 64.0;
    glyph.advance = ft_face_->glyph->metrics.horiAdvance / 64.0;

    glyph.ascender = ft_face_->size->metrics.ascender / 64.0;
    glyph.descender = ft_face_->size->metrics.ascender / 64.0;

    FT_Glyph_To_Bitmap(&ft_glyph, FT_RENDER_MODE_NORMAL, 0, 1);

    int width = ((FT_BitmapGlyph)ft_glyph)->bitmap.width;
    int height = ((FT_BitmapGlyph)ft_glyph)->bitmap.rows;

    glyph.width = width;
    glyph.height = height;
    glyph.left = ((FT_BitmapGlyph)ft_glyph)->left;
    glyph.top = ((FT_BitmapGlyph)ft_glyph)->top;

    // Create a signed distance field (SDF) for the glyph bitmap.
    if (width > 0) {
        unsigned int buffered_width = width + 2 * buffer;
        unsigned int buffered_height = height + 2 * buffer;

        unsigned char *distance = make_distance_map((unsigned char *)((FT_BitmapGlyph)ft_glyph)->bitmap.buffer, width, height, buffer);

        glyph.bitmap.resize(buffered_width * buffered_height);
        for (unsigned int y = 0; y < buffered_height; y++) {
            memcpy((unsigned char *)glyph.bitmap.data() + buffered_width * y, distance + y * distmap_size, buffered_width);
        }
        free(distance);
    }

    FT_Done_Glyph(ft_glyph);

    glyphs_.get()->emplace(glyph.glyph_index, glyph);
}

}
