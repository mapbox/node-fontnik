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

#include <cmath>

// mapnik
#include <mapnik/text/face.hpp>
#include <mapnik/debug.hpp>

// freetype-gl
#include <freetype-gl/distmap.h>

// agg
#include <agg/agg_curves.h>

// boost
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/index/rtree.hpp>

extern "C"
{
#include FT_GLYPH_H
}

namespace mapnik
{

namespace bg = boost::geometry;
namespace bgm = bg::model;
namespace bgi = bg::index;
typedef bgm::point<float, 2, bg::cs::cartesian> Point;
typedef bgm::box<Point> Box;
typedef bgi::rtree<Box, bgi::rstar<16>> Tree;

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
    glyph.descender = face_->size->metrics.descender / 64.0;

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

void font_face::close_ring(Points *ring) {
    FT_Vector first = ring->front();
    FT_Vector last = ring->back();

    if (first.x != last.x || first.y != last.y) {
        ring->push_back(first);
    }
}

int font_face::move_to(const FT_Vector *to, void *ptr) {
    User *user = (User*)ptr;
    if (!user->ring.empty()) {
        close_ring(&(user->ring));
        user->rings.push_back(user->ring);
        user->ring.clear();
    }
    user->ring.emplace_back(*to);
    return 0;
}

int font_face::line_to(const FT_Vector *to, void *ptr) {
    User *user = (User*)ptr;
    user->ring.emplace_back(*to);
    return 0;
}

int font_face::conic_to(const FT_Vector *control,
                        const FT_Vector *to,
                        void *ptr) {
    User *user = (User*)ptr;

    FT_Vector prev = user->ring.back();

    // pop off last point, duplicate of first point in bezier curve
    user->ring.pop_back();

    agg::curve3_div curve(prev.x, prev.y,
                          control->x, control->y,
                          to->x, to->y);

    curve.rewind(0);
    double x, y;
    unsigned cmd;
    while (agg::path_cmd_stop != (cmd = curve.vertex(&x, &y))) {
        FT_Vector point {
            .x = static_cast<FT_Pos>(x),
            .y = static_cast<FT_Pos>(y)
        };
        user->ring.push_back(point);
    }

    return 0;
}

int font_face::cubic_to(const FT_Vector *c1,
                        const FT_Vector *c2,
                        const FT_Vector *to,
                        void *ptr) {
    User *user = (User*)ptr;

    FT_Vector prev = user->ring.back();

    // pop off last point, duplicate of first point in bezier curve
    user->ring.pop_back();

    agg::curve4_div curve(prev.x, prev.y,
                          c1->x, c1->y,
                          c2->x, c2->y,
                          to->x, to->y);

    curve.rewind(0);
    double x, y;
    unsigned cmd;
    while (agg::path_cmd_stop != (cmd = curve.vertex(&x, &y))) {
        FT_Vector point {
            .x = static_cast<FT_Pos>(x),
            .y = static_cast<FT_Pos>(y)
        };
        user->ring.push_back(point);
    }

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

    /*
    float scale = face_->units_per_EM / size;
    int ascender = face_->ascender / scale;
    */

    if (FT_Load_Glyph (face_, glyph.glyph_index, FT_LOAD_NO_HINTING)) return;

    FT_Glyph ft_glyph;
    if (FT_Get_Glyph(face_->glyph, &ft_glyph)) return;

    FT_Outline_Funcs func_interface = {
        .move_to = &move_to,
        .line_to = &line_to,
        .conic_to = &conic_to,
        .cubic_to = &cubic_to,
        .shift = 0,
        .delta = 0
    };

    User user;

    // Decompose outline into bezier curves and line segments
    FT_Outline outline = ((FT_OutlineGlyph)ft_glyph)->outline;
    if (FT_Outline_Decompose(&outline, &func_interface, &user)) return;

    if (!user.ring.empty()) {
        close_ring(&(user.ring));
        user.rings.push_back(user.ring);
    }

    if (user.rings.empty()) return;

    // Calculate the real glyph bbox.
    double xMin = std::numeric_limits<double>::infinity(),
           yMin = std::numeric_limits<double>::infinity();

    double xMax = -std::numeric_limits<double>::infinity(),
           yMax = -std::numeric_limits<double>::infinity();

    for (auto ring : user.rings) {
        for (auto point : ring) {
            if (point.x > xMax) xMax = point.x;
            if (point.x < xMin) xMin = point.x;
            if (point.y > yMax) yMax = point.y;
            if (point.y < yMin) yMin = point.y;
        }
    }

    xMin = round(xMin);
    yMin = round(yMin);
    xMax = round(xMax);
    yMax = round(yMax);

    // Offset so that glyph outlines are in the bounding box.
    for (auto ring : user.rings) {
        for (auto point : ring) {
            point.x += -xMin + buffer;
            point.y += -yMin + buffer;
        }
    }

    if (xMax - xMin == 0 || yMax - yMin == 0) return;

    glyph.left = xMin;
    glyph.top = -yMin; // -yMin - ascender?
    glyph.width = xMax - xMin;
    glyph.height = yMax - yMin;

    glyph.line_height = face_->size->metrics.height;
    glyph.advance = face_->glyph->metrics.horiAdvance;
    glyph.ascender = face_->size->metrics.ascender;
    glyph.descender = face_->size->metrics.descender;

    glyph.bitmap.reserve(glyph.width * glyph.height);

    Tree tree;
    float offset = 0.5;
    int radius = 8;

    for (auto ring : user.rings) {
        auto next = ring.begin();
        next++;

        for (auto it = ring.begin(); next != ring.end(); it++, next++) {
            int xMin = std::min(it->x, next->x);
            int xMax = std::max(it->x, next->x);
            int yMin = std::min(it->y, next->y);
            int yMax = std::max(it->y, next->y);

            tree.insert(Box {
                Point {xMin, yMin},
                Point {xMax, yMax}
            });
        }
    }

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
