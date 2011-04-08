/****************************************************************************
 * Copyright (C) 2010-2011 GGA Software Services LLC
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

#include "indigo_io.h"
#include "indigo_molecule.h"
#include "indigo_reaction.h"
#include "reaction/reaction_auto_loader.h"
#include "reaction/rxnfile_saver.h"
#include "base_cpp/output.h"
#include "reaction/reaction_automapper.h"
#include "base_cpp/auto_ptr.h"
#include "indigo_array.h"
#include "reaction/rsmiles_loader.h"

IndigoBaseReaction::IndigoBaseReaction (int type_) : IndigoObject(type_)
{
}

IndigoBaseReaction::~IndigoBaseReaction ()
{
}

bool IndigoBaseReaction::is (IndigoObject &obj)
{
   int type = obj.type;

   if (type == REACTION || type == QUERY_REACTION || type == RDF_REACTION || type == SMILES_REACTION)
      return true;

   if (type == ARRAY_ELEMENT)
      return is(((IndigoArrayElement &)obj).get());

   return false;
}



RedBlackStringObjMap< Array<char> > * IndigoBaseReaction::getProperties ()
{
   return &properties;
}

IndigoReaction::IndigoReaction () : IndigoBaseReaction(REACTION)
{
}

IndigoReaction::~IndigoReaction ()
{
}

Reaction & IndigoReaction::getReaction ()
{
   return rxn;
}

BaseReaction & IndigoReaction::getBaseReaction ()
{
   return rxn;
}

const char * IndigoReaction::getName ()
{
   if (rxn.name.ptr() == 0)
      return "";
   return rxn.name.ptr();
}

IndigoQueryReaction::IndigoQueryReaction () : IndigoBaseReaction(QUERY_REACTION)
{
}

IndigoQueryReaction::~IndigoQueryReaction ()
{
}

BaseReaction & IndigoQueryReaction::getBaseReaction ()
{
   return rxn;
}

QueryReaction & IndigoQueryReaction::getQueryReaction ()
{
   return rxn;
}

const char * IndigoQueryReaction::getName ()
{
   if (rxn.name.ptr() == 0)
      return "";
   return rxn.name.ptr();
}

IndigoReactionMolecule::IndigoReactionMolecule (BaseReaction &reaction, int index) :
IndigoObject(REACTION_MOLECULE),
rxn(reaction),
idx(index)
{
}

IndigoReactionMolecule::~IndigoReactionMolecule ()
{
}

BaseMolecule & IndigoReactionMolecule::getBaseMolecule ()
{
   return rxn.getBaseMolecule(idx);
}

Molecule & IndigoReactionMolecule::getMolecule ()
{
   return rxn.getBaseMolecule(idx).asMolecule();
}

QueryMolecule & IndigoReactionMolecule::getQueryMolecule ()
{
   return rxn.getBaseMolecule(idx).asQueryMolecule();
}

int IndigoReactionMolecule::getIndex ()
{
   return idx;
}

IndigoObject * IndigoReactionMolecule::clone ()
{
   if (rxn.isQueryReaction())
      return IndigoQueryMolecule::cloneFrom(*this);
   else
      return IndigoMolecule::cloneFrom(*this); 
}

RedBlackStringObjMap< Array<char> > * IndigoReactionMolecule::getProperties ()
{
   return 0;
}

void IndigoReactionMolecule::remove ()
{
   rxn.remove(idx);
}

IndigoReactionIter::IndigoReactionIter (BaseReaction &rxn, int subtype) :
IndigoObject(REACTION_ITER),
_rxn(rxn)
{
   _subtype = subtype;
   _idx = -1;
}

IndigoReactionIter::~IndigoReactionIter ()
{
}

int IndigoReactionIter::_begin ()
{
   if (_subtype == REACTANTS)
      return _rxn.reactantBegin();
   if (_subtype == PRODUCTS)
      return _rxn.productBegin();
   if (_subtype == CATALYSTS)
      return _rxn.catalystBegin();

   return _rxn.begin();
}

int IndigoReactionIter::_end ()
{
   if (_subtype == REACTANTS)
      return _rxn.reactantEnd();
   if (_subtype == PRODUCTS)
      return _rxn.productEnd();
   if (_subtype == CATALYSTS)
      return _rxn.catalystEnd();

   return _rxn.end();
}

int IndigoReactionIter::_next (int i)
{
   if (_subtype == REACTANTS)
      return _rxn.reactantNext(i);
   if (_subtype == PRODUCTS)
      return _rxn.productNext(i);
   if (_subtype == CATALYSTS)
      return _rxn.catalystNext(i);

   return _rxn.next(i);
}

IndigoObject * IndigoReactionIter::next ()
{
   if (_idx == -1)
   {
      _idx = _begin();
   }
   else
      _idx = _next(_idx);

   if (_idx == _end())
      return 0;

   return new IndigoReactionMolecule(_rxn, _idx);
}

bool IndigoReactionIter::hasNext ()
{
   if (_idx == -1)
      return _begin() != _end();

   return _next(_idx) != _end();
}

IndigoReaction * IndigoReaction::cloneFrom (IndigoObject & obj)
{
   Reaction &rxn = obj.getReaction();

   AutoPtr<IndigoReaction> rxnptr;
   rxnptr.reset(new IndigoReaction());
   rxnptr->rxn.clone(rxn, 0, 0, 0);

   RedBlackStringObjMap< Array<char> > *props = obj.getProperties();
   if (props != 0)
      rxnptr->copyProperties(*props);
   return rxnptr.release();
}

IndigoQueryReaction * IndigoQueryReaction::cloneFrom (IndigoObject & obj)
{
   QueryReaction &rxn = obj.getQueryReaction();

   AutoPtr<IndigoQueryReaction> rxnptr;
   rxnptr.reset(new IndigoQueryReaction());
   rxnptr->rxn.clone(rxn, 0, 0, 0);

   RedBlackStringObjMap< Array<char> > *props = obj.getProperties();
   if (props != 0)
      rxnptr->copyProperties(*props);
   return rxnptr.release();
}

IndigoObject * IndigoReaction::clone ()
{
   return cloneFrom(*this);
}

IndigoObject * IndigoQueryReaction::clone ()
{
   return cloneFrom(*this);
}

int _indigoIterateReaction (int reaction, int subtype)
{
   INDIGO_BEGIN
   {
      BaseReaction &rxn = self.getObject(reaction).getBaseReaction();

      return self.addObject(new IndigoReactionIter(rxn, subtype));
   }
   INDIGO_END(-1)
}

CEXPORT int indigoLoadReaction (int source)
{
   INDIGO_BEGIN
   {
      IndigoObject &obj = self.getObject(source);
      Scanner &scanner = IndigoScanner::get(obj);

      ReactionAutoLoader loader(scanner);

      loader.ignore_stereocenter_errors = self.ignore_stereochemistry_errors;
      loader.treat_x_as_pseudoatom = self.treat_x_as_pseudoatom;
      loader.ignore_noncritical_query_features = self.ignore_noncritical_query_features;

      AutoPtr<IndigoReaction> rxnptr(new IndigoReaction());
      loader.loadReaction(rxnptr->rxn);
      return self.addObject(rxnptr.release());
   }
   INDIGO_END(-1)
}

CEXPORT int indigoLoadQueryReaction (int source)
{
   INDIGO_BEGIN
   {
      IndigoObject &obj = self.getObject(source);
      Scanner &scanner = IndigoScanner::get(obj);

      ReactionAutoLoader loader(scanner);

      loader.ignore_stereocenter_errors = self.ignore_stereochemistry_errors;
      loader.treat_x_as_pseudoatom = self.treat_x_as_pseudoatom;

      AutoPtr<IndigoQueryReaction> rxnptr(new IndigoQueryReaction());
      loader.loadQueryReaction(rxnptr->rxn);
      return self.addObject(rxnptr.release());
   }
   INDIGO_END(-1)
}

CEXPORT int indigoIterateReactants (int reaction)
{
   return _indigoIterateReaction(reaction, IndigoReactionIter::REACTANTS);
}

CEXPORT int indigoIterateProducts (int reaction)
{
   return _indigoIterateReaction(reaction, IndigoReactionIter::PRODUCTS);
}

CEXPORT int indigoIterateCatalysts (int reaction)
{
   return _indigoIterateReaction(reaction, IndigoReactionIter::CATALYSTS);
}

CEXPORT int indigoIterateMolecules (int reaction)
{
   return _indigoIterateReaction(reaction, IndigoReactionIter::MOLECULES);
}

CEXPORT int indigoCreateReaction (void)
{
   INDIGO_BEGIN
   {
      return self.addObject(new IndigoReaction());
   }
   INDIGO_END(-1);
}

CEXPORT int indigoCreateQueryReaction (void)
{
   INDIGO_BEGIN
   {
      return self.addObject(new IndigoQueryReaction());
   }
   INDIGO_END(-1);
}

CEXPORT int indigoAddReactant (int reaction, int molecule)
{
   INDIGO_BEGIN
   {
      BaseReaction &rxn = self.getObject(reaction).getBaseReaction();

      rxn.addReactantCopy(self.getObject(molecule).getBaseMolecule(), 0, 0);
      return 1;
   }
   INDIGO_END(-1);
}

CEXPORT int indigoAddProduct (int reaction, int molecule)
{
   INDIGO_BEGIN
   {
      BaseReaction &rxn = self.getObject(reaction).getBaseReaction();

      rxn.addProductCopy(self.getObject(molecule).getBaseMolecule(), 0, 0);
      return 1;
   }
   INDIGO_END(-1);
}

CEXPORT int indigoAddCatalyst (int reaction, int molecule)
{
   INDIGO_BEGIN
   {
      BaseReaction &rxn = self.getObject(reaction).getBaseReaction();

      rxn.addCatalystCopy(self.getObject(molecule).getBaseMolecule(), 0, 0);
      return 1;
   }
   INDIGO_END(-1);
}

CEXPORT int indigoCountReactants (int reaction)
{
   INDIGO_BEGIN
   {
      return self.getObject(reaction).getBaseReaction().reactantsCount();
   }
   INDIGO_END(-1);
}

CEXPORT int indigoCountProducts (int reaction)
{
   INDIGO_BEGIN
   {
      return self.getObject(reaction).getBaseReaction().productsCount();
   }
   INDIGO_END(-1);
}

CEXPORT int indigoCountCatalysts (int reaction)
{
   INDIGO_BEGIN
   {
      return self.getObject(reaction).getBaseReaction().catalystCount();
   }
   INDIGO_END(-1);
}

CEXPORT int indigoCountMolecules (int handle)
{
   INDIGO_BEGIN
   {
      IndigoObject &obj = self.getObject(handle);

      if (IndigoBaseReaction::is(obj))
         return obj.getBaseReaction().count();
      
      throw IndigoError("can not count molecules of %s", obj.debugInfo());
   }
   INDIGO_END(-1);
}

CEXPORT int indigoAutomap (int reaction, const char *mode)
{
   INDIGO_BEGIN
   {
      BaseReaction &rxn = self.getObject(reaction).getBaseReaction();
      int nmode;

      if (mode == 0 || mode[0] == 0 || strcasecmp(mode, "discard") == 0)
         nmode = ReactionAutomapper::AAM_REGEN_DISCARD;
      else if (strcasecmp(mode, "alter") == 0)
         nmode = ReactionAutomapper::AAM_REGEN_ALTER;
      else if (strcasecmp(mode, "keep") == 0)
         nmode = ReactionAutomapper::AAM_REGEN_KEEP;
      else if (strcasecmp(mode, "clear") == 0)
      {
         rxn.clearAAM();
         return 0;
      }
      else
         throw IndigoError("indigoAutomap(): unknown mode: %s", mode);

      ReactionAutomapper ram(rxn);

      ram.automap(nmode);
      return 1;
   }
   INDIGO_END(-1);
}

CEXPORT int indigoLoadReactionSmarts (int source)
{
   INDIGO_BEGIN
   {
      IndigoObject &obj = self.getObject(source);
      RSmilesLoader loader(IndigoScanner::get(obj));

      AutoPtr<IndigoQueryReaction> rxnptr(new IndigoQueryReaction());

      QueryReaction &qrxn = rxnptr->rxn;

      loader.smarts_mode = true;
      loader.loadQueryReaction(qrxn);
      return self.addObject(rxnptr.release());
   }
   INDIGO_END(-1);
}
