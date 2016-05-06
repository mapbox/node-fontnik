/*
 * Copyright © 2009  Red Hat, Inc.
 * Copyright © 2009  Keith Stribley
 * Copyright © 2015  Google, Inc.
 *
 *  This is part of HarfBuzz, a text shaping library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Red Hat Author(s): Behdad Esfahbod
 * Google Author(s): Behdad Esfahbod
 */

#include "hb-private.hh"

#include "hb-ft.h"

#include "hb-font-private.hh"

#include <protozero/pbf_reader.hpp>

#include <iostream>

#include FT_ADVANCES_H
#include FT_TRUETYPE_TABLES_H
#include FT_TRUETYPE_TAGS_H



#ifndef HB_DEBUG_FT
#define HB_DEBUG_FT (HB_DEBUG+0)
#endif


/* TODO:
 *
 * In general, this file does a fine job of what it's supposed to do.
 * There are, however, things that need more work:
 *
 *   - I remember seeing FT_Get_Advance() without the NO_HINTING flag to be buggy.
 *     Have not investigated.
 *
 *   - FreeType works in 26.6 mode.  Clients can decide to use that mode, and everything
 *     would work fine.  However, we also abuse this API for performing in font-space,
 *     but don't pass the correct flags to FreeType.  We just abuse the no-hinting mode
 *     for that, such that no rounding etc happens.  As such, we don't set ppem, and
 *     pass NO_HINTING as load_flags.  Would be much better to use NO_SCALE, and scale
 *     ourselves, like we do in uniscribe, etc.
 *
 *   - We don't handle / allow for emboldening / obliqueing.
 *
 *   - In the future, we should add constructors to create fonts in font space?
 *
 *   - FT_Load_Glyph() is exteremely costly.  Do something about it?
 */


struct hb_ft_font_t
{
  char* ft_face;
  int load_flags;
  bool unref; /* Whether to destroy ft_face when done. */
};

static hb_ft_font_t *
_hb_ft_font_create (char* ft_face, bool unref)
{
  std::cout << "_hb_ft_font_create" << std::endl;

  hb_ft_font_t *ft_font = (hb_ft_font_t *) calloc (1, sizeof (hb_ft_font_t));

  if (unlikely (!ft_font))
    return NULL;

  ft_font->ft_face = ft_face;
  ft_font->unref = unref;

  ft_font->load_flags = FT_LOAD_DEFAULT | FT_LOAD_NO_HINTING;

  return ft_font;
}

static void
_hb_ft_font_destroy (hb_ft_font_t *ft_font)
{
  if (ft_font->unref) {
    free (ft_font->ft_face);
    // FT_Done_Face (ft_font->ft_face);
  }

  free (ft_font);
}

/**
 * hb_ft_font_set_load_flags:
 * @font:
 * @load_flags:
 *
 * 
 *
 * Since: 1.0.5
 **/
void
hb_ft_font_set_load_flags (hb_font_t *font, int load_flags)
{
  if (font->immutable)
    return;

  if (font->destroy != (hb_destroy_func_t) _hb_ft_font_destroy)
    return;

  hb_ft_font_t *ft_font = (hb_ft_font_t *) font->user_data;

  ft_font->load_flags = load_flags;
}

/**
 * hb_ft_font_get_load_flags:
 * @font:
 *
 * 
 *
 * Return value:
 * Since: 1.0.5
 **/
int
hb_ft_font_get_load_flags (hb_font_t *font)
{
  if (font->destroy != (hb_destroy_func_t) _hb_ft_font_destroy)
    return 0;

  const hb_ft_font_t *ft_font = (const hb_ft_font_t *) font->user_data;

  return ft_font->load_flags;
}

char*
hb_ft_font_get_face (hb_font_t *font)
{
  if (font->destroy != (hb_destroy_func_t) _hb_ft_font_destroy)
    return NULL;

  const hb_ft_font_t *ft_font = (const hb_ft_font_t *) font->user_data;

  return ft_font->ft_face;
}



