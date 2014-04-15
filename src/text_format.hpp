#pragma once

// stl
#include <memory>

// boost
#include <boost/optional.hpp>

namespace fontserver {

struct text_format {
    text_format();

    std::string face_name;
    std::string fontstack;

    boost::optional<font_set> fontset;

    double text_size;
};

}
