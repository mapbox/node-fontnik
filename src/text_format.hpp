#pragma once

#include "font_set.hpp"

// boost
#include <boost/optional.hpp>

namespace fontserver {

struct text_format {
    std::string face_name;
    std::string fontstack;

    boost::optional<font_set> fontset;

    double text_size;
};
typedef std::shared_ptr<text_format> text_format_ptr;

}