static hb_bool_t
hb_ft_get_nominal_glyph (hb_font_t *font HB_UNUSED,
			 void *font_data,
			 hb_codepoint_t unicode,
			 hb_codepoint_t *glyph,
			 void *user_data HB_UNUSED)
{
  const hb_ft_font_t *ft_font = (const hb_ft_font_t *) font_data;

  // TODO: fix this lookup
  // unsigned int g = FT_Get_Char_Index (ft_font->ft_face, unicode);
  unsigned int g = unicode;

  std::cout << "hb_ft_get_nominal_glyph: " << g << std::endl;
  throw;

  if (unlikely (!g))
    return false;

  *glyph = g;
  return true;
}

static hb_bool_t
hb_ft_get_variation_glyph (hb_font_t *font HB_UNUSED,
			   void *font_data,
			   hb_codepoint_t unicode,
			   hb_codepoint_t variation_selector,
			   hb_codepoint_t *glyph,
			   void *user_data HB_UNUSED)
{
  const hb_ft_font_t *ft_font = (const hb_ft_font_t *) font_data;
  // TODO: fix this lookup
  // unsigned int g = FT_Face_GetCharVariantIndex (ft_font->ft_face, unicode, variation_selector);
  unsigned int g = unicode;

  if (unlikely (!g))
    return false;

  *glyph = g;
  return true;
}

static hb_position_t
hb_ft_get_glyph_h_advance (hb_font_t *font HB_UNUSED,
			   void *font_data,
			   hb_codepoint_t glyph,
			   void *user_data HB_UNUSED)
{
  const hb_ft_font_t *ft_font = (const hb_ft_font_t *) font_data;
  FT_Fixed v = 0;

  // TODO: check advance
  // if (unlikely (FT_Get_Advance (ft_font->ft_face, glyph, ft_font->load_flags, &v)))
  if (false)
    return 0;

  if (font->x_scale < 0)
    v = -v;

  return (v + (1<<9)) >> 10;
}

static hb_position_t
hb_ft_get_glyph_v_advance (hb_font_t *font HB_UNUSED,
			   void *font_data,
			   hb_codepoint_t glyph,
			   void *user_data HB_UNUSED)
{
  const hb_ft_font_t *ft_font = (const hb_ft_font_t *) font_data;
  FT_Fixed v = 0;

  // TODO: check advance
  // if (unlikely (FT_Get_Advance (ft_font->ft_face, glyph, ft_font->load_flags | FT_LOAD_VERTICAL_LAYOUT, &v)))
  if (false)
    return 0;

  if (font->y_scale < 0)
    v = -v;

  /* Note: FreeType's vertical metrics grows downward while other FreeType coordinates
   * have a Y growing upward.  Hence the extra negation. */
  return (-v + (1<<9)) >> 10;
}

static hb_bool_t
hb_ft_get_glyph_v_origin (hb_font_t *font HB_UNUSED,
			  void *font_data,
			  hb_codepoint_t glyph,
			  hb_position_t *x,
			  hb_position_t *y,
			  void *user_data HB_UNUSED)
{
  const hb_ft_font_t *ft_font = (const hb_ft_font_t *) font_data;
  char* ft_face = ft_font->ft_face;

  // TODO: load glyph
  // if (unlikely (FT_Load_Glyph (ft_face, glyph, ft_font->load_flags)))
  if (false)
    return false;

  /* Note: FreeType's vertical metrics grows downward while other FreeType coordinates
   * have a Y growing upward.  Hence the extra negation. */
  // TODO: get bearing
  /*
  *x = ft_face->glyph->metrics.horiBearingX -   ft_face->glyph->metrics.vertBearingX;
  *y = ft_face->glyph->metrics.horiBearingY - (-ft_face->glyph->metrics.vertBearingY);
  */

  if (font->x_scale < 0)
    *x = -*x;
  if (font->y_scale < 0)
    *y = -*y;

  return true;
}

