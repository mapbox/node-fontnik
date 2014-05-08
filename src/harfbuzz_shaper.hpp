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

#include "text_line.hpp"
#include "itemizer.hpp"
#include "font_engine_freetype.hpp"
#include "glyph_info.hpp"

// stl
#include <list>
#include <map>

// icu
#include <unicode/unistr.h>

// harfbuzz
#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>
#include <harfbuzz/hb-icu.h>

namespace fontserver {

class harfbuzz_shaper {
public:
    harfbuzz_shaper();
    ~harfbuzz_shaper();

    static void shape_text(text_line &line,
                           text_itemizer &itemizer,
                           std::map<unsigned, double> &width_map,
                           face_set_ptr &face_set,
                           // face_manager_freetype &font_manager,
                           double scale_factor);
};

}
