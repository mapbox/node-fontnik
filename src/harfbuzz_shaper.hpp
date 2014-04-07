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
#include "font_engine_freetype.hpp"

// stl
#include <list>

// icu
#include <unicode/unistr.h>

// harfbuzz
#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>
#include <harfbuzz/hb-icu.h>

class HarfbuzzShaper {
public:
    HarfbuzzShaper();
    ~HarfbuzzShaper();

    std::vector<glyph_info> Shape(std::string &value,
                                  std::string &fontstack,
                                  std::map<unsigned,double> &width_map,
                                  face_manager_freetype &font_manager,
                                  double scale_factor);
private:
    std::vector<std::pair<std::string, face_ptr>> Split(const std::string &s, char delim, std::vector<std::pair<std::string, face_ptr>> &elems);
};