static hb_position_t
hb_ft_get_glyph_h_kerning (hb_font_t *font,
			   void *font_data,
			   hb_codepoint_t left_glyph,
			   hb_codepoint_t right_glyph,
			   void *user_data HB_UNUSED)
{
  const hb_ft_font_t *ft_font = (const hb_ft_font_t *) font_data;
  FT_Vector kerningv;

  FT_Kerning_Mode mode = font->x_ppem ? FT_KERNING_DEFAULT : FT_KERNING_UNFITTED;
  // TODO: get kerning
  // if (FT_Get_Kerning (ft_font->ft_face, left_glyph, right_glyph, mode, &kerningv))
  if (false)
    return 0;

  return kerningv.x;
}

static hb_bool_t
hb_ft_get_glyph_extents (hb_font_t *font HB_UNUSED,
			 void *font_data,
			 hb_codepoint_t glyph,
			 hb_glyph_extents_t *extents,
			 void *user_data HB_UNUSED)
{
  const hb_ft_font_t *ft_font = (const hb_ft_font_t *) font_data;
  char* ft_face = ft_font->ft_face;

  std::cout << "hb_ft_get_glyph_extents: " << glyph << std::endl;

  // TODO: load glyph
  // if (unlikely (FT_Load_Glyph (ft_face, glyph, ft_font->load_flags)))
  if (false)
    return false;

  // TODO: get bearing and dimensions
  /*
  extents->x_bearing = ft_face->glyph->metrics.horiBearingX;
  extents->y_bearing = ft_face->glyph->metrics.horiBearingY;
  extents->width = ft_face->glyph->metrics.width;
  extents->height = -ft_face->glyph->metrics.height;
  */
  if (font->x_scale < 0)
  {
    extents->x_bearing = -extents->x_bearing;
    extents->width = -extents->width;
  }
  if (font->y_scale < 0)
  {
    extents->y_bearing = -extents->y_bearing;
    extents->height = -extents->height;
  }
  return true;
}

static hb_bool_t
hb_ft_get_glyph_contour_point (hb_font_t *font HB_UNUSED,
			       void *font_data,
			       hb_codepoint_t glyph,
			       unsigned int point_index,
			       hb_position_t *x,
			       hb_position_t *y,
			       void *user_data HB_UNUSED)
{
  const hb_ft_font_t *ft_font = (const hb_ft_font_t *) font_data;
  char* ft_face = ft_font->ft_face;

  std::cout << "hb_ft_get_glyph_contour_point: " << glyph <<
               "point_index: " << point_index << std::endl;

  // TODO: load glyph
  // if (unlikely (FT_Load_Glyph (ft_face, glyph, ft_font->load_flags)))
  if (false)
      return false;

  // TODO: verify glyph_format_outline
  // if (unlikely (ft_face->glyph->format != FT_GLYPH_FORMAT_OUTLINE))
  if (false)
      return false;

  // TODO: verify points arent in wrong order
  // if (unlikely (point_index >= (unsigned int) ft_face->glyph->outline.n_points))
  if (false)
      return false;

  // TODO: get outline points
  /*
  *x = ft_face->glyph->outline.points[point_index].x;
  *y = ft_face->glyph->outline.points[point_index].y;
  */

  return true;
}

static hb_bool_t
hb_ft_get_glyph_name (hb_font_t *font HB_UNUSED,
		      void *font_data,
		      hb_codepoint_t glyph,
		      char *name, unsigned int size,
		      void *user_data HB_UNUSED)
{
  const hb_ft_font_t *ft_font = (const hb_ft_font_t *) font_data;


  std::cout << "hb_ft_get_glyph_name: " << glyph << std::endl;

  // hb_bool_t ret = !FT_Get_Glyph_Name (ft_font->ft_face, glyph, name, size);
  hb_bool_t ret = false;
  if (ret && (size && !*name))
    ret = false;

  return ret;
}

