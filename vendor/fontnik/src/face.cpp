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

// fontnik
#include <fontnik/face.hpp>

// agg
#include "agg_curves.h"

namespace fontnik
{

struct User {
    Rings rings;
    Points ring;
};

Face::Face(FT_Face ft_face)
    : ft_face_(ft_face),
      char_height_(0.0)
{
}

Face::~Face()
{
    FT_Done_Face(ft_face_);
}

bool Face::set_character_sizes(double size)
{
    char_height_ = 0.0;
    return !FT_Set_Char_Size(ft_face_,0,(FT_F26Dot6)(size * (1<<6)),0,0);
}

bool Face::set_unscaled_character_sizes()
{
    char_height_ = 0.0;
    return !FT_Set_Char_Size(ft_face_,0,ft_face_->units_per_EM,0,0);
}

void CloseRing(Points &ring)
{
    const Point &first = ring.front();
    const Point &last = ring.back();

    if (first.get<0>() != last.get<0>() ||
        first.get<1>() != last.get<1>())
    {
        ring.push_back(first);
    }
}

int MoveTo(const FT_Vector *to, void *ptr)
{
    User *user = (User*)ptr;
    if (!user->ring.empty()) {
        CloseRing(user->ring);
        user->rings.push_back(user->ring);
        user->ring.clear();
    }
    user->ring.push_back(Point { float(to->x) / 64, float(to->y) / 64 });
    return 0;
}

int LineTo(const FT_Vector *to, void *ptr)
{
    User *user = (User*)ptr;
    user->ring.push_back(Point { float(to->x) / 64, float(to->y) / 64 });
    return 0;
}

int ConicTo(const FT_Vector *control,
            const FT_Vector *to,
            void *ptr)
{
    User *user = (User*)ptr;

    Point prev = user->ring.back();

    // pop off last point, duplicate of first point in bezier curve
    user->ring.pop_back();

    agg::curve3_div curve(prev.get<0>(), prev.get<1>(),
                          float(control->x) / 64, float(control->y) / 64,
                          float(to->x) / 64, float(to->y) / 64);

    curve.rewind(0);
    double x, y;
    unsigned cmd;

    while (agg::path_cmd_stop != (cmd = curve.vertex(&x, &y))) {
        user->ring.push_back(Point {x, y});
    }

    return 0;
}

int CubicTo(const FT_Vector *c1,
            const FT_Vector *c2,
            const FT_Vector *to,
            void *ptr)
{
    User *user = (User*)ptr;

    Point prev = user->ring.back();

    // pop off last point, duplicate of first point in bezier curve
    user->ring.pop_back();

    agg::curve4_div curve(prev.get<0>(), prev.get<1>(),
                          float(c1->x) / 64, float(c1->y) / 64,
                          float(c2->x) / 64, float(c2->y) / 64,
                          float(to->x) / 64, float(to->y) / 64);

    curve.rewind(0);
    double x, y;
    unsigned cmd;

    while (agg::path_cmd_stop != (cmd = curve.vertex(&x, &y))) {
        user->ring.push_back(Point {x, y});
    }

    return 0;
}

// point in polygon ray casting algorithm
bool PolyContainsPoint(const Rings &rings, const Point &p)
{
    bool c = false;

    for (const Points &ring : rings) {
        auto p1 = ring.begin();
        auto p2 = p1 + 1;

        for (; p2 != ring.end(); p1++, p2++) {
            if (((p1->get<1>() > p.get<1>()) != (p2->get<1>() > p.get<1>())) && (p.get<0>() < (p2->get<0>() - p1->get<0>()) * (p.get<1>() - p1->get<1>()) / (p2->get<1>() - p1->get<1>()) + p1->get<0>())) {
                c = !c;
            }
        }
    }

    return c;
}

double SquaredDistance(const Point &v, const Point &w)
{
    const double a = v.get<0>() - w.get<0>();
    const double b = v.get<1>() - w.get<1>();
    return a * a + b * b;
}

Point ProjectPointOnLineSegment(const Point &p,
                                const Point &v,
                                const Point &w)
{
  const double l2 = SquaredDistance(v, w);
  if (l2 == 0) return v;

  const double t = ((p.get<0>() - v.get<0>()) * (w.get<0>() - v.get<0>()) + (p.get<1>() - v.get<1>()) * (w.get<1>() - v.get<1>())) / l2;
  if (t < 0) return v;
  if (t > 1) return w;

  return Point {
      v.get<0>() + t * (w.get<0>() - v.get<0>()),
      v.get<1>() + t * (w.get<1>() - v.get<1>())
  };
}

double SquaredDistanceToLineSegment(const Point &p,
                                    const Point &v,
                                    const Point &w)
{
    const Point s = ProjectPointOnLineSegment(p, v, w);
    return SquaredDistance(p, s);
}

double MinDistanceToLineSegment(const Tree &tree,
                                const Point &p,
                                int radius)
{
    const int squared_radius = radius * radius;

    std::vector<SegmentValue> results;
    tree.query(bgi::intersects(
        Box{
            Point{p.get<0>() - radius, p.get<1>() - radius},
            Point{p.get<0>() + radius, p.get<1>() + radius}
        }),
        std::back_inserter(results));

    double sqaured_distance = std::numeric_limits<double>::infinity();

    for (const auto &value : results) {
        const SegmentPair &segment = value.second;
        const double dist = SquaredDistanceToLineSegment(p,
                                                         segment.first, 
                                                         segment.second);
        if (dist < sqaured_distance && dist < squared_radius) {
            sqaured_distance = dist;
        }
    }

    return std::sqrt(sqaured_distance);
}

void Face::RenderSDF(mapnik::glyph_info &glyph,
                     int size,
                     int buffer,
                     float cutoff) const
{
    // Check if char is already in cache
    auto const& itr = glyph_info_cache_.find(glyph.glyph_index);
    if (itr != glyph_info_cache_.cend()) {
        glyph = itr->second;
        return;
    }

    if (FT_Load_Glyph (ft_face_, glyph.glyph_index, FT_LOAD_NO_HINTING)) {
        return;
    }

    FT_Glyph ft_glyph;
    if (FT_Get_Glyph(ft_face_->glyph, &ft_glyph)) return;

    int advance = ft_face_->glyph->metrics.horiAdvance / 64;
    int ascender = ft_face_->size->metrics.ascender / 64;
    int descender = ft_face_->size->metrics.descender / 64;

    glyph.line_height = ft_face_->size->metrics.height;
    glyph.advance = advance;
    glyph.ascender = ascender;
    glyph.descender = descender;

    FT_Outline_Funcs func_interface = {
        .move_to = &MoveTo,
        .line_to = &LineTo,
        .conic_to = &ConicTo,
        .cubic_to = &CubicTo,
        .shift = 0,
        .delta = 0
    };

    User user;

    // Decompose outline into bezier curves and line segments
    FT_Outline outline = ((FT_OutlineGlyph)ft_glyph)->outline;
    if (FT_Outline_Decompose(&outline, &func_interface, &user)) return;

    if (!user.ring.empty()) {
        CloseRing(user.ring);
        user.rings.push_back(user.ring);
    }

    if (user.rings.empty()) return;

    // Calculate the real glyph bbox.
    double bbox_xmin = std::numeric_limits<double>::infinity(),
           bbox_ymin = std::numeric_limits<double>::infinity();

    double bbox_xmax = -std::numeric_limits<double>::infinity(),
           bbox_ymax = -std::numeric_limits<double>::infinity();

    for (const Points &ring : user.rings) {
        for (const Point &point : ring) {
            if (point.get<0>() > bbox_xmax) bbox_xmax = point.get<0>();
            if (point.get<0>() < bbox_xmin) bbox_xmin = point.get<0>();
            if (point.get<1>() > bbox_ymax) bbox_ymax = point.get<1>();
            if (point.get<1>() < bbox_ymin) bbox_ymin = point.get<1>();
        }
    }

    bbox_xmin = std::round(bbox_xmin);
    bbox_ymin = std::round(bbox_ymin);
    bbox_xmax = std::round(bbox_xmax);
    bbox_ymax = std::round(bbox_ymax);

    // Offset so that glyph outlines are in the bounding box.
    for (Points &ring : user.rings) {
        for (Point &point : ring) {
            point.set<0>(point.get<0>() + -bbox_xmin + buffer);
            point.set<1>(point.get<1>() + -bbox_ymin + buffer);
        }
    }

    if (bbox_xmax - bbox_xmin == 0 || bbox_ymax - bbox_ymin == 0) return;

    glyph.left = bbox_xmin;
    glyph.top = bbox_ymax;
    glyph.width = bbox_xmax - bbox_xmin;
    glyph.height = bbox_ymax - bbox_ymin;

    Tree tree;
    float offset = 0.5;
    int radius = 8;

    for (const Points &ring : user.rings) {
        auto p1 = ring.begin();
        auto p2 = p1 + 1;

        for (; p2 != ring.end(); p1++, p2++) {
            const int segment_x1 = std::min(p1->get<0>(), p2->get<0>());
            const int segment_x2 = std::max(p1->get<0>(), p2->get<0>());
            const int segment_y1 = std::min(p1->get<1>(), p2->get<1>());
            const int segment_y2 = std::max(p1->get<1>(), p2->get<1>());

            tree.insert(SegmentValue {
                Box {
                    Point {segment_x1, segment_y1},
                    Point {segment_x2, segment_y2}
                },
                SegmentPair {
                    Point {p1->get<0>(), p1->get<1>()},
                    Point {p2->get<0>(), p2->get<1>()}
                }
            });
        }
    }

    // Loop over every pixel and determine the positive/negative distance to the outline.
    unsigned int buffered_width = glyph.width + 2 * buffer;
    unsigned int buffered_height = glyph.height + 2 * buffer;
    unsigned int bitmap_size = buffered_width * buffered_height;
    glyph.bitmap.resize(bitmap_size);

    for (unsigned int y = 0; y < buffered_height; y++) {
        for (unsigned int x = 0; x < buffered_width; x++) {
            unsigned int ypos = buffered_height - y - 1;
            unsigned int i = ypos * buffered_width + x;

            double d = MinDistanceToLineSegment(tree, Point {x + offset, y + offset}, radius) * (256 / radius);

            // Invert if point is inside.
            const bool inside = PolyContainsPoint(user.rings, Point { x + offset, y + offset });
            if (inside) {
                d = -d;
            }

            // Shift the 0 so that we can fit a few negative values
            // into our 8 bits.
            d += cutoff * 256;

            // Clamp to 0-255 to prevent overflows or underflows.
            int n = d > 255 ? 255 : d;
            n = n < 0 ? 0 : n;

            glyph.bitmap[i] = static_cast<char>(255 - n);
        }
    }

    FT_Done_Glyph(ft_glyph);

    glyph_info_cache_.emplace(glyph.glyph_index, glyph);
}

} // ns fontnik
