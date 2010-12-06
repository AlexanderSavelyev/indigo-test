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
#include "indigo_io.h"
#include "indigo_array.h"
#include "molecule/molecule_auto_loader.h"
#include "molecule/molfile_saver.h"
#include "base_cpp/output.h"
#include "molecule/gross_formula.h"
#include "molecule/molecule_mass.h"
#include "molecule/query_molecule.h"
#include "molecule/smiles_loader.h"
#include "molecule/smiles_saver.h"
#include "molecule/canonical_smiles_saver.h"
#include "molecule/molecule_cml_saver.h"
#include "molecule/molecule_substructure_matcher.h"
#include "graph/graph_decomposer.h"
#include "molecule/molecule_inchi.h"
#include "base_c/bitarray.h"
#include "molecule/molecule_fingerprint.h"
#include "molecule/elements.h"

IndigoGross::IndigoGross() : IndigoObject(GROSS)
{
}

IndigoGross::~IndigoGross ()
{
}

void IndigoGross::toString (Array<char> &str)
{
   GrossFormula::toString(gross, str);
}

IndigoBaseMolecule::IndigoBaseMolecule (int type_) : IndigoObject(type_)
{
   highlighting.clear();
}

IndigoBaseMolecule::~IndigoBaseMolecule ()
{
}

GraphHighlighting * IndigoBaseMolecule::getMoleculeHighlighting ()
{
   return &highlighting;
}

RedBlackStringObjMap< Array<char> > * IndigoBaseMolecule::getProperties ()
{
   return &properties;
}

DLLEXPORT const char * IndigoBaseMolecule::debugInfo ()
{
   return "<base molecule>";
}

IndigoMolecule::IndigoMolecule () : IndigoBaseMolecule(MOLECULE)
{
}

IndigoMolecule::~IndigoMolecule ()
{
}

Molecule & IndigoMolecule::getMolecule ()
{
   return mol;
}

BaseMolecule & IndigoMolecule::getBaseMolecule ()
{
   return mol;
}

const char * IndigoMolecule::getName ()
{
   if (mol.name.ptr() == 0)
      return "";
   return mol.name.ptr();
}

IndigoQueryMolecule::IndigoQueryMolecule () : IndigoBaseMolecule(QUERY_MOLECULE)
{
}

IndigoQueryMolecule::~IndigoQueryMolecule ()
{
}

QueryMolecule & IndigoQueryMolecule::getQueryMolecule ()
{
   return qmol;
}

BaseMolecule & IndigoQueryMolecule::getBaseMolecule ()
{
   return qmol;
}

const char * IndigoQueryMolecule::getName ()
{
   if (qmol.name.ptr() == 0)
      return "";
   return qmol.name.ptr();
}


IndigoAtom::IndigoAtom (BaseMolecule &mol_, int idx_) : IndigoObject (ATOM)
{
   mol = &mol_;
   idx = idx_;
}

IndigoAtom::~IndigoAtom ()
{
}

int IndigoAtom::getIndex ()
{
   return idx;
}

IndigoAtom & IndigoAtom::cast (IndigoObject &obj)
{
   if (obj.type == IndigoObject::ATOM)
      return (IndigoAtom &)obj;
   if (obj.type == IndigoObject::ARRAY_ELEMENT)
      return cast(((IndigoArrayElement &)obj).get());
   throw IndigoError("%s does not represent an atom", obj.debugInfo());
}


IndigoAtomsIter::IndigoAtomsIter (BaseMolecule *mol, int type) : IndigoObject(ATOMS_ITER)
{
   _mol = mol;
   _type = type;
   _idx = -1;
}

IndigoAtomsIter::~IndigoAtomsIter ()
{
}

int IndigoAtomsIter::_shift (int idx)
{
   if (_type == PSEUDO)
   {
      for (; idx != _mol->vertexEnd(); idx = _mol->vertexNext(idx))
         if (_mol->isPseudoAtom(idx))
            break;
   }
   else if (_type == RSITE)
   {
      for (; idx != _mol->vertexEnd(); idx = _mol->vertexNext(idx))
         if (_mol->isRSite(idx))
            break;
   }
   else if (_type == STEREOCENTER)
   {
      for (; idx != _mol->vertexEnd(); idx = _mol->vertexNext(idx))
         if (_mol->stereocenters.getType(idx) != 0)
            break;
   }

   return idx;
}

bool IndigoAtomsIter::hasNext ()
{
   if (_idx == _mol->vertexEnd())
      return false;

   int next_idx;

   if (_idx == -1)
      next_idx = _shift(_mol->vertexBegin());
   else
      next_idx = _shift(_mol->vertexNext(_idx));

   return next_idx != _mol->vertexEnd();
}

IndigoObject * IndigoAtomsIter::next ()
{
   if (_idx == -1)
      _idx = _mol->vertexBegin();
   else
      _idx = _mol->vertexNext(_idx);

   _idx = _shift(_idx);

   if (_idx == _mol->vertexEnd())
      return 0;

   AutoPtr<IndigoAtom> atom(new IndigoAtom(*_mol, _idx));
   
   return atom.release();
}

IndigoBond::IndigoBond (BaseMolecule &mol_, int idx_) : IndigoObject(BOND)
{
   mol = &mol_;
   idx = idx_;
}

IndigoBond::~IndigoBond ()
{
}

int IndigoBond::getIndex ()
{
   return idx;
}

IndigoBond & IndigoBond::cast (IndigoObject &obj)
{
   if (obj.type == IndigoObject::BOND)
      return (IndigoBond &)obj;
   if (obj.type == IndigoObject::ARRAY_ELEMENT)
      return cast(((IndigoArrayElement &)obj).get());
   throw IndigoError("%s does not represent an atom", obj.debugInfo());
}

IndigoBondsIter::IndigoBondsIter (BaseMolecule *mol) : IndigoObject(BONDS_ITER)
{
   _mol = mol;
   _idx = -1;
}

IndigoBondsIter::~IndigoBondsIter ()
{
}

bool IndigoBondsIter::hasNext ()
{
   if (_idx == _mol->edgeEnd())
      return false;

   int next_idx;

   if (_idx == -1)
      next_idx = _mol->edgeBegin();
   else
      next_idx = _mol->edgeNext(_idx);

   return next_idx != _mol->edgeEnd();
}