static hb_bool_t
hb_ft_get_glyph_from_name (hb_font_t *font HB_UNUSED,
			   void *font_data,
			   const char *name, int len, /* -1 means nul-terminated */
			   hb_codepoint_t *glyph,
			   void *user_data HB_UNUSED)
{
  const hb_ft_font_t *ft_font = (const hb_ft_font_t *) font_data;
  char* ft_face = ft_font->ft_face;

  std::cout << "hb_ft_get_glyph_from_name: " << name << std::endl;

  // TODO: can we eliminate this function?
  /*
  if (len < 0)
    *glyph = FT_Get_Name_Index (ft_face, (FT_String *) name);
  else {
    // Make a nul-terminated version.
    char buf[128];
    len = MIN (len, (int) sizeof (buf) - 1);
    strncpy (buf, name, len);
    buf[len] = '\0';
    *glyph = FT_Get_Name_Index (ft_face, buf);
  }

  if (*glyph == 0)
  {
    // Check whether the given name was actually the name of glyph 0.
    char buf[128];
    if (!FT_Get_Glyph_Name(ft_face, 0, buf, sizeof (buf)) &&
        len < 0 ? !strcmp (buf, name) : !strncmp (buf, name, len))
      return true;
  }

  return *glyph != 0;
  */

  return false;
}

static hb_bool_t
hb_ft_get_font_h_extents (hb_font_t *font HB_UNUSED,
			  void *font_data,
			  hb_font_extents_t *metrics,
			  void *user_data HB_UNUSED)
{
  const hb_ft_font_t *ft_font = (const hb_ft_font_t *) font_data;
  char* ft_face = ft_font->ft_face;

  std::cout << "hb_ft_get_font_h_extents" << std::endl;

  // TODO: get these metrics
  /*
  metrics->ascender = ft_face->size->metrics.ascender;
  metrics->descender = ft_face->size->metrics.descender;
  metrics->line_gap = ft_face->size->metrics.height - (ft_face->size->metrics.ascender - ft_face->size->metrics.descender);
  */

  if (font->y_scale < 0)
  {
    metrics->ascender = -metrics->ascender;
    metrics->descender = -metrics->descender;
    metrics->line_gap = -metrics->line_gap;
  }
  return true;
}

static hb_font_funcs_t *static_ft_funcs = NULL;

#ifdef HB_USE_ATEXIT
static
void free_static_ft_funcs (void)
{
  hb_font_funcs_destroy (static_ft_funcs);
}
#endif

