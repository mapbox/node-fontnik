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

#include "text_item.hpp"

// stl
#include <string>
#include <list>
#include <vector>

// ICU
#include <unicode/unistr.h>

namespace fontserver {

/** This class splits text into parts which all have the same
 * - direction (LTR, RTL)
 * - format
 * - script (http://en.wikipedia.org/wiki/Scripts_in_Unicode)
 **/
class text_itemizer {
public:
    text_itemizer();

    void add_text(UnicodeString str, char_properties_ptr format);
    std::list<text_item> const& itemize(unsigned start = 0, unsigned end = 0);
    void clear();
    UnicodeString const& text() const { return text_; }

    /** Returns the start and end position of a certain line.
     *
     * Only forced line breaks with \n characters are handled here.
     */
    std::pair<unsigned, unsigned> line(unsigned i) const;

    unsigned num_lines() const;

private:
    template<typename T>
    struct run {
        run(T data, unsigned start, unsigned end)
            : data(data),
              start(start),
              end(end) {}
        T data;
        unsigned start;
        unsigned end;
    };

    typedef run<char_properties_ptr> format_run_t;
    typedef run<UBiDiDirection> direction_run_t;
    typedef run<UScriptCode> script_run_t;
    typedef std::list<format_run_t> format_run_list;
    typedef std::list<script_run_t> script_run_list;
    typedef std::list<direction_run_t> direction_run_list;

    UnicodeString text_;

    // Format runs are always sorted by char index
    format_run_list format_runs_;

    // Directions runs are always in visual order! This is different from
    // format and script runs!
    direction_run_list direction_runs_;

    // Script runs are always sorted by char index
    script_run_list script_runs_;

    void itemize_direction(unsigned start, unsigned end);
    void itemize_script();
    void create_item_list();

    std::list<text_item> output_;

    template <typename T> typename T::const_iterator find_run(T const& list, unsigned position);

    // Positions of \n characters
    std::vector<unsigned> forced_line_breaks_;
};

}
