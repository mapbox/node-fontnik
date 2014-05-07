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

#include "harfbuzz_shaper.hpp"
#include "text_format.hpp"
#include "font_face_set.hpp"
#include "font_set.hpp"

#include <iostream>

namespace fontserver {

harfbuzz_shaper::harfbuzz_shaper() {};

harfbuzz_shaper::~harfbuzz_shaper() {};

void harfbuzz_shaper::shape_text(text_line &line,
                                 text_itemizer &itemizer,
                                 std::map<unsigned, double> &width_map,
                                 face_set_ptr &face_set,
                                 // face_manager_freetype &font_manager,
                                 double scale_factor) {
    unsigned start = line.first_char();
    unsigned end = line.last_char();
    UnicodeString const& text = itemizer.text();
    size_t length = end - start;
    if (!length) return;

    // Preallocate memory based on estimated length.
    line.reserve(length);

    std::list<text_item> const& list = itemizer.itemize();

    auto hb_buffer_deleter = [](hb_buffer_t *buffer) {
        hb_buffer_destroy(buffer);
    };

    const std::unique_ptr<hb_buffer_t, decltype(hb_buffer_deleter)> buffer(hb_buffer_create(), hb_buffer_deleter);
    hb_buffer_set_unicode_funcs(buffer.get(), hb_icu_get_unicode_funcs());
    hb_buffer_pre_allocate(buffer.get(), length);

    for (auto const& text_item : list) {
        unsigned start = text_item.start;

        // TODO: can this face set be passed with the text_item
        // instead of being recreated each time?
        // face_set_ptr face_set = font_manager.get_face_set(text_item.format->fontset);

        double size = text_item.format->text_size * scale_factor;
        face_set->set_character_sizes(size);

        font_face_set::iterator face_itr = face_set->begin(), face_end = face_set->end();
        for (; face_itr != face_end; ++face_itr) {
            face_ptr const& face = *face_itr;

            hb_buffer_clear_contents(buffer.get());
            hb_buffer_add_utf16(buffer.get(), text.getBuffer(), text.length(), text_item.start, text_item.end - text_item.start);
            hb_buffer_set_direction(buffer.get(), (text_item.rtl == UBIDI_RTL) ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);
            hb_buffer_set_direction(buffer.get(), HB_DIRECTION_LTR);
            hb_buffer_set_script(buffer.get(), hb_icu_script_to_script(text_item.script));
            hb_font_t *font(hb_ft_font_create(face->get_face(), nullptr));
            hb_shape(font, buffer.get(), NULL, 0);
            hb_font_destroy(font);

            unsigned num_glyph_infos = hb_buffer_get_length(buffer.get());

            hb_glyph_info_t *glyph_infos = hb_buffer_get_glyph_infos(buffer.get(), nullptr);
            hb_glyph_position_t *positions = hb_buffer_get_glyph_positions(buffer.get(), nullptr);

            // Check if all glyphs are valid.
            bool font_has_all_glyphs = true;
            for (unsigned i = 0; i < num_glyph_infos; ++i) {
                if (!glyph_infos[i].codepoint) {
                    std::cout << face->family_name() << ' ' <<
                        face->style_name() << " is missing glyph" <<
                        glyph_infos[i].cluster;
                    font_has_all_glyphs = false;
                    break;
                }
            }

            // TODO: Is there a better way to check for more faces in face_set?
            if (!font_has_all_glyphs && face_itr + 1 != face_end) {
                // Try next font in fontset.
                continue;
            }

            double current_line_length = 0;
            for (unsigned i = 0; i < num_glyph_infos; ++i) {
                glyph_info tmp;
                tmp.glyph_index = glyph_infos[i].codepoint;

                // font_face glyphs_ cache will overwrite tmp if
                // codepoint is found in cache, so call this first
                face->glyph_dimensions(tmp);

                tmp.char_index = glyph_infos[i].cluster;
                tmp.face = face;
                tmp.format = text_item.format;

                // Overwrite default advance with better value
                // from HarfBuzz.
                tmp.advance = positions[i].x_advance >> 6;

                tmp.offset.set(positions[i].x_offset / 64.0,
                               positions[i].y_offset / 64.0);

                // TODO: character_spacing (clusters)
                std::map<unsigned, double>::const_iterator width_itr = width_map.find(tmp.char_index);
                if (width_itr == width_map.end()) {
                    width_map[tmp.char_index] = current_line_length;
                }
                current_line_length += tmp.advance;

                line.add_glyph(tmp, scale_factor);
            }

            line.update_max_char_height(face->get_char_height());

            // When we reach this point the current font had all glyphs.
            break; 
        }
    }

    return;
}

}