static void
_hb_ft_font_set_funcs (hb_font_t *font, char* ft_face, bool unref)
{
  std::cout << "_hb_ft_font_set_funcs" << std::endl;

retry:
  hb_font_funcs_t *funcs = (hb_font_funcs_t *) hb_atomic_ptr_get (&static_ft_funcs);

  if (unlikely (!funcs))
  {
    funcs = hb_font_funcs_create ();

    hb_font_funcs_set_font_h_extents_func (funcs, hb_ft_get_font_h_extents, NULL, NULL);
    //hb_font_funcs_set_font_v_extents_func (funcs, hb_ft_get_font_v_extents, NULL, NULL);
    hb_font_funcs_set_nominal_glyph_func (funcs, hb_ft_get_nominal_glyph, NULL, NULL);
    hb_font_funcs_set_variation_glyph_func (funcs, hb_ft_get_variation_glyph, NULL, NULL);
    hb_font_funcs_set_glyph_h_advance_func (funcs, hb_ft_get_glyph_h_advance, NULL, NULL);
    hb_font_funcs_set_glyph_v_advance_func (funcs, hb_ft_get_glyph_v_advance, NULL, NULL);
    //hb_font_funcs_set_glyph_h_origin_func (funcs, hb_ft_get_glyph_h_origin, NULL, NULL);
    hb_font_funcs_set_glyph_v_origin_func (funcs, hb_ft_get_glyph_v_origin, NULL, NULL);
    hb_font_funcs_set_glyph_h_kerning_func (funcs, hb_ft_get_glyph_h_kerning, NULL, NULL);
    //hb_font_funcs_set_glyph_v_kerning_func (funcs, hb_ft_get_glyph_v_kerning, NULL, NULL);
    hb_font_funcs_set_glyph_extents_func (funcs, hb_ft_get_glyph_extents, NULL, NULL);
    hb_font_funcs_set_glyph_contour_point_func (funcs, hb_ft_get_glyph_contour_point, NULL, NULL);
    hb_font_funcs_set_glyph_name_func (funcs, hb_ft_get_glyph_name, NULL, NULL);
    hb_font_funcs_set_glyph_from_name_func (funcs, hb_ft_get_glyph_from_name, NULL, NULL);

    hb_font_funcs_make_immutable (funcs);

    if (!hb_atomic_ptr_cmpexch (&static_ft_funcs, NULL, funcs)) {
      hb_font_funcs_destroy (funcs);
      goto retry;
    }

#ifdef HB_USE_ATEXIT
    atexit (free_static_ft_funcs); /* First person registers atexit() callback. */
#endif
  };

  hb_font_set_funcs (font,
		     funcs,
		     _hb_ft_font_create (ft_face, unref),
		     (hb_destroy_func_t) _hb_ft_font_destroy);
}


