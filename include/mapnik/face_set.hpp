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

#include "face.hpp"

// stl
#include <memory>
#include <vector>

// freetype2
extern "C"
{
#include <ft2build.h>
#include FT_FREETYPE_H
}

namespace mapnik {

class face_set {
public:
    typedef std::vector<face_ptr>::const_iterator iterator;

    face_set(void) : faces_() {}

    void add(face_ptr face);
    void set_character_sizes(double size);

    unsigned size() const { return faces_.size(); }
    iterator begin() { return faces_.cbegin(); }
    iterator end() { return faces_.cend(); }
private:
    std::vector<face_ptr> faces_;
};

typedef std::shared_ptr<face_set> face_set_ptr;

}
