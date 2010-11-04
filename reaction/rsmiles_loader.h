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

#ifndef __rsmiles_loader__
#define __rsmiles_loader__

#include "base_cpp/exception.h"

class Scanner;
class ReactionHighlighting;
class BaseReaction;
class Reaction;
class QueryReaction;

class RSmilesLoader
{
public:
   DEF_ERROR("reaction SMILES loader");

   DLLEXPORT RSmilesLoader (Scanner &scanner);

   DLLEXPORT void loadReaction (Reaction &rxn);
   DLLEXPORT void loadQueryReaction (QueryReaction &rxn);

   // see comment in SmilesLoader
   bool ignore_closing_bond_direction_mismatch;
   
   ReactionHighlighting *highlighting;

protected:
   struct _Atom
   {
      int mol_idx;
      int atom_idx;
   };

   int _selectGroup (int& idx, int rcnt, int ccnt, int pcnt) const;
   int _selectGroupByPair (int &lead_idx, int& idx, int rcnt, int ccnt, int pcnt) const;

   Scanner &_scanner;

   BaseReaction  *_brxn;
   QueryReaction *_qrxn;
   Reaction      *_rxn;

   void _loadReaction ();

private:
   RSmilesLoader (const RSmilesLoader &); // no implicit copy
};

#endif