static hb_blob_t *
reference_table  (hb_face_t *face HB_UNUSED, hb_tag_t tag, void *user_data)
{
  std::string tag_name;

  switch (tag) {
    case TTAG_avar: tag_name = "avar"; break;
    case TTAG_BASE: tag_name = "BASE"; break;
    case TTAG_bdat: tag_name = "bdat"; break;
    case TTAG_BDF:
      tag_name = "BDF "; break;
    case TTAG_bhed:
      tag_name = "bhed"; break;
    case TTAG_bloc:
      tag_name = "bloc"; break;
    case TTAG_bsln:
      tag_name = "bsln"; break;
    case TTAG_CBDT:
      tag_name = "CBDT"; break;
    case TTAG_CBLC:
      tag_name = "CBLC"; break;
    case TTAG_CFF:
      tag_name = "CFF "; break;
    case TTAG_CID:
      tag_name = "CID "; break;
    case TTAG_cmap:
      tag_name = "cmap"; break;
    case TTAG_cvar:
      tag_name = "cvar"; break;
    case TTAG_cvt:
      tag_name = "cvt "; break;
    case TTAG_DSIG:
      tag_name = "DSIG"; break;
    case TTAG_EBDT:
      tag_name = "EBDT"; break;
    case TTAG_EBLC:
      tag_name = "EBLC"; break;
    case TTAG_EBSC:
      tag_name = "EBSC"; break;
    case TTAG_feat:
      tag_name = "feat"; break;
    case TTAG_FOND:
      tag_name = "FOND"; break;
    case TTAG_fpgm:
      tag_name = "fpgm"; break;
    case TTAG_fvar:
      tag_name = "fvar"; break;
    case TTAG_gasp:
      tag_name = "gasp"; break;
    case TTAG_GDEF:
      tag_name = "GDEF"; break;
    case TTAG_glyf:
      tag_name = "glyf"; break;
    case TTAG_GPOS:
      tag_name = "GPOS"; break;
    case TTAG_GSUB:
      tag_name = "GSUB"; break;
    case TTAG_gvar:
      tag_name = "gvar"; break;
    case TTAG_hdmx:
      tag_name = "hdmx"; break;
    case TTAG_head:
      tag_name = "head"; break;
    case TTAG_hhea:
      tag_name = "hhea"; break;
    case TTAG_hmtx:
      tag_name = "hmtx"; break;
    case TTAG_JSTF:
      tag_name = "JSTF"; break;
    case TTAG_just:
      tag_name = "just"; break;
    case TTAG_kern:
      tag_name = "kern"; break;
    case TTAG_lcar:
      tag_name = "lcar"; break;
    case TTAG_loca:
      tag_name = "loca"; break;
    case TTAG_LTSH:
      tag_name = "LTSH"; break;
    case TTAG_LWFN:
      tag_name = "LWFN"; break;
    case TTAG_MATH:
      tag_name = "MATH"; break;
    case TTAG_maxp:
      tag_name = "maxp"; break;
    case TTAG_META:
      tag_name = "META"; break;
    case TTAG_MMFX:
      tag_name = "MMFX"; break;
    case TTAG_MMSD:
      tag_name = "MMSD"; break;
    case TTAG_mort:
      tag_name = "mort"; break;
    case TTAG_morx:
      tag_name = "morx"; break;
    case TTAG_name:
      tag_name = "name"; break;
    case TTAG_opbd:
      tag_name = "opbd"; break;
    case TTAG_OS2:
      tag_name = "OS/2"; break;
    case TTAG_OTTO:
      tag_name = "OTTO"; break;
    case TTAG_PCLT:
      tag_name = "PCLT"; break;
    case TTAG_POST:
      tag_name = "POST"; break;
    case TTAG_post:
      tag_name = "post"; break;
    case TTAG_prep:
      tag_name = "prep"; break;
    case TTAG_prop:
      tag_name = "prop"; break;
    case TTAG_sbix:
      tag_name = "sbix"; break;
    case TTAG_sfnt:
      tag_name = "sfnt"; break;
    case TTAG_SING:
      tag_name = "SING"; break;
    case TTAG_trak:
      tag_name = "trak"; break;
    case TTAG_true:
      tag_name = "true"; break;
    case TTAG_ttc:
      tag_name = "ttc "; break;
    case TTAG_ttcf:
      tag_name = "ttcf"; break;
    case TTAG_TYP1:
      tag_name = "TYP1"; break;
    case TTAG_typ1:
      tag_name = "typ1"; break;
    case TTAG_VDMX:
      tag_name = "VDMX"; break;
    case TTAG_vhea:
      tag_name = "vhea"; break;
    case TTAG_vmtx:
      tag_name = "vmtx"; break;
    case TTAG_wOFF:
      tag_name = "WOFF"; break;
    default:
      tag_name = "unknown"; break;
  }

  std::cout << "reference_table: " << tag_name << std::endl;

  FT_Face ft_face = (FT_Face) user_data;
  FT_Byte *buffer;
  FT_ULong  length = 0;
  FT_Error error;

  /* Note: FreeType like HarfBuzz uses the NONE tag for fetching the entire blob */

  error = FT_Load_Sfnt_Table (ft_face, tag, 0, NULL, &length);
  if (error)
    return NULL;

  buffer = (FT_Byte *) malloc (length);
  if (buffer == NULL)
    return NULL;

  error = FT_Load_Sfnt_Table (ft_face, tag, 0, buffer, &length);
  if (error)
    return NULL;

  return hb_blob_create ((const char *) buffer, length,
			 HB_MEMORY_MODE_WRITABLE,
			 buffer, free);
}

/**
 * hb_ft_face_create:
 * @ft_face: (destroy destroy) (scope notified): 
 * @destroy:
 *
 * 
 *
 * Return value: (transfer full): 
 * Since: 0.9.2
 **/
