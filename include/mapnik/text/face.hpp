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
#ifndef MAPNIK_FACE_HPP
#define MAPNIK_FACE_HPP

//mapnik
#include <mapnik/text/glyph_info.hpp>
#include <mapnik/config.hpp>
#include <mapnik/noncopyable.hpp>

// fontserver
#include <fontserver/guarded_map.hpp>

// freetype2
extern "C"
{
#include <ft2build.h>
#include FT_FREETYPE_H
}

//stl
#include <memory>
#include <string>
#include <vector>

namespace mapnik
{

typedef guarded_map<uint32_t, glyph_info> glyph_cache_type;
typedef std::shared_ptr<glyph_cache_type> glyph_cache_ptr;

class face : mapnik::noncopyable
{
public:
    face(FT_Face ft_face);
    face(FT_Face ft_face, glyph_cache_ptr glyphs);

    std::string family_name() const
    {
        return std::string(ft_face_->family_name);
    }

    std::string style_name() const
    {
        return std::string(ft_face_->style_name);
    }

    FT_Face get_face() const
    {
        return ft_face_;
    }

    double get_char_height() const;

    bool set_character_sizes(double size);

    void glyph_dimensions(glyph_info &glyph) const;

    ~face();

private:
    FT_Face ft_face_;
    mutable glyph_cache_ptr glyphs_;
    mutable double char_height_;
};
typedef std::shared_ptr<face> face_ptr;

inline bool operator==(face_ptr const& lhs, face_ptr const& rhs) {
    return lhs->get_face() == rhs->get_face();
}

class MAPNIK_DECL face_set : private mapnik::noncopyable
{
public:
    typedef std::vector<face_ptr>::const_iterator iterator;
    face_set(void) : faces_(){}

    void add(face_ptr face);
    void set_character_sizes(double size);

    unsigned size() const { return faces_.size(); }
    iterator begin() { return faces_.cbegin(); }
    iterator end() { return faces_.cend(); }
private:
    std::vector<face_ptr> faces_;
};
typedef std::shared_ptr<face_set> face_set_ptr;

} //ns mapnik

#endif // FACE_HPP