IndigoObject * IndigoBondsIter::next ()
{
   if (_idx == -1)
      _idx = _mol->edgeBegin();
   else
      _idx = _mol->edgeNext(_idx);

   if (_idx == _mol->edgeEnd())
      return 0;

   AutoPtr<IndigoBond> bond(new IndigoBond(*_mol, _idx));

   return bond.release();
}

CEXPORT int indigoIterateBonds (int molecule)
{
   INDIGO_BEGIN
   {
      BaseMolecule &mol = self.getObject(molecule).getBaseMolecule();

      AutoPtr<IndigoBondsIter> newiter(new IndigoBondsIter(&mol));

      return self.addObject(newiter.release());
   }
   INDIGO_END(-1);
}


CEXPORT int indigoLoadMolecule (int source)
{
   INDIGO_BEGIN
   {
      IndigoObject &obj = self.getObject(source);

      MoleculeAutoLoader loader(IndigoScanner::get(obj));

      loader.ignore_stereocenter_errors = self.ignore_stereochemistry_errors;
      loader.treat_x_as_pseudoatom = self.treat_x_as_pseudoatom;

      AutoPtr<IndigoMolecule> molptr(new IndigoMolecule());

      Molecule &mol = molptr->mol;
      loader.highlighting = &molptr->highlighting;

      loader.loadMolecule(mol);

      return self.addObject(molptr.release());
   }
   INDIGO_END(-1);
}

CEXPORT int indigoLoadQueryMolecule (int source)
{
   INDIGO_BEGIN
   {
      IndigoObject &obj = self.getObject(source);
      MoleculeAutoLoader loader(IndigoScanner::get(obj));

      loader.ignore_stereocenter_errors = self.ignore_stereochemistry_errors;
      loader.treat_x_as_pseudoatom = self.treat_x_as_pseudoatom;

      AutoPtr<IndigoQueryMolecule> molptr(new IndigoQueryMolecule());

      QueryMolecule &qmol = molptr->qmol;
      loader.highlighting = &molptr->highlighting;

      loader.loadQueryMolecule(qmol);
      return self.addObject(molptr.release());
   }
   INDIGO_END(-1);
}

CEXPORT int indigoLoadSmarts (int source)
{
   INDIGO_BEGIN
   {
      IndigoObject &obj = self.getObject(source);
      SmilesLoader loader(IndigoScanner::get(obj));

      AutoPtr<IndigoQueryMolecule> molptr(new IndigoQueryMolecule());

      QueryMolecule &qmol = molptr->qmol;

      loader.loadSMARTS(qmol);
      return self.addObject(molptr.release());
   }
   INDIGO_END(-1);
}

CEXPORT int indigoSaveMolfile (int molecule, int output)
{
   INDIGO_BEGIN
   {
      IndigoObject &obj = self.getObject(molecule);
      BaseMolecule &mol = obj.getBaseMolecule();
      Output &out = IndigoOutput::get(self.getObject(output));

      MolfileSaver saver(out);
      saver.mode = self.molfile_saving_mode;
      saver.highlighting = obj.getMoleculeHighlighting();
      if (mol.isQueryMolecule())
         saver.saveQueryMolecule(mol.asQueryMolecule());
      else
         saver.saveMolecule(mol.asMolecule());
      out.flush();
      return 1;
   }
   INDIGO_END(-1)
}

CEXPORT int indigoSaveCml (int molecule, int output)
{
   INDIGO_BEGIN
   {
      Molecule &mol = self.getObject(molecule).getMolecule();
      Output &out = IndigoOutput::get(self.getObject(output));

      MoleculeCmlSaver saver(out);
      saver.saveMolecule(mol);
      out.flush();
      return 1;
   }
   INDIGO_END(-1)
}

CEXPORT int indigoSdfAppend (int output, int molecule)
{
   INDIGO_BEGIN
   {
      BaseMolecule &mol = self.getObject(molecule).getBaseMolecule();
      RedBlackStringObjMap< Array<char> > *props = self.getObject(molecule).getProperties();
      Output &out = IndigoOutput::get(self.getObject(output));

      MolfileSaver saver(out);
      saver.mode = self.molfile_saving_mode;
      saver.highlighting = self.getObject(molecule).getMoleculeHighlighting();
      if (mol.isQueryMolecule())
         saver.saveQueryMolecule(mol.asQueryMolecule());
      else
         saver.saveMolecule(mol.asMolecule());

      if (props != 0)
      {
         int i;

         for (i = props->begin(); i != props->end(); i = props->next(i))
            out.printf(">  <%s>\n%s\n\n", props->key(i), props->value(i).ptr());
      }

      out.printfCR("$$$$");
      out.flush();
      return 1;
   }
   INDIGO_END(-1)
}

CEXPORT int indigoSmilesAppend (int output, int molecule)
{
   INDIGO_BEGIN
   {
      BaseMolecule &mol = self.getObject(molecule).getBaseMolecule();
      Output &out = IndigoOutput::get(self.getObject(output));

      SmilesSaver saver(out);
      if (mol.isQueryMolecule())
         saver.saveQueryMolecule(mol.asQueryMolecule());
      else
         saver.saveMolecule(mol.asMolecule());
      out.writeCR();
      out.flush();
      return 1;
   }
   INDIGO_END(-1)
}

CEXPORT int indigoGrossFormula (int molecule)
{
   INDIGO_BEGIN
   {
      BaseMolecule &mol = self.getObject(molecule).getBaseMolecule();
      AutoPtr<IndigoGross> grossptr(new IndigoGross());

      GrossFormula::collect(mol, grossptr->gross);
      return self.addObject(grossptr.release());
   }
   INDIGO_END(-1)
}

CEXPORT float indigoMolecularWeight (int molecule)
{
   INDIGO_BEGIN
   {
      Molecule &mol = self.getObject(molecule).getMolecule();

      MoleculeMass mass;
      return mass.molecularWeight(mol);
   }
   INDIGO_END(-1)
}

CEXPORT float indigoMostAbundantMass (int molecule)
{
   INDIGO_BEGIN
   {
      Molecule &mol = self.getObject(molecule).getMolecule();

      MoleculeMass mass;
      return mass.mostAbundantMass(mol);
   }
   INDIGO_END(-1)
}

