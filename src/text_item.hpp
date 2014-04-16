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

#include "text_format.hpp"

// ICU
#include <unicode/uscript.h>
#include <unicode/ubidi.h>

namespace fontserver {

struct text_item {
    text_item()
        : start(0),
          end(0),
          script(),
          format(),
          rtl(UBIDI_LTR) {}

    // First char (UTF16 offset)
    unsigned start;

    // Char _after_ the last char (UTF16 offset)
    unsigned end;

    UScriptCode script;
    text_format_ptr format;
    UBiDiDirection rtl;
};

}
