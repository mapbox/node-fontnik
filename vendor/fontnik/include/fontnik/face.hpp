/*****************************************************************************
 *
 * Copyright (C) 2014 Mapbox
 *
 * This file is part of Fontnik.
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

#ifndef FONTNIK_FACE_HPP
#define FONTNIK_FACE_HPP

// mapnik
#include <mapnik/text/glyph_info.hpp>

// boost
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/index/rtree.hpp>

// stl
#include <unordered_map>

// freetype2
extern "C"
{
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H
}

namespace bg = boost::geometry;
namespace bgm = bg::model;
namespace bgi = bg::index;
typedef bgm::point<float, 2, bg::cs::cartesian> Point;
typedef bgm::box<Point> Box;
typedef std::vector<Point> Points;
typedef std::vector<Points> Rings;
typedef std::pair<Point, Point> SegmentPair;
typedef std::pair<Box, SegmentPair> SegmentValue;
typedef bgi::rtree<SegmentValue, bgi::rstar<16>> Tree;

namespace fontnik
{

class Face
{

public:
    using glyph_info_cache_type = std::unordered_map<mapnik::glyph_index_t, mapnik::glyph_info>;

    Face(FT_Face ft_face);
    ~Face();

    FT_Face get_face() const
    {
        return ft_face_;
    }

    bool set_character_sizes(double size);
    bool set_unscaled_character_sizes();

    void RenderSDF(mapnik::glyph_info &glyph,
                   int size,
                   int buffer,
                   float cutoff) const;

private:
    FT_Face ft_face_;
    mutable glyph_info_cache_type glyph_info_cache_;
    mutable double char_height_;
};
typedef std::shared_ptr<Face> face_ptr;

} // ns fontnik

#endif // FONTNIK_FACE_HPP
