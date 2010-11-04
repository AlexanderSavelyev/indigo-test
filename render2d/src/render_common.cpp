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
#include "molecule/molecule.h"
#include "molecule/query_molecule.h"
#include "reaction/reaction.h"
#include "reaction/query_reaction.h"
#include "render_common.h"

// cos(a) to cos(a/2) 
double cos2c (const double cs)
{
   return sqrt((1 + cs)/2);
}

// cos(a) to sin(a/2) 
double sin2c (const double cs)
{
   return sqrt((1 - cs)/2);
}

// cos(a) to tg(a/2) 
double tg2c (const double cs)
{
   return sqrt((1 - cs) / (1 + cs));
}

// cos(a) to ctg(a/2) 
double ctg2c (const double cs)
{
   return sqrt((1 + cs) / (1 - cs));
}


RenderItem::RenderItem()
{
   clear();
}
void RenderItem::clear() 
{
   ritype = RIT_NULL;
   bbp.set(0, 0);
   bbsz.set(0, 0);
   relpos.set(0, 0);
   color = CWC_BASE;
   highlighted = false;
}

void TextItem::clear() { text.clear(); }

void GraphItem::clear() {}
          
AtomDesc::AtomDesc() 
{ 
   clear(); 
}

void AtomDesc::clear ()
{
   showLabel = showHydro = true;
   tibegin = gibegin = -1;
   ticount = gicount = 0;
   stereoGroupType = 
      stereoGroupNumber = -1;
   isRGroupAttachmentPoint = false;
   fixed = false;
   color = CWC_BASE;
}

BondEnd::BondEnd () 
{
   clear();
}

void BondEnd::clear ()
{    
   lRing = next = -1;
   centered = false;
   prolong = false;
   lang = (float)(2 * M_PI);
   rang = (float)(2 * M_PI);
   rcos = lcos = -1;
   rsin = lsin = 0;
   rnei = lnei = -1;
   offset = 0;
   width = 0;
}

BondDescr::BondDescr ()
{           
   clear();
}

void BondDescr::clear ()
{              
   type = -1; 
   queryType = -1;
   inRing = false;
   aromRing = false;
   stereoCare = false;
   thickness = 0.0f;
   stereodir = 0;
   centered = false;
   extP = extN = 0;  
   bahs = eahs = 0;
   tiTopology = -1;
   topology = 0;
}

int BondDescr::getBondEnd (int aid) const
{
   if (aid == beg)
      return be1;
   if (aid == end)
      return be2;
   throw Error("atom given is not adjacent to the bond");
}

Ring::Ring () 
{ 
   clear(); 
}

void Ring::clear ()
{
   bondEnds.clear();
   angles.clear();
   dblBondCount = 0;
   aromatic = true;
   center.set(0, 0);
   radius = 0;
}

MoleculeRenderData::MoleculeRenderData ()
{
   clear();
}

void MoleculeRenderData::clear ()
{
   atoms.clear();
   bonds.clear();
   bondends.clear();
   graphitems.clear();
   rings.clear();
   textitems.clear();
   aam.clear();
   reactingCenters.clear();
   inversions.clear();
   exactChanges.clear();
}

RenderSettings::RenderSettings () :
TL_CP_GET(bondDashAromatic), 
TL_CP_GET(bondDashAny), 
TL_CP_GET(bondDashSingleOrAromatic), 
TL_CP_GET(bondDashDoubleOrAromatic)
{  
   init(1.0f);
}

void RenderSettings::init (float sf)
{
   bondLineWidth = sf / 30;
   bondSpace = 2.5f * bondLineWidth;

   fzz[FONT_SIZE_LABEL] = bondLineWidth * 12;
   fzz[FONT_SIZE_ATTR] = bondLineWidth * 8;
   fzz[FONT_SIZE_RGROUP_LOGIC] = bondLineWidth * 12;
   fzz[FONT_SIZE_RGROUP_LOGIC_INDEX] = bondLineWidth * 8;
   fzz[FONT_SIZE_INDICES] = bondLineWidth * 6;   

   upperIndexShift = -0.4f;
   lowerIndexShift = 0.4f;
   boundExtent = 1.3f * bondLineWidth;
   labelInternalOffset = bondLineWidth;         
   stereoGroupLabelOffset = 2 * bondLineWidth;
   radicalRightOffset = bondLineWidth / 2;
   radicalRightVertShift = -0.2f;
   radicalTopOffset = 0.8f * bondLineWidth;
   radicalTopDistDot = bondLineWidth;
   radicalTopDistCap = bondLineWidth / 2;
   radicalTopDistCap = bondLineWidth / 2;
   dashUnit = bondLineWidth;
   eps = 1e-4f;
   cosineTreshold = 0.98f;
   prolongAdjSinTreshold = 0.2f;
   stereoCareBoxSize = bondSpace * 3 + bondLineWidth * 3;
   minBondLength = bondLineWidth * 5;

   graphItemDotRadius = bondLineWidth;
   graphItemCapSlope = 2;
   graphItemCapBase = 0.7f * bondLineWidth;
   graphItemCapWidth = 1.2f * bondLineWidth;
   graphItemDigitWidth = 4.5f * bondLineWidth;
   graphItemDigitHeight = 5.5f * bondLineWidth;
   graphItemSignLineWidth = 0.8f * bondLineWidth;
   graphItemPlusEdge = (graphItemDigitWidth - graphItemSignLineWidth) / 2;

   const int dashDot[] = {5,2,1,2};
   const int dash[] = {3,2}; 

   bondDashSingleOrAromatic.clear();
   bondDashDoubleOrAromatic.clear();
   bondDashAny.clear();
   bondDashAromatic.clear();
   for (int i = 0; i < NELEM(dashDot); ++i)
   {
      double val = dashDot[i] * dashUnit;
      bondDashSingleOrAromatic.push(val);
      bondDashDoubleOrAromatic.push(val);
   }
   for (int i = 0; i < NELEM(dash); ++i)
   {
      double val = dash[i] * dashUnit;
      bondDashAny.push(val);
      bondDashAromatic.push(val);
   }

   fontPathRegular = "fonts/dejavu-2.30/DejaVuSans.ttf"; 
   fontPathBold = "fonts/dejavu-2.30/DejaVuSans-Bold.ttf";   
   
   layoutMarginHorizontal = 1;
   layoutMarginVertical = 1;
   plusSize = 0.5;
   metaLineWidth = 1.0 / 16;
   arrowLength = 3 * plusSize;
   arrowHeadWidth = plusSize / 2;
   arrowHeadSize = plusSize / 2;
   equalityInterval = plusSize / 2;
   rGroupIfThenInterval = bondLineWidth * 4;
}

CanvasOptions::CanvasOptions ()
{
   clear();
}

void CanvasOptions::clear ()
{
   width = height = -1;
   xOffset = yOffset = 0;
   bondLength = 100;
   marginX = marginY = 0;
   commentMarginX = commentMarginY = 0;
}

RenderContextOptions::RenderContextOptions ()
{
   clear();
}

void RenderContextOptions::clear()
{
   aamColor.set(0, 0, 0);
   commentFontFactor = 20;
}

HighlightingOptions::HighlightingOptions ()
{
   clear();
}

void HighlightingOptions::clear()
{
   highlightThicknessEnable = false;
   highlightThicknessFactor = 1.8f;
   highlightColorEnable = true;
   highlightColor.set(1, 0, 0);
}
