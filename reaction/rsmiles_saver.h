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

#ifndef __rsmiles_saver__
#define __rsmiles_saver__

#include "base_cpp/exception.h"
#include "base_cpp/obj_array.h"
#include "base_cpp/tlscont.h"
#include "reaction.h"

class Output;
class BaseReaction;
class QueryReaction;
class Reaction;
class ReactionHighlighting;

class RSmilesSaver
{
public:
   DEF_ERROR("reaction SMILES saver");

   RSmilesSaver (Output &output);

   void saveReaction (Reaction &reaction);
   void saveQueryReaction (QueryReaction &reaction);

   ReactionHighlighting *highlighting;

protected:
   BaseReaction  *_brxn;
   QueryReaction *_qrxn;
   Reaction      *_rxn;

   void _saveReaction ();

   struct _Idx
   {
      int mol;
      int idx;
   };

   Output &_output;

   TL_CP_DECL(Array<_Idx>, _written_atoms);
   TL_CP_DECL(Array<_Idx>, _written_bonds);
   TL_CP_DECL(Array<int>, _ncomp);

   void _writeFragmentsInfo ();
   void _writeStereogroups ();
   void _writeRadicals ();
   void _writePseudoAtoms ();
   void _writeHighlighting ();

   bool _comma;

private:
   RSmilesSaver (const RSmilesSaver &); // no implicit copy
};

#endif