CEXPORT float indigoMonoisotopicMass (int molecule)
{
   INDIGO_BEGIN
   {
      Molecule &mol = self.getObject(molecule).getMolecule();

      MoleculeMass mass;
      return mass.monoisotopicMass(mol);
   }
   INDIGO_END(-1)
}

int _indigoIterateAtoms (Indigo &self, int molecule, int type)
{
   BaseMolecule &mol = self.getObject(molecule).getBaseMolecule();

   AutoPtr<IndigoAtomsIter> newiter(new IndigoAtomsIter(&mol, type));

   return self.addObject(newiter.release());
}

CEXPORT int indigoIterateAtoms (int molecule)
{
   INDIGO_BEGIN
   {
      return _indigoIterateAtoms(self, molecule, IndigoAtomsIter::ALL);
   }
   INDIGO_END(-1);
}

CEXPORT int indigoCountAtoms (int molecule)
{
   INDIGO_BEGIN
   {
      BaseMolecule &mol = self.getObject(molecule).getBaseMolecule();
      
      return mol.vertexCount();
   }
   INDIGO_END(-1);
}

CEXPORT int indigoCountBonds (int molecule)
{
   INDIGO_BEGIN
   {
      BaseMolecule &mol = self.getObject(molecule).getBaseMolecule();

      return mol.edgeCount();
   }
   INDIGO_END(-1);
}

CEXPORT int indigoCountPseudoatoms (int molecule)
{
   INDIGO_BEGIN
   {
      BaseMolecule &mol = self.getObject(molecule).getBaseMolecule();
      int i, res = 0;

      for (i = mol.vertexBegin(); i != mol.vertexEnd(); i = mol.vertexNext(i))
         if (mol.isPseudoAtom(i))
            res++;

      return res;
   }
   INDIGO_END(-1);
}

CEXPORT int indigoCountRSites (int molecule)
{
   INDIGO_BEGIN
   {
      BaseMolecule &mol = self.getObject(molecule).getBaseMolecule();
      int i, res = 0;

      for (i = mol.vertexBegin(); i != mol.vertexEnd(); i = mol.vertexNext(i))
         if (mol.isRSite(i))
            res++;

      return res;
   }
   INDIGO_END(-1);
}

CEXPORT int indigoIteratePseudoatoms (int molecule)
{
   INDIGO_BEGIN
   {
      return _indigoIterateAtoms(self, molecule, IndigoAtomsIter::PSEUDO);
   }
   INDIGO_END(-1);
}

CEXPORT int indigoIterateRSites (int molecule)
{
   INDIGO_BEGIN
   {
      return _indigoIterateAtoms(self, molecule, IndigoAtomsIter::RSITE);
   }
   INDIGO_END(-1);
}

CEXPORT int indigoIterateStereocenters (int molecule)
{
   INDIGO_BEGIN
   {
      return _indigoIterateAtoms(self, molecule, IndigoAtomsIter::STEREOCENTER);
   }
   INDIGO_END(-1);
}

CEXPORT const char * indigoSymbol (int atom)
{
   INDIGO_BEGIN
   {
      IndigoAtom &ia = IndigoAtom::cast(self.getObject(atom));

      if (ia.mol->isPseudoAtom(ia.idx))
         return ia.mol->getPseudoAtom(ia.idx);
      else if (ia.mol->isRSite(ia.idx))
      {
         QS_DEF(Array<int>, rgroups);
         int i;
         ia.mol->getAllowedRGroups(ia.idx, rgroups);

         if (rgroups.size() == 0)
            return "R";

         ArrayOutput output(self.tmp_string);
         for (i = 0; i < rgroups.size(); i++)
         {
            if (i > 0)
               output.writeChar(',');
            output.printf("R%d", rgroups[i]);
         }
         output.writeChar(0);
         return self.tmp_string.ptr();
      }
      else 
      {
         int number = ia.mol->getAtomNumber(ia.idx);
         QS_DEF(Array<int>, list);

         if (number != -1)
            return Element::toString(number);

         int query_atom_type;

         if (ia.mol->isQueryMolecule() &&
               (query_atom_type = QueryMolecule::parseQueryAtom(ia.mol->asQueryMolecule(), ia.idx, list)) != -1)
         {
            if (query_atom_type == QueryMolecule::QUERY_ATOM_A)
               return "A";
            if (query_atom_type == QueryMolecule::QUERY_ATOM_Q)
               return "Q";
            else if (query_atom_type == QueryMolecule::QUERY_ATOM_X)
               return "X";
            else if (query_atom_type == QueryMolecule::QUERY_ATOM_LIST ||
                     query_atom_type == QueryMolecule::QUERY_ATOM_NOTLIST)
            {
               int k;
               ArrayOutput output(self.tmp_string);

               if (query_atom_type == QueryMolecule::QUERY_ATOM_NOTLIST)
                  output.writeString("NOT");

               output.writeChar('[');
               for (k = 0; k < list.size(); k++)
               {
                  if (k > 0)
                     output.writeChar(',');
                  output.writeString(Element::toString(list[k]));
               }
               output.writeChar(']');
               output.writeChar(0);
               return self.tmp_string.ptr();
            }
         }
      }
      return "*";
   }
   INDIGO_END(0);
}

CEXPORT int indigoIsPseudoatom (int atom)
{
   INDIGO_BEGIN
   {
      IndigoAtom &ia = IndigoAtom::cast(self.getObject(atom));

      if (ia.mol->isPseudoAtom(ia.idx))
         return 1;
      return 0;
   }
   INDIGO_END(-1);
}

CEXPORT int indigoIsRSite (int atom)
{
   INDIGO_BEGIN
   {
      IndigoAtom &ia = IndigoAtom::cast(self.getObject(atom));

      if (ia.mol->isRSite(ia.idx))
         return 1;
      return 0;
   }
   INDIGO_END(-1);
}

