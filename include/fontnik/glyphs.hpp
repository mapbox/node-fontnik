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

#ifndef FONTNIK_GLYPHS_HPP
#define FONTNIK_GLYPHS_HPP

#include <node.h>
#include "glyphs.pb.h"

namespace fontnik
{

class Glyphs
{

public:
    Glyphs();
    Glyphs(const char *data, size_t length);
    ~Glyphs();

    std::string Serialize();
    void Range(std::string fontstack,
               std::string range,
               std::vector<std::uint32_t> chars);

    static std::vector<int> Codepoints(std::string fontstack);

    static std::string Trim(std::string str, std::string whitespace);

public:
    llmr::glyphs::glyphs glyphs;

};

} // ns fontnik

#endif // FONTNIK_GLYPHS_HPP
