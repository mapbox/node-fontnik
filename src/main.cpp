#define HAVE_FREETYPE
#include <iostream>

#include "harfbuzz_shaper.hpp"

int main() {
    std::string fontstack = "Open Sans Regular, Arial Unicode MS Regular";
    std::map<unsigned, double> width_map_;
    fontserver::freetype_engine font_engine_;
    fontserver::face_manager_freetype font_manager(font_engine_);
    fontserver::font_set fset(fontstack);
    fontserver::face_set_ptr face_set = font_manager.get_face_set(fset);

    if (!face_set.size()) {
        std::cout << "could not find face_set";
        return 1;
    }

    fontserver::text_format format(fontstack, 24);
    fontserver::text_format_ptr format_ptr = std::make_shared<fontserver::text_format>(format);

    fontserver::text_itemizer itemizer;
    itemizer.add_text("Hello ", format_ptr);
    itemizer.add_text("World", format_ptr);
    itemizer.add_text("किகே", format_ptr);
    itemizer.add_text("وگرىmixed", format_ptr);
    itemizer.add_text("وگرى", format_ptr);

    typedef std::map<uint32_t, fontserver::glyph_info> Glyphs;
    Glyphs glyphs;

    std::string str;
    itemizer.text().toUTF8String(str);
    fontserver::text_line line(0, str.length() - 1);

    const double scale_factor = 1.0;

    fontserver::harfbuzz_shaper shaper;
    shaper.shape_text(line,
                      itemizer,
                      glyphs,
                      width_map_,
                      font_manager,
                      scale_factor);

    /*
    std::list<fontserver::text_item> const& list = itemizer.itemize();
    std::list<fontserver::text_item>::const_iterator itr = list.begin(), end = list.end();
    for (;itr!=end; itr++) {
        std::string s;
        unsigned start = itr->start;
        itemizer.text().tempSubString(start, itr->end - start).toUTF8String(s);
        std::cout << "Text item: text: " << s << " rtl: " << itr->rtl << " format: " << itr->format << " script: " << uscript_getName(itr->script) << "\n";
    }
    */

    return 0;
}

