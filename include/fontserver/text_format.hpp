#pragma once

// mapnik
#include <mapnik/font_set.hpp>

// stl
#include <memory>

namespace fontserver {

struct text_format {
    text_format(std::string const& fontstack_, uint32_t text_size_)
        : fontstack(fontstack_),
          fontset(fontstack_),
          text_size(text_size_) {
        fontset.add_fontstack(fontstack, ',');
    };

    std::string fontstack;
    mapnik::font_set fontset;

    double text_size;
};
typedef std::shared_ptr<text_format> text_format_ptr;

}
