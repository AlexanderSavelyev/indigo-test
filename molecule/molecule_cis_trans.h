/****************************************************************************
 * Copyright (C) 2009-2011 GGA Software Services LLC
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

#ifndef __molecule_cis_trans__
#define __molecule_cis_trans__

#include "base_cpp/red_black.h"
#include "math/algebra.h"

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable:4251)
#endif

namespace indigo {

class BaseMolecule;
class Filter;

class DLLEXPORT MoleculeCisTrans
{
public:
   enum
   {
      CIS = 1,
      TRANS = 2
   };

   void clear ();
   void clear (BaseMolecule &mol);
   void build (BaseMolecule &mol, int *exclude_bonds);
   void buildFromSmiles (int *dirs);

   bool exists () const;

   int count ();

   void setParity (int bond_idx, int parity);
   int  getParity (int bond_idx) const;
   bool isIgnored (int bond_idx) const;

   void registerBond (int idx);

   void flipBond (BaseMolecule &mol, int atom_parent, int atom_from, int atom_to);

   const int * getSubstituents (int bond_idx) const;
   void getSubstituents_All (int bond_idx, int subst[4]);

   void add (int bond_idx, int substituents[4], int parity);

   int applyMapping (int idx, const int *mapping) const;
   static int applyMapping (int parity, const int *substituents, const int *mapping);

   static int getMappingParitySign (BaseMolecule &query, BaseMolecule &target,
                                    int bond_idx, const int *mapping);

   static bool checkSub (BaseMolecule &query, BaseMolecule &target, const int *mapping);

   void buildOnSubmolecule (BaseMolecule &super, BaseMolecule &sub, int *mapping);

   void restoreSubstituents (BaseMolecule &mol, int bond_idx);

   static bool isAutomorphism (BaseMolecule &mol, const Array<int> &mapping, const Filter *edge_filter = NULL);

   DEF_ERROR("cis-trans");

   static bool isGeomStereoBond (BaseMolecule &mol, int bond_idx, int *substituents, bool have_xyz);
   static int  sameside (const Vec3f &beg, const Vec3f &end, const Vec3f &nei_beg, const Vec3f &nei_end);

protected:

   BaseMolecule & _getMolecule ();
   
   struct _Bond
   {
      int parity; // CIS ot TRANS
      int ignored; // explicitly ignored cis-trans configuration on this bond
      int substituents[4];
   };

   Array<_Bond> _bonds;

   static bool _pureH (BaseMolecule &mol, int idx);
   static int _sameside (BaseMolecule &mol, int i_beg, int i_end, int i_nei_beg, int i_nei_end);
   bool _sortSubstituents (BaseMolecule &mol, int *substituents);
};

}

#ifdef _WIN32
#pragma warning(pop)
#endif

#endif
