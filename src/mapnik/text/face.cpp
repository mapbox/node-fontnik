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
#include <mapnik/debug.hpp>

// freetype-gl
#include <freetype-gl/distmap.h>

extern "C"
{
#include FT_GLYPH_H
}

namespace mapnik
{

font_face::font_face(FT_Face face)
    : face_(face),
      glyphs_(std::make_shared<glyph_cache_type>()),
      char_height_(0.0)
{
}

font_face::font_face(FT_Face face, glyph_cache_ptr glyphs)
    : face_(face),
      glyphs_(glyphs),
      char_height_(0.0) {}

double font_face::get_char_height() const
{
    if (char_height_ != 0.0) return char_height_;
    glyph_info tmp;
    tmp.glyph_index = FT_Get_Char_Index(face_, 'X');
    glyph_dimensions(tmp);
    char_height_ = tmp.height;
    return char_height_;
}

bool font_face::set_character_sizes(double size)
{
    char_height_ = 0.0;
    return !FT_Set_Char_Size(face_,0,(FT_F26Dot6)(size * (1<<6)),0,0);
}

void font_face::glyph_dimensions(glyph_info & glyph) const
{
    //Check if char is already in cache
    auto const& itr = glyphs_->find(glyph.glyph_index);
    if (itr != glyphs_.get()->cend()) {
        glyph = itr->second;
        return;
    }

    // TODO: remove hardcoded buffer size?
    int buffer = 3;

    if (FT_Load_Glyph (face_, glyph.glyph_index, FT_LOAD_NO_HINTING)) return;

    FT_Glyph image;
    if (FT_Get_Glyph(face_->glyph, &image)) return;

    glyph.line_height = face_->size->metrics.height / 64.0;
    glyph.advance = face_->glyph->metrics.horiAdvance / 64.0;
    glyph.ascender = face_->size->metrics.ascender / 64.0;
    glyph.descender = face_->size->metrics.ascender / 64.0;

    FT_Glyph_To_Bitmap(&image, FT_RENDER_MODE_NORMAL, 0, 1);

    int width = ((FT_BitmapGlyph)image)->bitmap.width;
    int height = ((FT_BitmapGlyph)image)->bitmap.rows;

    glyph.width = width;
    glyph.height = height;
    glyph.left = ((FT_BitmapGlyph)image)->left;
    glyph.top = ((FT_BitmapGlyph)image)->top;

    // TODO: move this out?
    // Create a signed distance field (SDF) for the glyph bitmap.
    if (width > 0) {
        unsigned int buffered_width = width + 2 * buffer;
        unsigned int buffered_height = height + 2 * buffer;

        unsigned char *distance = make_distance_map((unsigned char *)((FT_BitmapGlyph)image)->bitmap.buffer, width, height, buffer);

        glyph.bitmap.resize(buffered_width * buffered_height);
        for (unsigned int y = 0; y < buffered_height; y++) {
            memcpy((unsigned char *)glyph.bitmap.data() + buffered_width * y, distance + y * distmap_size, buffered_width);
        }
        free(distance);
    }

    FT_Done_Glyph(image);

    glyphs_.get()->emplace(glyph.glyph_index, glyph);
}

typedef std::pair<uint32_t, uint32_t> Point;
typedef std::vector<Point> Points;
typedef std::vector<Points> Rings;

struct User {
    Rings rings;
    Points ring;
} User_;

void close_ring(Points ring) {
    Point first = ring.front();
    Point last = ring.back();
    // if (first !== last) { // would this be preferable?
    if (first.first != last.first || first.second != last.second) {
        ring.push_back(Point(first.first, first.second));
    }
}

int move_to(const FT_Vector *to, User *user) {
    if (!user->ring.empty()) {
        close_ring(user->ring);
        user->rings.push_back(user->ring);
    }
    Points ring = { std::make_pair(to->x, to->y) };
    user->ring = ring;
    return 0;
}

int line_to(const FT_Vector *to, User *user) {
    Point point = std::make_pair(to->x, to->y);
    user->ring.push_back(point);
    return 0;
}

int conic_to(const FT_Vector *control,
             const FT_Vector *to,
             User *user) {
    Point point = user->ring.back();
    user->ring.pop_back();

    Points points;
    // curve3div

    // preallocate memory then concat
    user->ring.reserve(user->ring.size() + points.size());
    user->ring.insert(user->ring.end(), points.begin(), points.end());
    return 0;
}

int cubic_to(const FT_Vector *c1,
             const FT_Vector *c2,
             const FT_Vector *to,
             User *user) {
    // curve4div
    return 0;
}

void font_face::glyph_outlines(glyph_info &glyph,
                               int size,
                               int buffer,
                               float cutoff) const
{
    //Check if char is already in cache
    auto const& itr = glyphs_->find(glyph.glyph_index);
    if (itr != glyphs_.get()->cend()) {
        glyph = itr->second;
        return;
    }

    float scale = face_->units_per_EM / size;
    int ascender = face_->ascender / scale;

    if (FT_Load_Glyph (face_, glyph.glyph_index, FT_LOAD_NO_HINTING)) return;

    FT_Glyph ft_glyph;
    if (FT_Get_Glyph(face_->glyph, &ft_glyph)) return;

    FT_Outline_Funcs func_interface = {
        .move_to = *move_to,
        .line_to = *line_to,
        .conic_to = *conic_to,
        .cubic_to = *cubic_to,
        .shift = 0,
        .delta = 0
    };

    User user;

    // Decompose outline into bezier curves and line segments
    FT_Outline outline = ((FT_OutlineGlyph)ft_glyph)->outline;
    if (FT_Outline_Decompose(&outline, &func_interface, &user)) return;

    if (!user.ring.empty()) {
        close_ring(user.ring);
        user.rings.push_back(user.ring);
    }

    if (user.rings.empty()) return;

    FT_Done_Glyph(ft_glyph);

    glyphs_.get()->emplace(glyph.glyph_index, glyph);
}

font_face::~font_face()
{
    MAPNIK_LOG_DEBUG(font_face) <<
        "font_face: Clean up face \"" << family_name() <<
        " " << style_name() << "\"";

    FT_Done_Face(face_);
}

/******************************************************************************/

void font_face_set::add(face_ptr face)
{
    faces_.push_back(face);
}

void font_face_set::set_character_sizes(double size)
{
    for (face_ptr const& face : faces_)
    {
        face->set_character_sizes(size);
    }
}

}//ns mapnik
