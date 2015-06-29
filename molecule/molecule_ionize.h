/****************************************************************************
 * Copyright (C) 2009-2015 EPAM Systems
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

#ifndef __molecule_ionize_h__
#define __molecule_ionize_h__

#include "base_cpp/tlscont.h"
#include "base_cpp/obj_array.h"

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable:4251)
#endif

namespace indigo {

class Molecule;
class QueryMolecule;

class IonizeOptions
{
};

class DLLEXPORT MoleculeIonizer
{
public:
   MoleculeIonizer ();

   static bool ionize (Molecule &molecule, float ph, float ph_toll, const IonizeOptions &options);

   DECL_ERROR;
   CP_DECL;

protected:
   static void _loadPkaModel (const IonizeOptions &options, ObjArray<QueryMolecule> &acids, Array<float> &a_pkas,
                      ObjArray<QueryMolecule> &basics, Array<float> &b_pkas);
   static void _estimate_pKa (Molecule &mol, const IonizeOptions &options, Array<int> &acid_sites,
                      Array<int> &basic_sites, Array<float> &acid_pkas, Array<float> &basic_pkas);
   static void _setCharges (Molecule &mol, float ph, float ph_toll, const IonizeOptions &options, Array<int> &acid_sites,
                      Array<int> &basic_sites, Array<float> &acid_pkas, Array<float> &basic_pkas);
};

}

#ifdef _WIN32
#pragma warning(pop)
#endif

#endif
