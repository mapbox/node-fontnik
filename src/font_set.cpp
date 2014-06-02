/*****************************************************************************
 *
 * This file is part of Mapnik (c++ mapping toolkit)
 *
 * Copyright (C) 2011 Artem Pavlenko
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

// fontserver
#include <fontserver/font_set.hpp>

// stl
#include <sstream>

namespace fontserver {

font_set::font_set(std::string const& name)
    : name_(name) {}

font_set::font_set(font_set const& rhs)
    : name_(rhs.name_),
      face_names_(rhs.face_names_) {}

font_set::~font_set() {}

font_set& font_set::operator=(font_set const& other) {
    if (this == &other) return *this;

    name_ = other.name_;
    face_names_ = other.face_names_;

    return *this;
}

std::size_t font_set::size() const {
    return face_names_.size();
}

void font_set::add_face_name(std::string const& face_name) {
    face_names_.push_back(std::move(face_name));
}

void font_set::add_fontstack(std::string const& fontstack, char delim) {
    std::stringstream stream(fontstack);
    std::string face_name;

    // TODO: better to split on delim and font_names_.reserve() then add?
    while (std::getline(stream, face_name, delim)) {
        face_names_.push_back(std::move(font_set::trim(face_name)));
    }
}

void font_set::set_name(std::string const& name) {
    name_ = name;
}

std::string const& font_set::get_name() const {
    return name_;
}

std::vector<std::string> const& font_set::get_face_names() const {
    return face_names_;
}

std::string font_set::trim(std::string const& str,
                           std::string const& whitespace) {
    const auto strBegin = str.find_first_not_of(whitespace);

    if (strBegin == std::string::npos) {
        return "";
    }

    const auto strEnd = str.find_last_not_of(whitespace);
    const auto strRange = strEnd - strBegin + 1;

    return str.substr(strBegin, strRange);
}

}
