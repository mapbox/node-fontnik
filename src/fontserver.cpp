#include <node.h>
#include "font.hpp"
#include "shaping.hpp"

using namespace v8;



void RegisterModule(Handle<Object> target) {
    Font::Init(target);
    target->Set(String::NewSymbol("shape"), FunctionTemplate::New(Shaping)->GetFunction());


    // PangoFontDescription *desc = pango_font_description_new();
    // pango_font_description_set_family(desc, "Avenir Next");


    // PangoFont *font = pango_font_map_load_font(fontmap, context, desc);

    // PangoFontDescription *desc2 = pango_font_describe(font);

    // fprintf(stderr, "font: %s\n", pango_font_description_to_string(desc));




    // int n_families;
    // PangoFontFamily **families;
    // pango_font_map_list_families(fontmap, & families, & n_families);
    // printf ("There are %d families\n", n_families);
    // for (i = 0; i < n_families; i++) {
    //     PangoFontFamily * family = families[i];
    //     const char * family_name;

    //     family_name = pango_font_family_get_name (family);
    //     printf ("Family %d: %s\n", i, family_name);
    // }
    // g_free (families);
}

NODE_MODULE(fontserver, RegisterModule);
