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

#include "font_face.hpp"

// stl
#include <map>
#include <memory>
#include <string>
#include <vector>

// freetype2
extern "C"
{
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_ERRORS_H
// #include FT_STROKER_H
}

class font_face;
typedef std::shared_ptr<font_face> face_ptr;

class font_face_set {
public:
    typedef std::vector<face_ptr>::iterator iterator;
    font_face_set(void) : faces_() {}

    void add(face_ptr face);
    void set_character_sizes(double size);

    unsigned size() const { return faces_.size(); }
    iterator begin() { return faces_.begin(); }
    iterator end() { return faces_.end(); }
private:
    std::vector<face_ptr> faces_;

    static pthread_once_t init;
    static pthread_key_t map_key;
    static void init_map() {
        pthread_key_create(&map_key, delete_map);
    }
    static void delete_map(void *fontmap) {
        delete (font_face_set *)fontmap;
    }

public:
    static face_ptr face(FT_Face face) {
        // Get a thread-specific font/glyph map.
        pthread_once(&init, init_map);
        font_face_set *fontmap = (font_face_set *)pthread_getspecific(map_key);
        if (fontmap == NULL) {
            pthread_setspecific(map_key, fontmap = new font_face_set);
        }

        for (font_face_set::iterator itr = fontmap->begin(); itr != fontmap->end(); ++itr) {
            if (itr != fontmap->end()) {
                face_ptr const &font = std::make_shared<font_face>(face);
                fontmap->add(font);
                return font;
            } else if (itr->get()->get_face() == face) {
                return *itr;
            }
        }
    }
};

pthread_once_t font_face_set::init = PTHREAD_ONCE_INIT;
pthread_key_t font_face_set::map_key = 0;

typedef std::shared_ptr<font_face_set> face_set_ptr;