hb_face_t *
hb_ft_face_create (char*           ft_face,
                   size_t ft_face_size,
		   hb_destroy_func_t destroy)
{
  std::cout << "hb_ft_face_create" << std::endl;

  hb_face_t *face = nullptr;

  struct GlyphMetrics {
      operator bool() const {
          return !(width == 0 && height == 0 && advance == 0);
      }

      // Glyph metrics.
      int32_t x_bearing = 0;
      int32_t y_bearing = 0;
      uint32_t width = 0;
      uint32_t height = 0;
      uint32_t advance = 0;

  };

  class Glyph {
  public:
      uint32_t glyph_index = 0;
      uint32_t codepoint = 0;

      // Glyph metrics
      GlyphMetrics metrics;

      // A signed distance field of the glyph
      std::string bitmap;
  };

  /*
  if (ft_face->stream->read == NULL) {
    hb_blob_t *blob;

    blob = hb_blob_create ((const char *) ft_face->stream->base,
			   (unsigned int) ft_face->stream->size,
			   HB_MEMORY_MODE_READONLY,
			   ft_face, destroy);
    face = hb_face_create (blob, ft_face->face_index);
    hb_blob_destroy (blob);
  } else {
    face = hb_face_create_for_tables (reference_table, ft_face, destroy);
  }
  */

  face = hb_face_create_for_tables (reference_table, ft_face, destroy);

  const std::string font_string(ft_face, ft_face_size);
  protozero::pbf_reader font_pbf(font_string);

  while (font_pbf.next(1)) {
      auto face_pbf = font_pbf.get_message();

      while (face_pbf.next(4)) {
          auto face_metrics_pbf = face_pbf.get_message();

          while (face_metrics_pbf.next()) {
              switch (face_metrics_pbf.tag()) {
              case 1: // face_index
                  hb_face_set_index (face, face_metrics_pbf.get_uint32());
                  std::cout << "face_index: " << hb_face_get_index(face) << std::endl;
                  break;
              case 2: // units per EM
                  hb_face_set_upem (face, face_metrics_pbf.get_uint32());
                  std::cout << "upem: " << hb_face_get_upem(face) << std::endl;
                  break;
              default:
                  face_metrics_pbf.skip();
                  break;
              }
          }
      }
  }

  return face;
}

/**
 * hb_ft_face_create_referenced:
 * @ft_face:
 *
 * 
 *
 * Return value: (transfer full): 
 * Since: 0.9.38
 **/
/*
hb_face_t *
hb_ft_face_create_referenced (FT_Face ft_face)
{
  FT_Reference_Face (ft_face);
  return hb_ft_face_create (ft_face, (hb_destroy_func_t) FT_Done_Face);
}

static void
hb_ft_face_finalize (FT_Face ft_face)
{
  hb_face_destroy ((hb_face_t *) ft_face->generic.data);
}
*/

/**
 * hb_ft_face_create_cached:
 * @ft_face: 
 *
 * 
 *
 * Return value: (transfer full): 
 * Since: 0.9.2
 **/
/*
hb_face_t *
hb_ft_face_create_cached (FT_Face ft_face)
{
  if (unlikely (!ft_face->generic.data || ft_face->generic.finalizer != (FT_Generic_Finalizer) hb_ft_face_finalize))
  {
    if (ft_face->generic.finalizer)
      ft_face->generic.finalizer (ft_face);

    ft_face->generic.data = hb_ft_face_create (ft_face, NULL);
    ft_face->generic.finalizer = (FT_Generic_Finalizer) hb_ft_face_finalize;
  }

  return hb_face_reference ((hb_face_t *) ft_face->generic.data);
}
*/


/**
 * hb_ft_font_create:
 * @ft_face: (destroy destroy) (scope notified): 
 * @destroy:
 *
 * 
 *
 * Return value: (transfer full): 
 * Since: 0.9.2
 **/
