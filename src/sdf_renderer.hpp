#pragma once

#include <string>
#include <vector>
#include <pango/pango-renderer.h>

typedef struct _PangoSDFRenderer PangoSDFRenderer;
#define PANGO_TYPE_SDF_RENDERER      (pango_sdf_renderer_get_type())
#define PANGO_SDF_RENDERER(object)   (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_SDF_RENDERER, PangoSDFRenderer))
GType pango_sdf_renderer_get_type(void) G_GNUC_CONST;


struct PangoSDFGlyph {
    PangoFont *font;
    PangoGlyph glyph;
    double x, y;
};

typedef std::vector<PangoSDFGlyph> PangoSDFGlyphs;

PangoRenderer *pango_sdf_get_renderer();
void pango_sdf_renderer_reset(PangoSDFRenderer *renderer);
const PangoSDFGlyphs& pango_sdf_renderer_get_glyphs(PangoSDFRenderer *renderer);