CEXPORT int indigoStereocenterType (int atom)
{
   INDIGO_BEGIN
   {
      IndigoAtom &ia = IndigoAtom::cast(self.getObject(atom));

      switch (ia.mol->stereocenters.getType(ia.idx))
      {
         case MoleculeStereocenters::ATOM_ABS: return INDIGO_ABS;
         case MoleculeStereocenters::ATOM_OR: return INDIGO_OR;
         case MoleculeStereocenters::ATOM_AND: return INDIGO_AND;
         case MoleculeStereocenters::ATOM_ANY: return INDIGO_EITHER;
         default: return 0;
      }
      return 0;
   }
   INDIGO_END(-1);
}

CEXPORT int indigoSingleAllowedRGroup (int atom)
{
   INDIGO_BEGIN
   {
      IndigoAtom &ia = IndigoAtom::cast(self.getObject(atom));

      return ia.mol->getSingleAllowedRGroup(ia.idx);
   }
   INDIGO_END(-1);
}


IndigoRGroup::IndigoRGroup () : IndigoObject(RGROUP)
{
}

IndigoRGroup::~IndigoRGroup ()
{
}

int IndigoRGroup::getIndex ()
{
   return idx;
}

IndigoRGroup & IndigoRGroup::getRGroup ()
{
   return *this;
}

IndigoRGroupsIter::IndigoRGroupsIter (QueryMolecule *mol) : IndigoObject(RGROUPS_ITER)
{
   _mol = mol;
   _idx = 0;
}

IndigoRGroupsIter::~IndigoRGroupsIter ()
{
}

CEXPORT int indigoIterateRGroups (int molecule)
{
   INDIGO_BEGIN
   {
      IndigoObject &obj = self.getObject(molecule);

      if (obj.isBaseMolecule())
      {
         QueryMolecule &mol = obj.getQueryMolecule();

         return self.addObject(new IndigoRGroupsIter(&mol));
      }

      throw IndigoError("%s can not have r-groups", obj.debugInfo());
   }
   INDIGO_END(-1);
}

IndigoRGroupFragment::IndigoRGroupFragment (IndigoRGroup &rgp, int idx) : IndigoObject(RGROUP_FRAGMENT)
{
   rgroup.idx = rgp.idx;
   rgroup.mol = rgp.mol;
   frag_idx = idx;
}

IndigoRGroupFragment::IndigoRGroupFragment (QueryMolecule *mol, int rgroup_idx, int fragment_idx) :
IndigoObject(RGROUP_FRAGMENT)
{
   rgroup.mol = mol;
   rgroup.idx = rgroup_idx;
   frag_idx = fragment_idx;
}

IndigoRGroupFragment::~IndigoRGroupFragment ()
{
}

int IndigoRGroupFragment::getIndex ()
{
   return frag_idx;
}

QueryMolecule & IndigoRGroupFragment::getQueryMolecule ()
{
   return *rgroup.mol->asQueryMolecule().rgroups.getRGroup(rgroup.idx).fragments[frag_idx];
}

BaseMolecule & IndigoRGroupFragment::getBaseMolecule ()
{
   return getQueryMolecule();
}

IndigoRGroupFragmentsIter::IndigoRGroupFragmentsIter (IndigoRGroup& rgp) :
IndigoObject(RGROUP_FRAGMENTS_ITER)
{
   _mol = &rgp.mol->asQueryMolecule();
   _rgroup_idx = rgp.idx;
   _frag_idx = -1;
}

IndigoRGroupFragmentsIter::~IndigoRGroupFragmentsIter ()
{
}

bool IndigoRGroupFragmentsIter::hasNext ()
{
   return _mol->rgroups.getRGroup(_rgroup_idx).fragmentsCount() > _frag_idx + 1;
}

IndigoObject * IndigoRGroupFragmentsIter::next ()
{
   if (!hasNext())
      return 0;

   _frag_idx++;

   AutoPtr<IndigoRGroupFragment> rgroup(new IndigoRGroupFragment(_mol, _rgroup_idx, _frag_idx));

   return rgroup.release();
}

CEXPORT int indigoIterateRGroupFragments (int rgroup)
{
   INDIGO_BEGIN
   {
      IndigoRGroup &rgp = self.getObject(rgroup).getRGroup();

      AutoPtr<IndigoRGroupFragmentsIter> newiter(new IndigoRGroupFragmentsIter(rgp));
      return self.addObject(newiter.release());
   }
   INDIGO_END(-1);
}

bool IndigoRGroupsIter::hasNext ()
{
   return _idx + 1 <= _mol->rgroups.getRGroupCount();
}

IndigoObject * IndigoRGroupsIter::next ()
{
   if (!hasNext())
      return 0;
   
   _idx += 1;
   AutoPtr<IndigoRGroup> rgroup(new IndigoRGroup());

   rgroup->mol = _mol;
   rgroup->idx = _idx;
   return rgroup.release();
}

CEXPORT int indigoCountAttachmentPoints (int rgroupp)
{
   INDIGO_BEGIN
   {
      IndigoRGroup &rgroup = self.getObject(rgroupp).getRGroup();

      return rgroup.mol->rgroups.getRGroup(rgroup.idx).fragments[0]->
               getRGroupFragment().attachmentPointCount();
   }
   INDIGO_END(-1);
}

CEXPORT int indigoDegree (int atom)
{
   INDIGO_BEGIN
   {
      IndigoAtom &ia = IndigoAtom::cast(self.getObject(atom));

      return ia.mol->getVertex(ia.idx).degree();
   }
   INDIGO_END(-1);
}

CEXPORT int indigoGetCharge (int atom, int *charge)
{
   INDIGO_BEGIN
   {
      IndigoAtom &ia = IndigoAtom::cast(self.getObject(atom));
      int ch = ia.mol->getAtomCharge(ia.idx);
      if (ch == CHARGE_UNKNOWN)
      {
         *charge = 0;
         return 0;
      }
      *charge = ch;
      return 1;
   }
   INDIGO_END(-1);
}

CEXPORT int indigoGetExplicitValence (int atom, int *valence)
{
   INDIGO_BEGIN
   {
      IndigoAtom &ia = IndigoAtom::cast(self.getObject(atom));
      int val = ia.mol->getExplicitValence(ia.idx);
      if (val == -1)
      {
         *valence = 0;
         return 0;
      }
      *valence = val;
      return 1;
   }
   INDIGO_END(-1);
}

