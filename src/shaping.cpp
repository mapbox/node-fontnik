#include "shaping.hpp"
#include "font.hpp"


#include <pango/pangoft2.h>
#include "sdf_renderer.hpp"


static PangoContext *cached_context() {
    static PangoContext *context = NULL;
    if (context == NULL) {
        PangoFontMap *fontmap = pango_ft2_font_map_new();
        context = pango_font_map_create_context(fontmap);
    }
    return context;
}


using namespace v8;

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

    PangoContext *context = cached_context();
    PangoFontDescription *desc = pango_font_description_from_string(*font_stack);

    PangoLayout *layout = pango_layout_new(context);
    pango_layout_set_markup(layout, *text, text.length());
    pango_layout_set_font_description(layout, desc);

    PangoRenderer *renderer = pango_sdf_get_renderer();
    pango_renderer_draw_layout (renderer, layout, 0, 0);

    const PangoSDFGlyphs& glyphs = pango_sdf_renderer_get_glyphs(PANGO_SDF_RENDERER(renderer));

    Local<Array> result = Array::New();
    for (size_t i = 0; i < glyphs.size(); i++) {
        const PangoSDFGlyph& glyph = glyphs[i];
        Local<Object> info = Object::New();

        PangoFcFont *fc_font = PANGO_FC_FONT(glyph.font);
        FT_Face face = pango_fc_font_lock_face(fc_font);
        info->Set(String::NewSymbol("family"), String::New(face->family_name));
        info->Set(String::NewSymbol("style"), String::New(face->style_name));
        pango_fc_font_unlock_face (fc_font);

        info->Set(String::NewSymbol("glyph"), Uint32::New(glyph.glyph));
        info->Set(String::NewSymbol("x"), Number::New(glyph.x));
        info->Set(String::NewSymbol("y"), Number::New(glyph.y));

        result->Set(i, info);
    }
    return scope.Close(result);
}

