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

// fontnik
#include <fontnik/glyphs.hpp>
#include <mapnik/font_engine_freetype.hpp>
#include <mapnik/text/face_set.hpp>

// stl
#include <algorithm>
#include <memory>
#include <sstream>

// freetype2
extern "C"
{
#include <ft2build.h>
#include FT_FREETYPE_H
}

namespace fontnik
{

Glyphs::Glyphs() {}

Glyphs::Glyphs(const char *data, size_t length)
{
    glyphs.ParseFromArray(data, length);
}

Glyphs::~Glyphs() {}

std::string Glyphs::Serialize()
{
    return glyphs.SerializeAsString();
}

void Glyphs::Range(std::string fontstack,
                   std::string range,
                   std::vector<std::uint32_t> chars)
{
    mapnik_fontnik::freetype_engine font_engine_;
    mapnik_fontnik::face_manager_freetype font_manager(font_engine_);

    mapnik_fontnik::font_set font_set(fontstack);
    std::stringstream stream(fontstack);
    std::string face_name;

    // TODO: better to split on delim and font_names_.reserve() then add?
    while (std::getline(stream, face_name, ',')) {
        font_set.add_face_name(Trim(face_name, " \t"));
    }

    mapnik_fontnik::face_set_ptr face_set;

    // This may throw.
    face_set = font_manager.get_face_set(font_set);

    llmr::glyphs::fontstack *mutable_fontstack = glyphs.add_stacks();
    mutable_fontstack->set_name(fontstack);
    mutable_fontstack->set_range(range);

    const double scale_factor = 1.0;

    // Set character sizes.
    double size = 24 * scale_factor;
    face_set->set_character_sizes(size);

    for (std::vector<uint32_t>::size_type i = 0; i != chars.size(); i++) {
        FT_ULong char_code = chars[i];
        mapnik_fontnik::glyph_info glyph;

        for (auto const& face : *face_set) {
            // Get FreeType face from face_ptr.
            FT_Face ft_face = face->get_face();
            FT_UInt char_index = FT_Get_Char_Index(ft_face, char_code);

            // Try next font in fontset.
            if (!char_index) continue;

            glyph.glyph_index = char_index;
            face->RenderSDF(glyph, 24, 3, 0.25);

            // Add glyph to fontstack.
            llmr::glyphs::glyph *mutable_glyph = mutable_fontstack->add_glyphs();
            mutable_glyph->set_id(char_code);
            mutable_glyph->set_width(glyph.width);
            mutable_glyph->set_height(glyph.height);
            mutable_glyph->set_left(glyph.left);
            mutable_glyph->set_top(glyph.top - glyph.ascender);
            mutable_glyph->set_advance(glyph.advance);

            if (glyph.width > 0) {
                mutable_glyph->set_bitmap(glyph.bitmap);
            }

            // Glyph added, continue to next char_code.
            break;
        }
    }
}

std::vector<int> Glyphs::Codepoints(std::string const& fontstack)
{
    std::vector<int> points;
    mapnik_fontnik::freetype_engine font_engine_;
    mapnik_fontnik::face_manager_freetype font_manager(font_engine_);

    mapnik_fontnik::font_set font_set(fontstack);
    std::stringstream stream(fontstack);
    std::string face_name;

    while (std::getline(stream, face_name, ',')) {
        font_set.add_face_name(Trim(face_name, " \t"));
    }

    mapnik_fontnik::face_set_ptr face_set;

    // This may throw.
    face_set = font_manager.get_face_set(font_set);
    for (auto const& face : *face_set) {
        FT_Face ft_face = face->get_face();
        FT_ULong  charcode;
        FT_UInt   gindex;
        charcode = FT_Get_First_Char(ft_face, &gindex);
        while (gindex != 0) {
            charcode = FT_Get_Next_Char(ft_face, charcode, &gindex);
            if (charcode != 0) points.push_back(charcode);
        }
    }

    // for font sets, eliminate duplicates of codepoints
    // that are shared.
    std::sort(points.begin(), points.end());
    auto last = std::unique(points.begin(), points.end());
    points.erase(last, points.end());

    return points;
}

std::vector<std::map<std::string, std::string>> Glyphs::FontInfo(std::string const& fontstack)
{
    std::vector<std::map<std::string, std::string>> faces;
    mapnik_fontnik::freetype_engine font_engine_;
    mapnik_fontnik::face_manager_freetype font_manager(font_engine_);

    mapnik_fontnik::font_set font_set(fontstack);
    std::stringstream stream(fontstack);
    std::string face_name;

    while (std::getline(stream, face_name, ',')) {
        font_set.add_face_name(Trim(face_name, " \t"));
    }

    mapnik_fontnik::face_set_ptr face_set;

    face_set = font_manager.get_face_set(font_set);
    for (auto const& face : *face_set) {
        std::map<std::string, std::string> metadata;
        FT_Face ft_face = face->get_face();
        FT_String* family_name = ft_face->family_name;
        FT_String* style_name = ft_face->style_name;
        metadata.emplace("family_name", family_name);
        metadata.emplace("style_name", style_name);
        faces.push_back(metadata);
    }

    return faces;
}

std::string Glyphs::Trim(std::string str, std::string whitespace)
{
    const auto strBegin = str.find_first_not_of(whitespace);

    if (strBegin == std::string::npos) {
        return "";
    }

    const auto strEnd = str.find_last_not_of(whitespace);
    const auto strRange = strEnd - strBegin + 1;

    return str.substr(strBegin, strRange);
}

} // ns fontnik