CEXPORT int indigoIsotope (int atom)
{
   INDIGO_BEGIN
   {
      IndigoAtom &ia = IndigoAtom::cast(self.getObject(atom));
      int iso = ia.mol->getAtomIsotope(ia.idx);
      return iso == -1 ? 0 : iso;
   }
   INDIGO_END(-1);
}

CEXPORT int indigoAtomicNumber (int atom)
{
   INDIGO_BEGIN
   {
      IndigoAtom &ia = IndigoAtom::cast(self.getObject(atom));

      if (ia.mol->isPseudoAtom(ia.idx))
         throw IndigoError("indigoAtomicNumber() called on a pseudoatom");
      if (ia.mol->isRSite(ia.idx))
         throw IndigoError("indigoAtomicNumber() called on an R-site");

      int num = ia.mol->getAtomNumber(ia.idx);
      return num == -1 ? 0 : num;
   }
   INDIGO_END(-1);
}

CEXPORT int indigoGetRadicalElectrons (int atom, int *electrons)
{
   INDIGO_BEGIN
   {
      IndigoAtom &ia = IndigoAtom::cast(self.getObject(atom));
      int rad = ia.mol->getAtomRadical(ia.idx);

      if (rad == -1)
      {
         *electrons = 0;
         return 0;
      }
      *electrons = Element::radicalElectrons(rad);
      return 1;
   }
   INDIGO_END(-1);
}

CEXPORT float * indigoXYZ (int atom)
{
   INDIGO_BEGIN
   {
      IndigoAtom &ia = IndigoAtom::cast(self.getObject(atom));
      BaseMolecule *mol = ia.mol;

      Vec3f &pos = mol->getAtomXyz(ia.idx);
      self.tmp_xyz[0] = pos.x;
      self.tmp_xyz[1] = pos.y;
      self.tmp_xyz[2] = pos.z;
      return self.tmp_xyz;
   }
   INDIGO_END(0)
}

CEXPORT int indigoResetCharge (int atom)
{
   INDIGO_BEGIN
   {
      IndigoAtom &ia = IndigoAtom::cast(self.getObject(atom));
      BaseMolecule *mol = ia.mol;

      if (mol->isQueryMolecule())
         mol->asQueryMolecule().getAtom(ia.idx).removeConstraints(QueryMolecule::ATOM_CHARGE);
      else
         mol->asMolecule().setAtomCharge(ia.idx, 0);
      return 1;
   }
   INDIGO_END(-1);
}

CEXPORT int indigoResetExplicitValence (int atom)
{
   INDIGO_BEGIN
   {
      IndigoAtom &ia = IndigoAtom::cast(self.getObject(atom));
      BaseMolecule *mol = ia.mol;

      if (mol->isQueryMolecule())
         mol->asQueryMolecule().getAtom(ia.idx).removeConstraints(QueryMolecule::ATOM_VALENCE);
      else
         mol->asMolecule().resetExplicitValence(ia.idx);
      return 1;
   }
   INDIGO_END(-1);
}

CEXPORT int indigoResetRadical (int atom)
{
   INDIGO_BEGIN
   {
      IndigoAtom &ia = IndigoAtom::cast(self.getObject(atom));
      BaseMolecule *mol = ia.mol;

      if (mol->isQueryMolecule())
         mol->asQueryMolecule().getAtom(ia.idx).removeConstraints(QueryMolecule::ATOM_RADICAL);
      else
         mol->asMolecule().setAtomRadical(ia.idx, 0);
      return 1;
   }
   INDIGO_END(-1);
}

CEXPORT int indigoResetIsotope (int atom)
{
   INDIGO_BEGIN
   {
      IndigoAtom &ia = IndigoAtom::cast(self.getObject(atom));
      BaseMolecule *mol = ia.mol;

      if (mol->isQueryMolecule())
         mol->asQueryMolecule().getAtom(ia.idx).removeConstraints(QueryMolecule::ATOM_ISOTOPE);
      else
         mol->asMolecule().setAtomIsotope(ia.idx, 0);
      return 1;
   }
   INDIGO_END(-1);
}

CEXPORT const char * indigoCanonicalSmiles (int molecule)
{
   INDIGO_BEGIN
   {
      Molecule &mol = self.getObject(molecule).getMolecule();

      ArrayOutput output(self.tmp_string);
      CanonicalSmilesSaver saver(output);
      
      saver.saveMolecule(mol);
      self.tmp_string.push(0);
      return self.tmp_string.ptr();
   }
   INDIGO_END(0);
}

CEXPORT const char * indigoLayeredCode (int molecule)
{
   INDIGO_BEGIN
   {
      Molecule &mol = self.getObject(molecule).getMolecule();

      ArrayOutput output(self.tmp_string);

      MoleculeInChI inchi_saver(output);
      inchi_saver.outputInChI(mol);

      self.tmp_string.push(0);
      return self.tmp_string.ptr();
   }
   INDIGO_END(0);
}

CEXPORT int indigoCreateSubmolecule (int molecule, int nvertices, int *vertices)
{
   INDIGO_BEGIN
   {
      BaseMolecule &mol = self.getObject(molecule).getBaseMolecule();
      
      QS_DEF(Array<int>, vertices_arr);

      vertices_arr.copy(vertices, nvertices);

      if (mol.isQueryMolecule())
      {
         AutoPtr<IndigoQueryMolecule> molptr(new IndigoQueryMolecule());

         molptr->qmol.makeSubmolecule(mol, vertices_arr, 0, 0);
         return self.addObject(molptr.release());
      }
      else
      {
         AutoPtr<IndigoMolecule> molptr(new IndigoMolecule());

         molptr->mol.makeSubmolecule(mol, vertices_arr, 0, 0);
         return self.addObject(molptr.release());
      }
   }
   INDIGO_END(-1)
}

CEXPORT int indigoCreateEdgeSubmolecule (int molecule, int nvertices, int *vertices,
                                                       int nedges, int *edges)
{
   INDIGO_BEGIN
   {
      BaseMolecule &mol = self.getObject(molecule).getBaseMolecule();

      QS_DEF(Array<int>, vertices_arr);
      QS_DEF(Array<int>, edges_arr);

      vertices_arr.copy(vertices, nvertices);
      edges_arr.copy(edges, nedges);

      if (mol.isQueryMolecule())
      {
         AutoPtr<IndigoQueryMolecule> molptr(new IndigoQueryMolecule());

         molptr->qmol.makeEdgeSubmolecule(mol, vertices_arr, edges_arr, 0, 0);
         return self.addObject(molptr.release());
      }
      else
      {
         AutoPtr<IndigoMolecule> molptr(new IndigoMolecule());

         molptr->mol.makeEdgeSubmolecule(mol, vertices_arr, edges_arr, 0, 0);
         return self.addObject(molptr.release());
      }
   }
   INDIGO_END(-1)
}

