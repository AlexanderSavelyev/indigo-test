/****************************************************************************
 * Copyright (C) 2010 GGA Software Services LLC
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

#include "indigo_internal.h"
#include "base_cpp/output.h"
#include "base_cpp/scanner.h"
#include "molecule/sdf_loader.h"
#include "molecule/rdf_loader.h"
#include "reaction/reaction.h"

IndigoObject::IndigoObject (int type_)
{
   type = type_;
   
   ArrayOutput out(_dbg_info);
   out.printf("<type %d>", type);
   out.writeChar(0);
}

IndigoObject::~IndigoObject ()
{
}

const char * IndigoObject::debugInfo ()
{
   return _dbg_info.ptr();
}

void IndigoObject::toString (Array<char> &str)
{
   throw IndigoError("can not convert %s to string", debugInfo());
}

void IndigoObject::toBuffer (Array<char> &buf)
{
   return toString(buf);
}


Molecule & IndigoObject::getMolecule ()
{
   throw IndigoError("%s is not a molecule", debugInfo());
}

BaseMolecule & IndigoObject::getBaseMolecule ()
{
   throw IndigoError("%s is not a base molecule", debugInfo());
}

QueryMolecule & IndigoObject::getQueryMolecule ()
{
   throw IndigoError("%s is not a query molecule", debugInfo());
}

GraphHighlighting * IndigoObject::getMoleculeHighlighting ()
{
   throw IndigoError("%s does not have a molecule highlighting", debugInfo());
}

RedBlackStringObjMap< Array<char> > * IndigoObject::getProperties ()
{
   throw IndigoError("%s can not have properties", debugInfo());
}

void IndigoObject::copyProperties (RedBlackStringObjMap< Array<char> > &other)
{
   RedBlackStringObjMap< Array<char> > *props = getProperties();

   if (props == 0)
      throw IndigoError("copyProperties(): zero destination");

   int i;

   props->clear();

   for (i = other.begin(); i != other.end(); i = other.next(i))
      props->value(props->insert(other.key(i))).copy(other.value(i));
}

Reaction & IndigoObject::getReaction ()
{
   throw IndigoError("%s is not a reaction", debugInfo());
}

BaseReaction & IndigoObject::getBaseReaction ()
{
   throw IndigoError("%s is not a query reaction", debugInfo());
}

QueryReaction & IndigoObject::getQueryReaction ()
{
   throw IndigoError("%s is not a query reaction", debugInfo());
}

ReactionHighlighting * IndigoObject::getReactionHighlighting()
{
   throw IndigoError("%s does not have a reaction highlighting", debugInfo());
}

Scanner & IndigoObject::getScanner ()
{
   if (type == SCANNER)
      return *((IndigoScanner *)this)->ptr;
   throw IndigoError("%s is not a scanner", debugInfo());
}

Output & IndigoObject::getOutput ()
{
   if (type == OUTPUT)
      return *((IndigoOutput *)this)->ptr;
   throw IndigoError("%s is not an output", debugInfo());
}

IndigoObject * IndigoObject::next ()
{
   throw IndigoError("%s is not iterable", debugInfo());
}

bool IndigoObject::hasNext ()
{
   throw IndigoError("%s is not iterable", debugInfo());
}

const char * IndigoObject::getName ()
{
   throw IndigoError("%s does not have a name", debugInfo());
}

IndigoAtom & IndigoObject::getAtom ()
{
   throw IndigoError("%s does not represent an atom", debugInfo());
}

int IndigoObject::getIndex ()
{
   throw IndigoError("%s does not have an index", debugInfo());
}

IndigoRGroup & IndigoObject::getRGroup ()
{
   throw IndigoError("%s does not represent an r-rgroup", debugInfo());
}

bool IndigoObject::isBaseMolecule ()
{
   if (type == MOLECULE || type == QUERY_MOLECULE ||
       type == REACTION_MOLECULE || type == SCAFFOLD || 
       type == RGROUP_FRAGMENT || type == RDF_MOLECULE || type == SMILES_MOLECULE)
      return true;
   
   return false;
}

bool IndigoObject::isBaseReaction ()
{
   if (type == REACTION || type == QUERY_REACTION || type == RDF_REACTION || type == SMILES_REACTION)
      return true;

   return false;
}

bool IndigoObject::isAtom ()
{
   return type == ATOM || type == ATOMS_ITER;
}

IndigoArray & IndigoObject::asArray ()
{
   throw IndigoError("%s is not an array", debugInfo());
}

IndigoObject * IndigoObject::clone ()
{
   throw IndigoError("%s is not cloneable", debugInfo());
}

IndigoFingerprint & IndigoObject::asFingerprint()
{
   throw IndigoError("%s is not a fingerprint", debugInfo());
}

IndigoBond & IndigoObject::asBond ()
{
   if (type == BOND)
      return *(IndigoBond *)this;
   if (type == ARRAY_ELEMENT)
   {
      IndigoArrayElement &ae = *(IndigoArrayElement *)this;

      return ae.array->objects[ae.idx]->asBond();
   }
   throw IndigoError("%s is not a bond", debugInfo());
}
