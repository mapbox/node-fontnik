#include "shaping.hpp"
#include "font.hpp"
#include "globals.hpp"

#include <map>

#include <pango/pangoft2.h>
#include "sdf_renderer.hpp"


using namespace v8;

static Persistent<String> sym_x;
static Persistent<String> sym_y;
static Persistent<String> sym_glyph;
static Persistent<String> sym_face;


static PangoRenderer *renderer;

void InitShaping(Handle<Object> target) {
    renderer = pango_sdf_get_renderer();

    sym_x = Persistent<String>::New(String::NewSymbol("x"));
    sym_y = Persistent<String>::New(String::NewSymbol("y"));
    sym_glyph = Persistent<String>::New(String::NewSymbol("glyph"));
    sym_face = Persistent<String>::New(String::NewSymbol("face"));

    target->Set(String::NewSymbol("shape"), FunctionTemplate::New(Shaping)->GetFunction());
}





Handle<Value> Shaping(const Arguments& args) {
    HandleScope scope;

    if (args.Length() < 1) {
        return ThrowException(Exception::TypeError(String::New("First argument must be the string to shape")));
    }

    if (args.Length() < 2) {
        return ThrowException(Exception::TypeError(String::New("Second argument must be a font stack")));
    }

    String::Utf8Value text(args[0]->ToString());
    String::Utf8Value font_stack(args[1]->ToString());

    PangoFontDescription *desc = pango_font_description_from_string(*font_stack);
    pango_font_description_set_absolute_size(desc, 24 * 1024);

    PangoLayout *layout = pango_layout_new(pango_context());
    pango_layout_set_markup(layout, *text, text.length());
    pango_layout_set_font_description(layout, desc);

    pango_sdf_renderer_reset(PANGO_SDF_RENDERER(renderer));
    pango_renderer_draw_layout (renderer, layout, 0, 0);
    const PangoSDFGlyphs& glyphs = pango_sdf_renderer_get_glyphs(PANGO_SDF_RENDERER(renderer));

    std::map<PangoFont *, Handle<Value> > fonts;

    Local<Array> result = Array::New();
    for (size_t i = 0; i < glyphs.size(); i++) {
        const PangoSDFGlyph& glyph = glyphs[i];
        Local<Object> info = Object::New();

        auto pos = fonts.find(glyph.font);
        if (pos != fonts.end()) {
            info->Set(sym_face, pos->second);
        } else {
            fonts[glyph.font] = Font::New(glyph.font);
            info->Set(sym_face, fonts[glyph.font]);
        }

        info->Set(sym_glyph, Uint32::New(glyph.glyph));
        info->Set(sym_x, Number::New(glyph.x));
        info->Set(sym_y, Number::New(glyph.y));

        result->Set(i, info);
    }

    return scope.Close(result);
}