IndigoObject * IndigoMolecule::clone ()
{
   AutoPtr<IndigoMolecule> molptr;
   molptr.reset(new IndigoMolecule());
   molptr->mol.clone(mol, 0, 0);
   molptr->copyProperties(properties);
   return molptr.release();
}

DLLEXPORT const char * IndigoMolecule::debugInfo ()
{
   return "<molecule>";
}

IndigoObject * IndigoQueryMolecule::clone ()
{
   AutoPtr<IndigoQueryMolecule> molptr;
   molptr.reset(new IndigoQueryMolecule());
   molptr->qmol.clone(qmol, 0, 0);
   molptr->copyProperties(properties);
   return molptr.release();
}

DLLEXPORT const char * IndigoQueryMolecule::debugInfo ()
{
   return "<query molecule>";
}

//
// IndigoMoleculeSubstructureMatch
//

IndigoMoleculeSubstructureMatch::IndigoMoleculeSubstructureMatch (Molecule &target, QueryMolecule &query) :
   IndigoObject(MOLECULE_SUBSTRUCTURE_MATCH), target(target), query(query)
{
}

IndigoMoleculeSubstructureMatch::~IndigoMoleculeSubstructureMatch ()
{
}

const char * IndigoMoleculeSubstructureMatch::debugInfo ()
{
   return "<molecule substructure match>";
}

CEXPORT int indigoMatchHighlight (int match)
{
   INDIGO_BEGIN
   {
      IndigoObject &obj = self.getObject(match);
      if (obj.type != IndigoObject::MOLECULE_SUBSTRUCTURE_MATCH)
         throw IndigoError("indigoMatchHighlight(): match must be given, not %s", obj.debugInfo());

      IndigoMoleculeSubstructureMatch &match = (IndigoMoleculeSubstructureMatch &)obj;

      AutoPtr<IndigoMolecule> mol(new IndigoMolecule());

      QS_DEF(Array<int>, mapping);
      mol->mol.clone(match.target, &mapping, 0);

      mol->highlighting.init(mol->mol);

      for (int i = mol->mol.vertexBegin(); i != mol->mol.vertexEnd(); i = mol->mol.vertexNext(i))
      {
         if (match.highlighting.hasVertex(mapping[i]))
            mol->highlighting.onVertex(i);
      }

      for (int i = mol->mol.edgeBegin(); i != mol->mol.edgeEnd(); i = mol->mol.edgeNext(i))
      {
         const Edge &edge = mol->mol.getEdge(i);

         if (match.highlighting.hasEdge(match.target.findEdgeIndex(mapping[edge.beg], mapping[edge.end])))
            mol->highlighting.onEdge(i);
      }

      return self.addObject(mol.release());
   }
   INDIGO_END(-1)
}

CEXPORT int indigoMapAtom (int match, int query_atom)
{
   INDIGO_BEGIN
   {
      IndigoObject &obj = self.getObject(match);
      if (obj.type != IndigoObject::MOLECULE_SUBSTRUCTURE_MATCH)
         throw IndigoError("indigoMatchHighlight(): match must be given, not %s", obj.debugInfo());
      IndigoAtom &ia = IndigoAtom::cast(self.getObject(query_atom));
      
      IndigoMoleculeSubstructureMatch &match = (IndigoMoleculeSubstructureMatch &)obj;
      match.query.getAtom(ia.idx); // will throw an exception if the atom index is invalid
      int idx = match.query_atom_mapping[ia.idx];

      return self.addObject(new IndigoAtom(match.target, idx));
   }
   INDIGO_END(-1)
}

CEXPORT int indigoMapBond (int match, int query_bond)
{
   INDIGO_BEGIN
   {
      IndigoObject &obj = self.getObject(match);
      if (obj.type != IndigoObject::MOLECULE_SUBSTRUCTURE_MATCH)
         throw IndigoError("indigoMatchHighlight(): match must be given, not %s", obj.debugInfo());
      IndigoBond &ib = IndigoBond::cast(self.getObject(query_bond));

      IndigoMoleculeSubstructureMatch &match = (IndigoMoleculeSubstructureMatch &)obj;
      const Edge &edge = match.query.getEdge(ib.idx);
      int idx = match.target.findEdgeIndex(match.query_atom_mapping[edge.beg],
                                           match.query_atom_mapping[edge.end]);
      
      return self.addObject(new IndigoBond(match.target, idx));
   }
   INDIGO_END(-1)
}

//
// IndigoMoleculeSubstructureMatchIter
//

IndigoMoleculeSubstructureMatchIter::IndigoMoleculeSubstructureMatchIter (Molecule &target, 
                                                                          QueryMolecule &query,
                                                                          Molecule &original_target) :
        IndigoObject(MOLECULE_SUBSTRUCTURE_MATCH_ITER),
        matcher(target),
        target(target),
        original_target(original_target),
        query(query)
{
   matcher.setQuery(query);
   matcher.fmcache = &fmcache;
   matcher.highlighting = &highlighting;

   highlighting.init(target);

   _initialized = false;
   _found = false;
   _need_find = true;
}

IndigoMoleculeSubstructureMatchIter::~IndigoMoleculeSubstructureMatchIter ()
{
}

const char * IndigoMoleculeSubstructureMatchIter::debugInfo ()
{
   return "<molecule substructure match iterator>";
}

IndigoObject * IndigoMoleculeSubstructureMatchIter::next ()
{
   if (!hasNext())
      return 0;

   AutoPtr<IndigoMoleculeSubstructureMatch> mptr(
         new IndigoMoleculeSubstructureMatch(original_target, query));

   // Expand mapping to fit possible implicit hydrogens
   mapping.expandFill(target.vertexEnd(), -1);

   mptr->highlighting.init(original_target);
   mptr->highlighting.copy(highlighting, &mapping);

   mptr->query_atom_mapping.copy(matcher.getQueryMapping(), query.vertexEnd());

   for (int v = query.vertexBegin(); v != query.vertexEnd(); v = query.vertexNext(v))
   {
      int mapped = mptr->query_atom_mapping[v];
      mptr->query_atom_mapping[v] = mapping[mapped];
   }

   _need_find = true;
   return mptr.release();
}

