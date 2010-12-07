#include "math/algebra.h"
#include "base_cpp/array.h"
#include "base_cpp/obj_array.h"
#include "base_cpp/output.h"
#include "molecule/molecule.h"
#include "reaction/reaction.h"
#include "render_context.h"

#ifdef _WIN32
#include <windows.h>
#include <cairo-win32.h>
#endif 

using namespace indigo;

void RenderContext::cairoCheckStatus () const
{
#ifdef DEBUG
   cairo_status_t s;
   if (_cr) {
      s = cairo_status(_cr);
      if (s != CAIRO_STATUS_SUCCESS /*&& s <= CAIRO_STATUS_INVALID_WEIGHT*/)
         throw Error("Cairo error: %i -- %s\n", s, cairo_status_to_string(s));
   }
#endif
}

void RenderContext::fontsClear()
{
   memset(_scaled_fonts, 0, FONT_SIZE_COUNT * 2 * sizeof(cairo_scaled_font_t*));

   cairoFontFaceRegular = NULL;
   cairoFontFaceBold = NULL;
   fontOptions = NULL;

   cairo_matrix_init_identity(&fontCtm);
   cairoCheckStatus();
   cairo_matrix_init_identity(&fontScale);
   cairoCheckStatus();
}

void RenderContext::fontsInit()
{
   fontOptions = cairo_font_options_create();
   cairoCheckStatus();
   cairo_font_options_set_antialias(fontOptions, CAIRO_ANTIALIAS_GRAY);
   cairoCheckStatus();
   cairo_set_font_options(_cr, fontOptions);
   cairoCheckStatus();
}

void RenderContext::fontsDispose()
{
   for (int i = 0; i < FONT_SIZE_COUNT * 2; ++i) {
      if (_scaled_fonts[i] != NULL) {
         cairo_scaled_font_destroy(_scaled_fonts[i]);
         cairoCheckStatus();
      }
   }
   if (cairoFontFaceRegular != NULL) {
      cairo_font_face_destroy(cairoFontFaceRegular);
      cairoCheckStatus();
   }
   if (cairoFontFaceBold != NULL) {
      cairo_font_face_destroy(cairoFontFaceBold);
      cairoCheckStatus();
   }

   if (fontOptions != NULL) {
      cairo_font_options_destroy(fontOptions);
      cairoCheckStatus();
   }
   fontsClear();
}

void RenderContext::fontsSetFont(cairo_t* cr, FONT_SIZE size, bool bold)
{
   cairo_select_font_face(cr, "Arial", CAIRO_FONT_SLANT_NORMAL, bold ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL);
   cairoCheckStatus();
   if (size == FONT_SIZE_COMMENT)
      cairo_set_font_size(cr, _rcOpt->commentFontFactor);
   else if (size == FONT_SIZE_TITLE)
      cairo_set_font_size(cr, _rcOpt->titleFontFactor);
   else
      cairo_set_font_size(cr, _settings.fzz[size]);
   cairoCheckStatus();
}

void RenderContext::fontsGetTextExtents(cairo_t* cr, const char* text, int size, float& dx, float& dy, float& rx, float& ry)
{
   cairo_text_extents_t te;
   _tlock.lock();
   cairo_text_extents(cr, text, &te);
   _tlock.unlock();
   cairoCheckStatus();

   dx = (float)te.width;
   dy = (float)te.height;
   rx = (float)-te.x_bearing;
   ry = (float)-te.y_bearing;
}

void RenderContext::fontsDrawText(const TextItem& ti, const Vec3f& color, bool bold)
{
   setSingleSource(color);
   moveTo(ti.bbp);
   moveToRel(ti.relpos);
   _tlock.lock();
   cairo_text_path(_cr, ti.text.ptr());
   bbIncludePath(false);
   _tlock.unlock();
   cairo_new_path(_cr);
   moveTo(ti.bbp);
   moveToRel(ti.relpos);

   if (metafileFontsToCurves) { // TODO: remove
      _tlock.lock();
      cairo_text_path(_cr, ti.text.ptr());
      _tlock.unlock();
      cairoCheckStatus();
      cairo_fill(_cr);
      cairoCheckStatus();
   } else {
      _tlock.lock();
      cairo_show_text(_cr, ti.text.ptr());
      _tlock.unlock();
      cairoCheckStatus();
   }
}