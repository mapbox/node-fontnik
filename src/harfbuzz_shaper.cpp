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
#include "text_line.hpp"
#include "tile_face.hpp"
#include "font_face_set.hpp"
#include "font_set.hpp"

#include <iostream>

HarfbuzzShaper::HarfbuzzShaper() {};

HarfbuzzShaper::~HarfbuzzShaper() {};

void HarfbuzzShaper::Shape(std::string &value,
                           std::string &fontstack,
                           std::map<uint32_t, fontserver::glyph_info> &glyphs,
                           std::map<unsigned,double> &width_map,
                           fontserver::face_manager_freetype &font_manager,
                           double scale_factor) {
    fontserver::text_line line(0, value.size() - 1);

    unsigned start = line.first_char();
    unsigned end = line.last_char();
    UnicodeString const &text = value.data();
    size_t length = end - start;
    if (!length) return;

    // Preallocate memory based on estimated length.
    line.reserve(length);

    // std::list<text_item> const& list = itemizer.itemize(start, end);

    auto hb_buffer_deleter = [](hb_buffer_t * buffer) { hb_buffer_destroy(buffer);};
    const std::unique_ptr<hb_buffer_t, decltype(hb_buffer_deleter)> buffer(hb_buffer_create(),hb_buffer_deleter);
    hb_buffer_set_unicode_funcs(buffer.get(), hb_icu_get_unicode_funcs());
    hb_buffer_pre_allocate(buffer.get(), length);

    // for (auto const& text_item : list) {
        font_set fonts = font_set(fontstack);
        fonts.add_fontstack(fontstack, ',');

        // face_set_ptr face_set = font_manager.get_face_set(text_item.format->face_name, text_item.format->fontset);
        // double size = text_item.format->text_size * scale_factor;
        fontserver::face_set_ptr face_set = font_manager.get_face_set(fonts);
        double size = 24.0 * scale_factor;
        face_set->set_character_sizes(size);

        fontserver::font_face_set::iterator face_itr = face_set->begin(), face_end = face_set->end();
        for (; face_itr != face_end; ++face_itr) {
            fontserver::face_ptr const& face = *face_itr;

            hb_buffer_clear_contents(buffer.get());
            hb_buffer_add_utf16(buffer.get(), text.getBuffer(), text.length(), 0, length);
            // hb_buffer_set_direction(buffer.get(), (text_item.rtl == UBIDI_RTL)?HB_DIRECTION_RTL:HB_DIRECTION_LTR);
            hb_buffer_set_direction(buffer.get(), HB_DIRECTION_LTR);
            // hb_buffer_set_script(buffer.get(), hb_icu_script_to_script(text_item.script));
            hb_font_t *font(hb_ft_font_create(face->get_face(), nullptr));
            hb_shape(font, buffer.get(), NULL, 0);
            hb_font_destroy(font);

            unsigned num_glyph_infos = hb_buffer_get_length(buffer.get());

            hb_glyph_info_t *glyph_infos = hb_buffer_get_glyph_infos(buffer.get(), nullptr);
            hb_glyph_position_t *positions = hb_buffer_get_glyph_positions(buffer.get(), nullptr);

            bool font_has_all_glyphs = true;
            // Check if all glyphs are valid.
            for (unsigned i = 0; i < num_glyph_infos; ++i) {
                if (!glyph_infos[i].codepoint) {
                    font_has_all_glyphs = false;
                    break;
                }
            }

            if (!font_has_all_glyphs && face_itr+1 != face_end) {
                // Try next font in fontset.
                continue;
            }

            pixel_position cluster_offset;
            for (unsigned i = 0; i < num_glyph_infos; ++i) {
                fontserver::glyph_info tmp;
                tmp.char_index = glyph_infos[i].cluster;
                tmp.glyph_index = glyph_infos[i].codepoint;
                tmp.face = face->get_face();
                // tmp.format = text_item.format;
                face->glyph_dimensions(tmp);

                // Overwrite default width with better value from HarfBuzz
                tmp.width = positions[i].x_advance / 64.0;

                cluster_offset.x += tmp.width;
                tmp.x = cluster_offset.x;

                tmp.offset.set(positions[i].x_offset / 64.0,
                               positions[i].y_offset / 64.0);

                width_map[glyph_infos[i].cluster] += tmp.width;
                line.add_glyph(tmp, scale_factor);

                glyphs.emplace(tmp.glyph_index, tmp);
            }

            line.update_max_char_height(face->get_char_height());

            // When we reach this point the current font had all glyphs.
            break; 
        }
    // }
}
