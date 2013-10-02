#include <pango/pangoft2.h>
#include "sdf_renderer.hpp"

struct _PangoSDFRenderer {
	PangoRenderer parent_instance;
    PangoSDFGlyphs glyphs;
};

typedef struct _PangoRendererClass PangoSDFRendererClass;


G_DEFINE_TYPE(PangoSDFRenderer, pango_sdf_renderer, PANGO_TYPE_RENDERER);

PangoRenderer *pango_sdf_get_renderer()
{
    return (PangoRenderer *)g_object_new(PANGO_TYPE_SDF_RENDERER, NULL);
}

void pango_sdf_renderer_reset(PangoSDFRenderer *renderer)
{
    renderer->glyphs.clear();
}

const PangoSDFGlyphs& pango_sdf_renderer_get_glyphs(PangoSDFRenderer *renderer)
{
    return renderer->glyphs;
}


static void pango_sdf_renderer_draw_glyph(PangoRenderer *renderer, PangoFont *font, PangoGlyph glyph, double x, double y)
{
    PangoSDFGlyphs& glyphs = PANGO_SDF_RENDERER(renderer)->glyphs;

    PangoSDFGlyph g;
    g.font = font;
    g.glyph = glyph;
    g.x = x;
    g.y = y;
    glyphs.push_back(g);
}


static void pango_sdf_renderer_draw_trapezoid(PangoRenderer *renderer, PangoRenderPart part G_GNUC_UNUSED, double y1, double x11, double x21, double y2, double x12, double x22)
{
    // fprintf(stderr, "trapezoid: y1:%f\tx11:%f\tx21:%f\ty2:%f\tx12:%f\tx22:%f\n", y1, x11, x21, y2, x12, x22);

}

static void pango_sdf_renderer_class_init(PangoSDFRendererClass *klass)
{
	PangoRendererClass *renderer_class = PANGO_RENDERER_CLASS(klass);
	renderer_class->draw_glyph = pango_sdf_renderer_draw_glyph;
	renderer_class->draw_trapezoid = pango_sdf_renderer_draw_trapezoid;
}

static void pango_sdf_renderer_init(PangoSDFRenderer *renderer G_GNUC_UNUSED)
{
}