hb_font_t *
hb_ft_font_create (char*           ft_face,
                   size_t ft_face_size,
		   hb_destroy_func_t destroy)
{
  std::cout << "hb_ft_font_create" << std::endl;

  hb_font_t *font;
  hb_face_t *face;

  face = hb_ft_face_create (ft_face, ft_face_size, destroy);
  font = hb_font_create (face);
  hb_face_destroy (face);

  _hb_ft_font_set_funcs (font, ft_face, false);
  hb_font_set_scale(font, 1, 1);
  /*
  hb_font_set_scale (font,
		     (int) (((uint64_t) ft_face->size->metrics.x_scale * (uint64_t) ft_face->units_per_EM + (1<<15)) >> 16),
		     (int) (((uint64_t) ft_face->size->metrics.y_scale * (uint64_t) ft_face->units_per_EM + (1<<15)) >> 16));
  */

#if 0 /* hb-ft works in no-hinting model */
  hb_font_set_ppem (font,
		    ft_face->size->metrics.x_ppem,
		    ft_face->size->metrics.y_ppem);
#endif

  return font;
}

/**
 * hb_ft_font_create_referenced:
 * @ft_face:
 *
 * 
 *
 * Return value: (transfer full): 
 * Since: 0.9.38
 **/
/*
hb_font_t *
hb_ft_font_create_referenced (FT_Face ft_face)
{
  FT_Reference_Face (ft_face);
  return hb_ft_font_create (ft_face, (hb_destroy_func_t) FT_Done_Face);
}
*/


/* Thread-safe, lock-free, FT_Library */

/*
static FT_Library ft_library;

#ifdef HB_USE_ATEXIT
static
void free_ft_library (void)
{
  FT_Done_FreeType (ft_library);
}
#endif

static FT_Library
get_ft_library (void)
{
retry:
  FT_Library library = (FT_Library) hb_atomic_ptr_get (&ft_library);

  if (unlikely (!library))
  {
    // Not found; allocate one.
    if (FT_Init_FreeType (&library))
      return NULL;

    if (!hb_atomic_ptr_cmpexch (&ft_library, NULL, library)) {
      FT_Done_FreeType (library);
      goto retry;
    }

#ifdef HB_USE_ATEXIT
    atexit (free_ft_library); // First person registers atexit() callback.
#endif
  }

  return library;
}

static void
_release_blob (FT_Face ft_face)
{
  hb_blob_destroy ((hb_blob_t *) ft_face->generic.data);
}

void
hb_ft_font_set_funcs (hb_font_t *font)
{
  std::cout << "hb_ft_font_set_funcs" << std::endl;

  hb_blob_t *blob = hb_face_reference_blob (font->face);
  unsigned int blob_length;
  const char *blob_data = hb_blob_get_data (blob, &blob_length);
  if (unlikely (!blob_length))
    DEBUG_MSG (FT, font, "Font face has empty blob");

  FT_Face ft_face = NULL;
  FT_Error err = FT_New_Memory_Face (get_ft_library (),
				     (const FT_Byte *) blob_data,
				     blob_length,
				     hb_face_get_index (font->face),
				     &ft_face);

  if (unlikely (err)) {
    hb_blob_destroy (blob);
    DEBUG_MSG (FT, font, "Font face FT_New_Memory_Face() failed");
    return;
  }

  FT_Select_Charmap (ft_face, FT_ENCODING_UNICODE);

  FT_Set_Char_Size (ft_face,
		    abs (font->x_scale), abs (font->y_scale),
		    0, 0);
#if 0
		    font->x_ppem * 72 * 64 / font->x_scale,
		    font->y_ppem * 72 * 64 / font->y_scale);
#endif
  if (font->x_scale < 0 || font->y_scale < 0)
  {
    FT_Matrix matrix = { font->x_scale < 0 ? -1 : +1, 0,
			  0, font->y_scale < 0 ? -1 : +1};
    FT_Set_Transform (ft_face, &matrix, NULL);
  }

  ft_face->generic.data = blob;
  ft_face->generic.finalizer = (FT_Generic_Finalizer) _release_blob;

  // TODO: pass char* here
  // _hb_ft_font_set_funcs (font, ft_face, true);
  hb_ft_font_set_load_flags (font, FT_LOAD_DEFAULT | FT_LOAD_NO_HINTING);
}
*/