bool IndigoMoleculeSubstructureMatchIter::hasNext ()
{
   if (!_need_find)
      return _found;

   if (!_initialized)
   {
      _initialized = true;
      _found = matcher.find();
   }
   else
      _found = matcher.findNext();

   _need_find = false;
   return _found;
}

struct MatchCountContext
{
   int embeddings_count, max_count;
};

static bool _matchCountEmbeddingsCallback (Graph &sub, Graph &super, 
                                           const int *core1, const int *core2, void *context_)
{
   MatchCountContext *context = (MatchCountContext *)context_;
   context->embeddings_count++;
   if (context->embeddings_count >= context->max_count)
      return false;
   return true;
}

int IndigoMoleculeSubstructureMatchIter::countMatches (int max_embeddings)
{
   MatchCountContext context;
   context.embeddings_count = 0;
   context.max_count = max_embeddings;

   matcher.find_all_embeddings = true;
   matcher.cb_embedding = _matchCountEmbeddingsCallback;
   matcher.cb_embedding_context = &context;
   matcher.highlighting = 0;
   matcher.find();
   return context.embeddings_count;
}

//
// IndigoMoleculeSubstructureMatcher
//

IndigoMoleculeSubstructureMatcher::IndigoMoleculeSubstructureMatcher (Molecule &target) : 
   IndigoObject(MOLECULE_SUBSTRUCTURE_MATCHER),
   target(target)
{
}

IndigoMoleculeSubstructureMatcher::~IndigoMoleculeSubstructureMatcher ()
{
}

const char * IndigoMoleculeSubstructureMatcher::debugInfo ()
{
   return "<molecule substructure matcher>";
}

IndigoMoleculeSubstructureMatchIter* 
   IndigoMoleculeSubstructureMatcher::iterateQueryMatches (QueryMolecule &query, 
   bool embedding_edges_uniqueness)
{
   Molecule *target_prepared;
   Array<int> *mapping;
   if (MoleculeSubstructureMatcher::shouldUnfoldTargetHydrogens(query))
   {
      _target_arom_h_unfolded.clone(target, &_mapping_arom_h_unfolded, 0);
      target_prepared = &_target_arom_h_unfolded;
      mapping = &_mapping_arom_h_unfolded;
   }
   else
   {
      _target_arom.clone(target, &_mapping_arom, 0);
      target_prepared = &_target_arom;
      mapping = &_mapping_arom;
   }
   if (!target.isAromatized() && !target_prepared->isAromatized())
      target_prepared->aromatize();

   AutoPtr<IndigoMoleculeSubstructureMatchIter> 
      iter(new IndigoMoleculeSubstructureMatchIter(*target_prepared, query, target));

   iter->matcher.find_unique_embeddings = true;
   iter->matcher.find_unique_by_edges = embedding_edges_uniqueness;
   iter->matcher.restore_unfolded_h = false;
   iter->mapping.copy(*mapping);

   return iter.release();
}

CEXPORT int indigoSubstructureMatcher (int target)
{
   INDIGO_BEGIN
   {
      Molecule &mol = self.getObject(target).getMolecule();

      AutoPtr<IndigoMoleculeSubstructureMatcher> matcher(
         new IndigoMoleculeSubstructureMatcher(mol));

      return self.addObject(matcher.release());
   }
   INDIGO_END(-1)
}

static IndigoMoleculeSubstructureMatchIter* getMatchIterator (int target_matcher, int query, Indigo &self)
{
   IndigoObject &obj = self.getObject(target_matcher);
   if (obj.type != IndigoObject::MOLECULE_SUBSTRUCTURE_MATCHER)
      throw IndigoError("%s is not a matcher object", obj.debugInfo());

   IndigoMoleculeSubstructureMatcher &matcher = (IndigoMoleculeSubstructureMatcher &)obj;
   QueryMolecule &querymol = self.getObject(query).getQueryMolecule();
   return matcher.iterateQueryMatches(querymol, self.embedding_edges_uniqueness);
}

CEXPORT int indigoMatch (int target_matcher, int query)
{
   INDIGO_BEGIN
   {
      AutoPtr<IndigoMoleculeSubstructureMatchIter> 
         match_iter(getMatchIterator(target_matcher, query, self));

      if (!match_iter->hasNext())
         return 0;
      return self.addObject(match_iter->next());
   }
   INDIGO_END(-1)
}       

int indigoCountMatches (int target_matcher, int query)
{
   INDIGO_BEGIN
   {
      AutoPtr<IndigoMoleculeSubstructureMatchIter> 
         match_iter(getMatchIterator(target_matcher, query, self));

      return match_iter->countMatches(self.max_embeddings);
   }
   INDIGO_END(-1)
}

int indigoIterateMatches (int target_matcher, int query)
{
   INDIGO_BEGIN
   {
      AutoPtr<IndigoMoleculeSubstructureMatchIter> 
         match_iter(getMatchIterator(target_matcher, query, self));

      return self.addObject(match_iter.release());
   }
   INDIGO_END(-1)
}

CEXPORT int indigoCountComponents (int molecule)
{
   INDIGO_BEGIN
   {
      BaseMolecule &mol = self.getObject(molecule).getBaseMolecule();

      GraphDecomposer decomposer(mol);

      decomposer.decompose();
      
      return decomposer.getComponentsCount();
   }
   INDIGO_END(-1)
}

CEXPORT int indigoHasZCoord (int molecule)
{
   INDIGO_BEGIN
   {
      BaseMolecule &mol = self.getObject(molecule).getBaseMolecule();

      return BaseMolecule::hasZCoord(mol) ? 1 : 0;
   }
   INDIGO_END(-1)
}

CEXPORT int indigoCountStereocenters (int molecule)
{
   INDIGO_BEGIN
   {
      BaseMolecule &mol = self.getObject(molecule).getBaseMolecule();

      return mol.stereocenters.size();
   }
   INDIGO_END(-1)
}

