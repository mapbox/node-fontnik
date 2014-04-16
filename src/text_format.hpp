#pragma once

#include "font_set.hpp"

// boost
// #include <boost/optional.hpp>

namespace fontserver {

struct text_format {
    text_format(std::string const& fontstack_, uint32_t text_size_)
        : fontstack(fontstack_),
          text_size(text_size_) {
        // fontset->add_fontstack(fontstack, ',');
    };

    std::string fontstack;
    double text_size;
    // boost::optional<font_set> fontset;
};
typedef std::shared_ptr<text_format> text_format_ptr;

}
