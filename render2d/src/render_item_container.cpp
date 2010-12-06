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

#include "molecule/molecule.h"
#include "molecule/query_molecule.h"
#include "base_cpp/output.h"
#include "layout/metalayout.h"
#include "render_context.h"
#include "render_internal.h"
#include "render_item_container.h"

using namespace indigo;

RenderItemContainer::RenderItemContainer (RenderContext& rc) : 
   RenderItemBase(rc)
{
}

void RenderItemContainer::estimateSize ()
{
   for (int i = 0; i < _items.size(); ++i) {
      _items[i]->estimateSize();
   }                                        
}

void RenderItemContainer::setObjScale (float scale)
{
   for (int i = 0; i < _items.size(); ++i) {
      _items[i]->setObjScale(scale);
   }                                        
}

double RenderItemContainer::getTotalBondLength ()
{
   double sum = 0.0;
   for (int i = 0; i < _items.size(); ++i) {
      sum += _items[i]->getTotalBondLength();
   }                                        
   return sum;
}

double RenderItemContainer::getTotalClosestAtomDistance()
{
   double sum = 0.0;
   for (int i = 0; i < _items.size(); ++i) {
      sum += _items[i]->getTotalClosestAtomDistance();
   }
   return sum;
}

int RenderItemContainer::getBondCount ()
{
   int sum = 0;
   for (int i = 0; i < _items.size(); ++i) {
      sum += _items[i]->getBondCount();
   }
   return sum;
}

int RenderItemContainer::getAtomCount ()
{
   int sum = 0;
   for (int i = 0; i < _items.size(); ++i) {
      sum += _items[i]->getAtomCount();
   }
   return sum;
}