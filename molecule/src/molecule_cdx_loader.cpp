/****************************************************************************
 * Copyright (C) 2015 GGA Software Services LLC
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

#include "base_cpp/scanner.h"
#include "molecule/molecule.h"
#include "molecule/elements.h"
#include "molecule/molecule_scaffold_detection.h"
#include "molecule/molecule_cdx_loader.h"

using namespace indigo;

IMPL_ERROR(MoleculeCdxLoader, "molecule CDX loader");

CP_DEF(MoleculeCdxLoader);

MoleculeCdxLoader::MoleculeCdxLoader (Scanner &scanner) :
CP_INIT,
TL_CP_GET(_nodes),
TL_CP_GET(_bonds),
TL_CP_GET(_stereo_care_atoms),
TL_CP_GET(_stereo_care_bonds),
TL_CP_GET(_stereocenter_types),
TL_CP_GET(_stereocenter_groups),
TL_CP_GET(_sensible_bond_directions),
TL_CP_GET(_ignore_cistrans)
{
   _scanner = &scanner;
}

void MoleculeCdxLoader::loadMolecule (Molecule &mol)
{
    mol.clear();
   _nodes.clear();
   _bonds.clear();

   _bmol = &mol;
   _mol = &mol;

   if (_scanner != 0)
   {
     _checkHeader();
     _loadMolecule();
   }
}

void MoleculeCdxLoader::_checkHeader ()
{
   int pos_saved = _scanner->tell();

   if ((_scanner->length() - pos_saved) < 8)
      return;

   char id[8];
   _scanner->readCharsFix(8, id);

   if (strncmp(id, kCDX_HeaderString, kCDX_HeaderStringLen) == 0)
   {
      _scanner->seek(kCDX_HeaderLength - kCDX_HeaderStringLen, SEEK_CUR);
   }
   else
   {
      _scanner->seek(pos_saved, SEEK_SET);
   }
}

void MoleculeCdxLoader::_loadMolecule ()
{
   UINT16 tag;
   UINT16 size;
   UINT32 id;

   int level = 1;

   while (!_scanner->isEOF())
   {
      tag = _scanner->readBinaryWord();

      if (tag & kCDXTag_Object)
      {
         id = _scanner->readBinaryDword();
         if (tag == kCDXObj_Fragment)
         {
            _readFragment(id);
         }
         else if ((tag == kCDXObj_Graphic) ||
                  (tag == kCDXObj_Text) ||
                  (tag == kCDXObj_BracketedGroup) ||
                  (tag == kCDXObj_BracketAttachment) ||
                  (tag == kCDXObj_CrossingBond) ||
                  (tag == kCDXObj_ReactionStep) ||
                  (tag == kCDXObj_Curve) ||
                  (tag == kCDXObj_EmbeddedObject))
         {
            _skipObject();
         }
         else if ((tag == kCDXObj_Page) ||
                  (tag == kCDXObj_Group))
         {
            level++;
         }
         else 
         {
            level++;
         }
      }
      else if (tag == 0)
      {
         level--;
      }
      else
      {
         size = _scanner->readBinaryWord();
         switch (tag)
         {
            case kCDXProp_Name:
              _scanner->seek (size, SEEK_CUR);
              break;
            default:
              _scanner->seek (size, SEEK_CUR);
              break;
         }
      }
      if (level == 0)
         break;
   }

   int idx;
   RedBlackMap<int, int> _atom_mapping;

   for (int i = 0; i < _nodes.size(); i++)
   {
      if (_nodes[i].type == kCDXNodeType_Element)
      {
         idx = _mol->addAtom(_nodes[i].label);
         _mol->setAtomCharge_Silent(idx, _nodes[i].charge);
         _mol->setAtomIsotope(idx, _nodes[i].isotope);
         _mol->setAtomRadical(idx, _nodes[i].radical);
//         _mol->setExplicitValence(idx, _nodes[i].valence);
         _bmol->setAtomXyz(idx, (float)_nodes[i].x*COORD_COEF, (float)_nodes[i].y*COORD_COEF,
                           (float)_nodes[i].z*COORD_COEF);
         _nodes[i].index = idx;
         _atom_mapping.insert(_nodes[i].id, i);
      }
      else if (_nodes[i].type == kCDXNodeType_ExternalConnectionPoint)
      {
         _atom_mapping.insert(_nodes[i].id, i);
      }
      else
      {
         _atom_mapping.insert(_nodes[i].id, i);
      }
   }

   for (int i = 0; i < _bonds.size(); i++)
   {
      if ((_nodes[_atom_mapping.at(_bonds[i].beg)].type == kCDXNodeType_Element) &&
          (_nodes[_atom_mapping.at(_bonds[i].end)].type == kCDXNodeType_Element))
      {
         if (_bonds[i].swap_bond)
            _bonds[i].index = _mol->addBond_Silent(_nodes[_atom_mapping.at(_bonds[i].end)].index,
                                                   _nodes[_atom_mapping.at(_bonds[i].beg)].index, _bonds[i].type);
         else
            _bonds[i].index = _mol->addBond_Silent(_nodes[_atom_mapping.at(_bonds[i].beg)].index,
                                                   _nodes[_atom_mapping.at(_bonds[i].end)].index, _bonds[i].type);

         if (_bonds[i].dir > 0)
            _bmol->setBondDirection(_bonds[i].index, _bonds[i].dir);
      }
      else if (_nodes[_atom_mapping.at(_bonds[i].beg)].type == kCDXNodeType_ExternalConnectionPoint)
      {
      }
      else if (_nodes[_atom_mapping.at(_bonds[i].end)].type == kCDXNodeType_ExternalConnectionPoint)
      {
      }
   }

   _postLoad();
}

void MoleculeCdxLoader::_postLoad ()
{
   _sensible_bond_directions.clear_resize(_bonds.size());
   _sensible_bond_directions.zerofill();
   _ignore_cistrans.clear_resize(_bonds.size());
   _ignore_cistrans.zerofill();

   _bmol->stereocenters.buildFromBonds(stereochemistry_options, _sensible_bond_directions.ptr());
   _bmol->allene_stereo.buildFromBonds(stereochemistry_options.ignore_errors, _sensible_bond_directions.ptr());
   _bmol->cis_trans.build(_ignore_cistrans.ptr());
   _bmol->have_xyz = true;
}

void MoleculeCdxLoader::_readFragment (UINT32 fragment_id)
{
   UINT16 tag;
   UINT16 size;
   UINT32 id;

   int level = 1;

   while (!_scanner->isEOF())
   {
      tag = _scanner->readBinaryWord();

      if (tag & kCDXTag_Object)
      {
         id = _scanner->readBinaryDword();
         if (tag == kCDXObj_Fragment)
         {
            _readFragment(id);
         }
         else if (tag == kCDXObj_Node)
         {
            _readNode(id);
         }
         else if (tag == kCDXObj_Bond)
         {
            _readBond(id);
         }
         else if ((tag == kCDXObj_Graphic) ||
                  (tag == kCDXObj_Text))
         {
            _skipObject();
         }
         else 
         {
           _skipObject();
         }
      }
      else if (tag == 0)
      {
         level--;
      }
      else
      {
         size = _scanner->readBinaryWord();
         _scanner->seek (size, SEEK_CUR);
      }
      if (level == 0)
         return;
   }
}

void MoleculeCdxLoader::_readNode (UINT32 node_id)
{
   UINT16 tag;
   UINT16 size;
   UINT32 id;

   int node_type = 0;

   int level = 1;

   _NodeDesc &node = _nodes.push();
   memset(&node, 0, sizeof(_NodeDesc));
   node.id = node_id;
   node.type = kCDXNodeType_Element;
   node.label = ELEM_C;
   node.hydrogens = -1;
   node.valence = -1;
   node.radical = -1;

   while (!_scanner->isEOF())
   {
      tag = _scanner->readBinaryWord();

      if (tag & kCDXTag_Object)
      {
         id = _scanner->readBinaryDword();
         if (tag == kCDXObj_Fragment)
         {
            _readFragment(id);
         }
         else if (tag == kCDXObj_Group)
         {
         }
         else if ((tag == kCDXObj_Graphic) ||
                  (tag == kCDXObj_Text) ||
                  (tag == kCDXObj_ObjectTag))
         {
            _skipObject();
         }
         else 
         {
           _skipObject();
         }
      }
      else if (tag == 0)
      {
         level--;
      }
      else
      {
         size = _scanner->readBinaryWord();
         switch (tag)
         {
            case kCDXProp_Atom_NumHydrogens:
              node.hydrogens = _scanner->readBinaryWord();
              break;
            case kCDXProp_2DPosition:
              _read2DPosition(node.x, node.y);
              break;
            case kCDXProp_3DPosition:
              _read3DPosition(node.x, node.y, node.z);
              break;
            case kCDXProp_Node_Element:
              node.label = _getElement();
              break;
            case kCDXProp_Atom_Charge:
              node.charge = _getCharge(size);
              break;
            case kCDXProp_Node_Type:
              node.type = _scanner->readBinaryWord();
              break;
            case kCDXProp_Atom_Isotope:
              node.isotope = _scanner->readBinaryWord();;
              break;
            case kCDXProp_Atom_Radical:
              node.radical = _getRadical();
              break;
//            case kCDXProp_Atom_EnhancedStereoType:
//              node.enchanced_stereo = _scanner->readByte();
//            case kCDXProp_Atom_EnhancedStereoGroupNum:
//              node.stereo_group = _scanner->readBinaryWord();
            case kCDXProp_Atom_CIPStereochemistry:
              node.stereo = _scanner->readByte();
              break;
            case kCDXProp_Atom_ElementList:
            case kCDXProp_Atom_AbnormalValence:
            case kCDXProp_Name:
            case kCDXProp_IgnoreWarnings:
            case kCDXProp_ChemicalWarning:
              _scanner->seek (size, SEEK_CUR);
              break; 
            case kCDXProp_MarginWidth:
            case kCDXProp_ZOrder:
            case kCDXProp_Atom_GenericNickname:
            case kCDXProp_Atom_Geometry:
            case kCDXProp_Atom_BondOrdering:
            case kCDXProp_Node_LabelDisplay:
            case kCDXProp_LabelStyle:
            case kCDXProp_ForegroundColor:
            case kCDXProp_BackgroundColor:
              _scanner->seek (size, SEEK_CUR);
              break;
            default:
              _scanner->seek (size, SEEK_CUR);
              break;
         }
      }
      if (level == 0)
      {
         switch (node.type)
         {
            case kCDXNodeType_Fragment:
              break;
            case kCDXNodeType_ExternalConnectionPoint:
              break;
            case kCDXNodeType_Element:
              break;
            default:
              break;
         }
         return;
      }
   }
}

void MoleculeCdxLoader::_read2DPosition (int &x, int &y)
{
   y = _scanner->readBinaryDword();
   x = _scanner->readBinaryDword();
}

void MoleculeCdxLoader::_read3DPosition (int &x, int &y, int &z)
{
   z = _scanner->readBinaryDword();
   y = _scanner->readBinaryDword();
   x = _scanner->readBinaryDword();
}

int MoleculeCdxLoader::_getElement ()
{
   return _scanner->readBinaryWord();
}

int MoleculeCdxLoader::_getCharge (int size)
{
   if (size == 4)
      return _scanner->readBinaryDword();
   else
      return _scanner->readByte();
}

int MoleculeCdxLoader::_getRadical ()
{
   return _scanner->readByte();
}

void MoleculeCdxLoader::_readBond (UINT32 bond_id)
{
   UINT16 tag;
   UINT16 size;
   UINT32 id;

   int level = 1;

   _BondDesc &bond = _bonds.push();
   memset(&bond, 0, sizeof(_BondDesc));
   bond.type = BOND_SINGLE;

   while (!_scanner->isEOF())
   {
      tag = _scanner->readBinaryWord();

      if (tag & kCDXTag_Object)
      {
         id = _scanner->readBinaryDword();
         _skipObject();
      }
      else if (tag == 0)
      {
         level--;
      }
      else
      {
         size = _scanner->readBinaryWord();
         switch (tag)
         {
            case kCDXProp_Bond_Begin:
              bond.beg = _scanner->readBinaryDword();
              break;
            case kCDXProp_Bond_End:
              bond.end = _scanner->readBinaryDword();
              break;
            case kCDXProp_Bond_Order:
              bond.type = _getBondType();
              break;
            case kCDXProp_Bond_Display:
              bond.dir = _getBondDirection(bond.swap_bond);
              break;
            case kCDXProp_BondLength:
            case kCDXProp_Bond_CIPStereochemistry:
              bond.stereo = _scanner->readByte();
              break;
            case kCDXProp_Bond_BeginAttach:
            case kCDXProp_Bond_EndAttach:
            case kCDXProp_ChemicalWarning:
              _scanner->seek (size, SEEK_CUR);
              break;
            default:
              _scanner->seek (size, SEEK_CUR);
              break;
         }
      }
      if (level == 0)
      {
         return;
      }
   }
}

int MoleculeCdxLoader::_getBondType ()
{
   UINT16 order;
   int type = BOND_SINGLE;

   order = _scanner->readBinaryWord();
   switch (order)
   {
      case kCDXBondOrder_Single:
         type = BOND_SINGLE;
         break;
      case kCDXBondOrder_Double:
         type = BOND_DOUBLE;
         break;
      case kCDXBondOrder_Triple:
         type = BOND_TRIPLE;
         break;
      case kCDXBondOrder_OneHalf: 
         type = BOND_AROMATIC;
         break;
      default:
         type = BOND_SINGLE;
         break;
   }
   return type;
}

int MoleculeCdxLoader::_getBondDirection (bool &swap_bond)
{
   UINT16 display;
   int direction = 0;

   display = _scanner->readBinaryWord();
   switch (display)
   {
      case kCDXBondDisplay_WedgedHashBegin:
         direction = BOND_DOWN;
         swap_bond = false;
         break;
      case kCDXBondDisplay_WedgedHashEnd:
         direction = BOND_DOWN;
         swap_bond = true;
         break;
      case kCDXBondDisplay_WedgeBegin:
         direction = BOND_UP;
         swap_bond = false;
         break;
      case kCDXBondDisplay_WedgeEnd:
         direction = BOND_UP;
         swap_bond = true;
         break;
      case kCDXBondDisplay_Wavy:
         direction = BOND_EITHER;
         break;
      default:
         direction = 0;
         break;
   }
   return direction;
}

void MoleculeCdxLoader::_skipObject ()
{
   UINT16 tag;
   UINT16 size;
   UINT32 id;

   int level = 1;

   while (!_scanner->isEOF())
   {
      tag = _scanner->readBinaryWord();

      if (tag & kCDXTag_Object)
      {
         id = _scanner->readBinaryDword();
         _skipObject();
      }
      else if (tag == 0)
      {
         level--;
      }
      else
      {
         size = _scanner->readBinaryWord();
         _scanner->seek (size, SEEK_CUR);
      }
      if (level == 0)
         return;
   }
}
