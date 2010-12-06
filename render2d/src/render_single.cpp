/****************************************************************************
 * Copyright (C) 2009-2010 GGA Software Services LLC
 * 
 * This file is part of Indigo toolkit.
 * 
 * This file may be distributed and/or modified under the terms of the
 * GNU General Public License version 3 as published by the Free Software
 * Foundation and appearing in the file LICENSE.GPL included in the
 * packaging of this file.
 * 
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 ***************************************************************************/

#include "math/algebra.h"
#include "base_cpp/array.h"
#include "base_cpp/obj_array.h"
#include "base_cpp/output.h"
#include "graph/graph_highlighting.h"
#include "base_cpp/reusable_obj_array.h"
#include "layout/metalayout.h"
#include "molecule/molecule.h"
#include "molecule/query_molecule.h"
#include "reaction/reaction.h"
#include "reaction/query_reaction.h"
#include "render_context.h"
#include "render_item.h"
#include "render_item_factory.h"
#include "render_single.h"

using namespace indigo;

RenderSingle::RenderSingle (RenderContext& rc, RenderItemFactory& factory) : Render(rc, factory)
{}

RenderSingle::~RenderSingle()
{}

void RenderSingle::_drawObj ()
{
   _rc.storeTransform();
   {
      _rc.translate((objArea.x - objSize.x * scale) / 2, (objArea.y - objSize.y * scale) / 2);
      _rc.scale(scale);
      _factory.getItem(obj).render();
   }
   _rc.restoreTransform();
   _rc.removeStoredTransform();
   _rc.translate(0, objArea.y);
}

void RenderSingle::_drawComment ()
{
   if (comment < 0)
      return;
   _rc.storeTransform();
   {
      float diff = (float)(_cnvOpt.width - 2 * outerMargin.x - commentSize.x);
      switch (_opt.commentAlign) {
         case ALIGNMENT_LEFT:
            break;
         case ALIGNMENT_CENTER:
            _rc.translate(0.5f * diff, 0);
            break;
         case ALIGNMENT_RIGHT:
            _rc.translate(diff, 0);
            break;
         default:
            throw Error("Alignment value invalid");
      }     
      _factory.getItem(comment).render();
   }
   _rc.restoreTransform();
   _rc.removeStoredTransform();
   _rc.translate(0, commentSize.y);
}

void RenderSingle::draw ()
{     
   _rc.initMetaSurface();
   _rc.fontsClear();

   _factory.getItem(obj).init();

   float objScale = _getObjScale(obj);
   _factory.getItem(obj).setObjScale(objScale);
   _factory.getItem(obj).estimateSize();
   objSize.copy(_factory.getItem(obj).size);

   commentSize.set(0,0);
   commentOffset = 0;
   if (comment >= 0) {
      _factory.getItem(comment).init();
      _factory.getItem(comment).estimateSize();
      commentSize.copy(_factory.getItem(comment).size);
      commentOffset = _cnvOpt.commentOffset;
   }
   outerMargin.x = (float)(minMarg + _cnvOpt.commentMarginX);
   outerMargin.y = (float)(minMarg + _cnvOpt.commentMarginY);
   
   scale = _getScale();
   _rc.initContext(_cnvOpt.width, _cnvOpt.height);
   objArea.set((float)_cnvOpt.width, (float)_cnvOpt.height);
   objArea.sub(outerMargin);
   objArea.y -= commentSize.y + commentOffset;
   _rc.init();
   _rc.translate((float)outerMargin.x, (float)outerMargin.y);
   if (_cnvOpt.xOffset > 0 || _cnvOpt.yOffset > 0)
      _rc.translate((float)_cnvOpt.xOffset, (float)_cnvOpt.yOffset);
   _rc.storeTransform();
   {
      if (_opt.commentPos == COMMENT_POS_TOP) {
         _drawComment();
         _rc.translate(0, commentOffset);
         _drawObj();
      } else {
         _drawObj();
         _rc.translate(0, commentOffset);
        _drawComment();
      }
   }
   _rc.resetTransform();
   _rc.removeStoredTransform();
   _rc.destroyMetaSurface();
}

float RenderSingle::_getScale ()
{
   int maxPageSize = _rc.getMaxPageSize();
   float s;
   if (_cnvOpt.width <= 0 || _cnvOpt.height <= 0)
   {
      s = _cnvOpt.bondLength;

      _cnvOpt.width = (int)ceil(__max(objSize.x * s, commentSize.x) + outerMargin.x * 2);
      _cnvOpt.height = (int)ceil(objSize.y * s + commentOffset + commentSize.y + outerMargin.y * 2);

      if (maxPageSize < 0 || __max(_cnvOpt.width, _cnvOpt.height) < maxPageSize)
         return s;
      _cnvOpt.width = __min(_cnvOpt.width, maxPageSize);
      _cnvOpt.height = __min(_cnvOpt.height, maxPageSize);
   }

   float x = _cnvOpt.width - 2 * outerMargin.x,
      y = _cnvOpt.height - (commentSize.y + 2 * outerMargin.y + commentOffset);
   if (x < 1 || y < 1)
      throw Error("Image too small, the layout requires at least %dx%d", 
         2 * outerMargin.x + 1, 
         commentSize.y + 2 * outerMargin.y + commentOffset + 1);
   if (x * objSize.y < y * objSize.x)
      s = x / objSize.x;
   else
      s = y / objSize.y;
   return s;
}