CEXPORT int indigoBondOrder (int bondd)
{
   INDIGO_BEGIN
   {
      IndigoBond &bond = self.getObject(bondd).asBond();

      int num = bond.mol->getBondOrder(bond.idx);
      return num == -1 ? 0 : num;
   }
   INDIGO_END(-1);
}

CEXPORT int indigoBondStereo (int bondd)
{
   INDIGO_BEGIN
   {
      IndigoBond &bond = self.getObject(bondd).asBond();
      BaseMolecule &mol = *bond.mol;

      int dir = mol.stereocenters.getBondDirection(bond.idx);

      if (dir == MoleculeStereocenters::BOND_UP)
         return INDIGO_UP;
      if (dir == MoleculeStereocenters::BOND_DOWN)
         return INDIGO_DOWN;
      if (dir == MoleculeStereocenters::BOND_EITHER)
         return INDIGO_EITHER;

      int parity = mol.cis_trans.getParity(bond.idx);

      if (parity == MoleculeCisTrans::CIS)
         return INDIGO_CIS;
      if (parity == MoleculeCisTrans::TRANS)
         return INDIGO_TRANS;
      return 0;
   }
   INDIGO_END(-1);
}

CEXPORT int indigoGetAtom (int molecule, int idx)
{
   INDIGO_BEGIN
   {
      BaseMolecule &mol = self.getObject(molecule).getBaseMolecule();

      return self.addObject(new IndigoAtom(mol, idx));
   }
   INDIGO_END(-1)
}

CEXPORT int indigoGetBond (int molecule, int idx)
{
   INDIGO_BEGIN
   {
      BaseMolecule &mol = self.getObject(molecule).getBaseMolecule();

      return self.addObject(new IndigoBond(mol, idx));
   }
   INDIGO_END(-1)
}

IndigoAtomNeighbor::IndigoAtomNeighbor (BaseMolecule &mol_, int atom_idx, int bond_idx_) :
         IndigoAtom(mol_, atom_idx)
{
   type = ATOM_NEIGHBOR;

   bond_idx = bond_idx_;
}

IndigoAtomNeighbor::~IndigoAtomNeighbor ()
{
}

IndigoAtomNeighborsIter::IndigoAtomNeighborsIter (BaseMolecule *molecule, int atom_idx) :
         IndigoObject(ATOM_NEIGHBORS_ITER)
{
   _mol = molecule;
   _atom_idx = atom_idx;
   _nei_idx = -1;
}

IndigoAtomNeighborsIter::~IndigoAtomNeighborsIter ()
{
}

IndigoObject * IndigoAtomNeighborsIter::next ()
{
   const Vertex &vertex = _mol->getVertex(_atom_idx);

   if (_nei_idx == -1)
      _nei_idx = vertex.neiBegin();
   else if (_nei_idx != vertex.neiEnd())
      _nei_idx = vertex.neiNext(_nei_idx);

   if (_nei_idx == vertex.neiEnd())
      return 0;

   return new IndigoAtomNeighbor(*_mol, vertex.neiVertex(_nei_idx), vertex.neiEdge(_nei_idx));
}

bool IndigoAtomNeighborsIter::hasNext ()
{
   const Vertex &vertex = _mol->getVertex(_atom_idx);

   if (_nei_idx == -1)
      return vertex.neiBegin() != vertex.neiEnd();

   if (_nei_idx == vertex.neiEnd())
      return false;

   return vertex.neiNext(_nei_idx) != vertex.neiEnd();
}

CEXPORT int indigoIterateNeighbors (int atom)
{
   INDIGO_BEGIN
   {
      IndigoAtom &ia = IndigoAtom::cast(self.getObject(atom));
      
      return self.addObject(new IndigoAtomNeighborsIter(ia.mol, ia.idx));
   }
   INDIGO_END(-1)
}

CEXPORT int indigoBond (int nei)
{
   INDIGO_BEGIN
   {
      IndigoObject &obj = self.getObject(nei);

      if (obj.type != IndigoObject::ATOM_NEIGHBOR)
         throw IndigoError("indigoBond(): not applicable to %s", obj.debugInfo());

      IndigoAtomNeighbor &atomnei = (IndigoAtomNeighbor &)obj;

      return self.addObject(new IndigoBond(*atomnei.mol, atomnei.bond_idx));
   }
   INDIGO_END(-1)
}

CEXPORT float indigoAlignAtoms (int molecule, int natoms, int *atom_ids, float *desired_xyz)
{
   INDIGO_BEGIN
   {
      BaseMolecule &mol = self.getObject(molecule).getBaseMolecule();
      QS_DEF(Array<Vec3f>, points);
      QS_DEF(Array<Vec3f>, goals);
      int i;

      if (natoms < 1)
         throw IndigoError("indigoAlignAtoms(): can not align %d atoms", natoms);

      if (atom_ids == 0 || desired_xyz == 0)
         throw IndigoError("indigoAlignAtoms(): zero pointer given as input");
         
      points.clear();
      goals.clear();

      for (i= 0; i < natoms; i++)
      {
         points.push(mol.getAtomXyz(atom_ids[i]));
         goals.push(Vec3f(desired_xyz[i * 3], desired_xyz[i * 3 + 1], desired_xyz[i * 3 + 2]));
      }

      if (points.size() < 1)
         return true;

      float sqsum;
      Transform3f matr;

      if (!matr.bestFit(points.size(), points.ptr(), goals.ptr(), &sqsum))
         return false;

      for (i = mol.vertexBegin(); i != mol.vertexEnd(); i = mol.vertexNext(i))
         mol.getAtomXyz(i).transformPoint(matr);

      return (float)(sqrt(sqsum / natoms));
   }
   INDIGO_END(-1)
}

CEXPORT int indigoInvertStereo (int item)
{
   INDIGO_BEGIN
   {
      IndigoAtom &ia = IndigoAtom::cast(self.getObject(item));

      int k, *pyramid = ia.mol->stereocenters.getPyramid(ia.idx);
      if (pyramid == 0)
         throw IndigoError("indigoInvertStereo: not a stereoatom");
      __swap(pyramid[0], pyramid[1], k);
      return 1;
   }
   INDIGO_END(-1)
}
