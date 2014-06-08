#ifndef FONTSERVER_GLYPHS_HPP
#define FONTSERVER_GLYPHS_HPP

#include <node.h>

#include "glyphs.pb.h"

namespace fontserver
{

class Glyphs
{

public:
    Glyphs();
    Glyphs(const char *data, size_t length);
    ~Glyphs();

    std::string Serialize();
    void Range(std::string fontstack,
               std::string range,
               std::vector<std::uint32_t> chars);

    static std::string Trim(std::string str, std::string whitespace);

public:
    llmr::glyphs::glyphs glyphs;

};

} // ns fontserver

#endif // FONTSERVER_GLYPHS_HPP
