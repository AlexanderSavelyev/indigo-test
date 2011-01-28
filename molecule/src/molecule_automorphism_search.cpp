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

#include "molecule/molecule_automorphism_search.h"

#include "base_cpp/profiling.h"
#include "graph/spanning_tree.h"
#include "molecule/molecule.h"
#include "molecule/molecule_scaffold_detection.h"
#include "graph/graph_decomposer.h"
#include "molecule/elements.h"

using namespace indigo;

MoleculeAutomorphismSearch::MoleculeAutomorphismSearch () :
   TL_CP_GET(_approximation_orbits),
   TL_CP_GET(_hcount),
   TL_CP_GET(_cistrans_stereo_bond_parity),
   TL_CP_GET(_degree),
   TL_CP_GET(_stereocenter_state),
   TL_CP_GET(_cistrans_bond_state)
{
   cb_vertex_cmp = _vertex_cmp;
   cb_edge_rank = _edge_rank;
   cb_check_automorphism = _check_automorphism;
   cb_compare_mapped = _compare_mapped;
   cb_automorphism = _automorphismCallback;
   context_automorphism = this;
   context = this;

   detect_invalid_stereocenters = false;
   detect_invalid_cistrans_bonds = false;
   find_canonical_ordering = true;
   _fixed_atom = -1;
}

void MoleculeAutomorphismSearch::_getFirstApproximation (Molecule &mol)
{
   _stereocenter_state.clear_resize(mol.vertexEnd());
   _cistrans_bond_state.clear_resize(mol.edgeEnd());

   const MoleculeStereocenters &stereocenters = mol.stereocenters;
   for (int i = 0; i < _stereocenter_state.size(); i++)
      _stereocenter_state[i] = _NO_STEREO;
   for (int i = stereocenters.begin(); i != stereocenters.end(); i = stereocenters.next(i))
   {
      int atom_index = stereocenters.getAtomIndex(i);
      _stereocenter_state[atom_index] = _UNDEF;
   }

   for (int i = 0; i < _cistrans_bond_state.size(); i++)
      _cistrans_bond_state[i] = _NO_STEREO;
   for (int i = mol.edgeBegin(); i != mol.edgeEnd(); i = mol.edgeNext(i))
      if (mol.cis_trans.getParity(i))
         _cistrans_bond_state[i] = _UNDEF;

   _cistrans_stereo_bond_parity.clear_resize(mol.edgeEnd());
   _cistrans_stereo_bond_parity.zerofill();

   _treat_undef_as = _INVALID;
   _target_stereocenter = -1;
   _target_bond = -1;

   // Approximation orbits cannot be used on this step because 
   // of existance of check_automorphism method. But order from 
   // first refinment can be used here.
   _approximation_orbits.fffill();

   profTimerStart(t0, "mol_auto.first_search");
   AutomorphismSearch::process(mol);
   profTimerStop(t0);

   getCanonicallyOrderedOrbits(_approximation_orbits);

   _findCisTransStereoBondParirties(mol);
}

void MoleculeAutomorphismSearch::_findCisTransStereoBondParirties (Molecule &mol)
{
   const MoleculeStereocenters &stereocenters = mol.stereocenters;

   // Mark edges that connects two stereocenters and that common parity can be detected
   for (int i = mol.edgeBegin(); i != mol.edgeEnd(); i = mol.edgeNext(i))
   {
      const Edge &edge = mol.getEdge(i);
      if (!stereocenters.exists(edge.beg) || !stereocenters.exists(edge.end))
         continue;
      if (stereocenters.getGroup(edge.beg) != stereocenters.getGroup(edge.end))
         continue;
      if (stereocenters.getType(edge.beg) != stereocenters.getType(edge.end))
         continue;

      // Both stereocenters exists and groups are the same

      int orb_beg = _approximation_orbits[edge.beg];
      int orb_end = _approximation_orbits[edge.end];

      _approximation_orbits[edge.beg] = -2; // Some value different from -1 and other orbits
      _approximation_orbits[edge.end] = -2;

      int parity_beg, parity_end;
      if (_validStereocenterByAtom(edge.beg, _approximation_orbits, &parity_beg) == _VALID &&
         _validStereocenterByAtom(edge.end, _approximation_orbits, &parity_end) == _VALID)
      {
         // 1 - means that both stereocenters have the same parity
         _cistrans_stereo_bond_parity[i] = -parity_beg * parity_end;
      }

      // Restore orbits
      _approximation_orbits[edge.beg] = orb_beg;
      _approximation_orbits[edge.end] = orb_end;
   }
}

void MoleculeAutomorphismSearch::_initialize (Molecule &mol)
{
   _fixed_atom = -1;
   _calculateHydrogensAndDegree(mol);

   // Initialize first orbits approximation to -1
   _approximation_orbits.clear_resize(mol.vertexEnd());
   _approximation_orbits.fffill();

   // Get first approximation without stereocenters if 
   // detect_invalid_stereocenters or detect_invalid_cistrans_bonds is set
   _getFirstApproximation(mol);
}

void MoleculeAutomorphismSearch::process (Molecule &mol)
{
   _initialize(mol);

   if (detect_invalid_stereocenters || detect_invalid_cistrans_bonds)
   {
      // Mark stereocenters that are valid
      _markValidOrInvalidStereo(true, _approximation_orbits, NULL);
      _markValidOrInvalidStereo(false, _approximation_orbits, NULL);

      // Temposrary solution: mark unknown stereocenters as valid if 
      // number of them is more then 1 in each component
      _markComplicatedStereocentersAsValid(mol);

      // Find invalid stereocenters and bonds iteratively
      while (_findInvalidStereo(mol))
         ;
   }

   // Mark all other stereocenters and stereobonds as valid
   const MoleculeStereocenters &stereocenters = mol.stereocenters;
   for (int i = stereocenters.begin(); i != stereocenters.end(); i = stereocenters.next(i))
   {
      int atom_index = stereocenters.getAtomIndex(i);
      if (_stereocenter_state[atom_index] == _UNDEF)
         _stereocenter_state[atom_index] = _VALID;
   }
   for (int i = mol.edgeBegin(); i != mol.edgeEnd(); i = mol.edgeNext(i))
      if (_cistrans_bond_state[i] == _UNDEF && mol.cis_trans.getParity(i) != 0)
         _cistrans_bond_state[i] = _VALID;

   // Find final orbits and canonical ordering with found 
   // valid and invalid stereocenters and bonds
   _target_stereocenter = -1;
   _target_bond = -1;
   _fixed_atom = -1;

   if (find_canonical_ordering)
   {
      profTimerStart(t0, "mol_auto.final_search");
      AutomorphismSearch::process(mol);
      profTimerStop(t0);
   }
}

bool MoleculeAutomorphismSearch::_hasStereo (Molecule &mol)
{
   for (int i = mol.edgeBegin(); i != mol.edgeEnd(); i = mol.edgeNext(i))
      if (mol.cis_trans.getParity(i) != 0)
         return true;
   if (mol.stereocenters.size() != 0)
      return true;
   return false;
}

int MoleculeAutomorphismSearch::_vertex_cmp (Graph &graph, int v1, int v2, const void *context)
{
   const MoleculeAutomorphismSearch &self = *(MoleculeAutomorphismSearch *)context;
   Molecule &mol = (Molecule &)graph;
   int ret;

   // Compare orbits of the first approximation
   ret = self._approximation_orbits[v1] - self._approximation_orbits[v2];
   if (ret != 0)
      return ret;

   ret = Molecule::matchAtomsCmp(graph, graph, v1, v2, 0);
   if (ret != 0)
      return ret;

   if (self._hcount[v1] < self._hcount[v2])
      return 1;
   if (self._hcount[v1] > self._hcount[v2])
      return -1;

   // DPX: r-sites logic removed from here

   // Stereo should be compared at the end because it is 
   // refinement on the first approximation 
   ret = _compareStereo(mol, v1, v2, context);
   if (ret != 0)
      return -ret;

   return 0;
}

int MoleculeAutomorphismSearch::_edge_rank (Graph &graph, int edge_idx, const void *context)
{
   const MoleculeAutomorphismSearch &self = *(const MoleculeAutomorphismSearch *)context;
   Molecule &mol = (Molecule &)graph;

   if (self._cistrans_stereo_bond_parity[edge_idx] != 0)
   {
      // Check parity
      const Edge &edge = mol.getEdge(edge_idx);
      bool beg_valid = (self._getStereo(self._stereocenter_state[edge.beg]) == _VALID);
      bool end_valid = (self._getStereo(self._stereocenter_state[edge.beg]) == _VALID);
      if (beg_valid && end_valid)
      {
         if (self._cistrans_stereo_bond_parity[edge_idx] == 1)
            return 5;
         else 
            return 6;
      }
   }

   return mol.getBondOrder(edge_idx);
}

bool MoleculeAutomorphismSearch::_check_automorphism (Graph &graph, const Array<int> &mapping, const void *context)
{
   const MoleculeAutomorphismSearch &self = *(MoleculeAutomorphismSearch *)context;
   Molecule &mol = (Molecule &)graph;

   int i;

   for (i = mol.edgeBegin(); i != mol.edgeEnd(); i = mol.edgeNext(i))
   {
      const Edge &edge = mol.getEdge(i);

      if (mapping[edge.beg] == -1 || mapping[edge.end] == -1)
         continue;

      int edge_idx = mol.findEdgeIndex(mapping[edge.beg], mapping[edge.end]);

      if (edge_idx == -1)
         throw Error("internal error: this should be checked in AutomorphismSearch");

      if (mol.getBondOrder(i) != mol.getBondOrder(edge_idx))
         return false;
   }

   for (i = mol.edgeBegin(); i != mol.edgeEnd(); i = mol.edgeNext(i))
   {
      if (self._getStereo(self._cistrans_bond_state[i]) != _VALID || mol.cis_trans.getParity(i) == 0)
         continue;

      if (!_isCisTransBondMappedRigid(mol, i, mapping.ptr()))
         return false;
   }

   if (!self._checkStereocentersAutomorphism(mol, mapping))
      return false;

   return true;
}

bool MoleculeAutomorphismSearch::_checkStereocentersAutomorphism (Molecule &mol,
      const Array<int> &mapping) const
{
   const MoleculeStereocenters &stereocenters = mol.stereocenters;
   if (stereocenters.size() != 0)
   {
      Filter stereocenters_valid_filter;
      if (_treat_undef_as == _VALID)
         stereocenters_valid_filter.init(_stereocenter_state.ptr(), Filter::NEQ, _INVALID);
      else
         stereocenters_valid_filter.init(_stereocenter_state.ptr(), Filter::EQ, _VALID);

      bool ret = MoleculeStereocenters::checkSub(stereocenters, stereocenters, 
         mapping.ptr(), false, &stereocenters_valid_filter);
      if (!ret)
         return false;

      QS_DEF(Array<int>, inv_mapping);
      inv_mapping.clear_resize(mol.vertexEnd());
      inv_mapping.fffill();
      for (int i = mol.vertexBegin(); i != mol.vertexEnd(); i = mol.vertexNext(i))
      {
         if (mapping[i] != -1)
            inv_mapping[mapping[i]] = i;
      }

      ret = MoleculeStereocenters::checkSub(stereocenters, stereocenters, 
         inv_mapping.ptr(), false, &stereocenters_valid_filter);
      if (!ret)
         return false;
   }
   return true;
}


int MoleculeAutomorphismSearch::_compare_mapped (Graph &graph, const Array<int> &mapping1, const Array<int> &mapping2, const void *context)
{
   const MoleculeAutomorphismSearch &self = *(MoleculeAutomorphismSearch *)context;
   Molecule &mol = (Molecule &)graph;
   int i;

   QS_DEF(Array<int>, inv_mapping1);
   QS_DEF(Array<int>, inv_mapping2);

   QS_DEF(Array<int>, tmp1);
   QS_DEF(Array<int>, tmp2);

   inv_mapping1.clear_resize(graph.vertexEnd());
   inv_mapping2.clear_resize(graph.vertexEnd());

   inv_mapping1.fffill();
   inv_mapping2.fffill();

   tmp1.clear_resize(graph.vertexEnd());
   tmp2.clear_resize(graph.vertexEnd());

   for (i = 0; i < mapping1.size(); i++)
   {
      inv_mapping1[mapping1[i]] = i;
      inv_mapping2[mapping2[i]] = i;
   }

   int min_diff_beg = graph.vertexEnd();
   int min_diff_end = graph.vertexEnd();
   int diff_sign = 0;

   for (i = graph.edgeBegin(); i != graph.edgeEnd(); i = graph.edgeNext(i))
   {
      const Edge &edge = graph.getEdge(i);

      int beg1 = inv_mapping1[edge.beg], end1 = inv_mapping1[edge.end];

      if (beg1 == -1 || end1 == -1)
         continue;

      int beg2 = inv_mapping2[edge.beg], end2 = inv_mapping2[edge.end], tmp;

      if (beg1 > end1)
         __swap(beg1, end1, tmp);
      if (beg2 > end2)
         __swap(beg2, end2, tmp);

      bool ok1 = (beg1 < min_diff_beg || (beg1 == min_diff_beg && end1 < min_diff_end));
      bool ok2 = (beg2 < min_diff_beg || (beg2 == min_diff_beg && end2 < min_diff_end));

      if (!ok1 && !ok2)
         continue;

      int type1, type2;
      int mapped_12 = graph.findEdgeIndex(mapping2[beg1], mapping2[end1]);
      int mapped_21 = graph.findEdgeIndex(mapping1[beg2], mapping1[end2]);

      type1 = type2 = mol.getBondOrder(i);

      int mapped_12_type = (mapped_12 < 0) ? 0 : mol.getBondOrder(mapped_12);
      int mapped_21_type = (mapped_21 < 0) ? 0 : mol.getBondOrder(mapped_21);

      int parity = mol.cis_trans.getParity(i);
      if (parity != 0 && self._getStereo(self._cistrans_bond_state[i]) == _VALID)
      {
         int subst[4];

         memcpy(subst, mol.cis_trans.getSubstituents(i), 4 * sizeof(int));
         if (inv_mapping1[subst[0]] >= 0 && inv_mapping1[subst[2]] >= 0)
         {
            if (subst[1] >= 0 && inv_mapping1[subst[1]] < 0)
               subst[1] = -1;
            if (subst[3] >= 0 && inv_mapping1[subst[3]] < 0)
               subst[3] = -1;

            int parity1 = MoleculeCisTrans::applyMapping(parity, subst, inv_mapping1.ptr());
            type1 += parity1 * 10;
         }

         memcpy(subst, mol.cis_trans.getSubstituents(i), 4 * sizeof(int));
         if (inv_mapping2[subst[0]] >= 0 && inv_mapping2[subst[2]] >= 0)
         {
            if (subst[1] >= 0 && inv_mapping2[subst[1]] < 0)
               subst[1] = -1;
            if (subst[3] >= 0 && inv_mapping2[subst[3]] < 0)
               subst[3] = -1;

            int parity2 = MoleculeCisTrans::applyMapping(parity, subst, inv_mapping2.ptr());
            type2 += parity2 * 10;
         }

         int p12 = mol.cis_trans.getParity(mapped_12);

         if (mapped_12 >= 0 && p12 > 0)
         {
            memcpy(subst, mol.cis_trans.getSubstituents(mapped_12), 4 * sizeof(int));

            if (inv_mapping2[subst[0]] >= 0 && inv_mapping2[subst[2]] >= 0)
            {
               if (subst[1] >= 0 && inv_mapping2[subst[1]] < 0)
                  subst[1] = -1;
               if (subst[3] >= 0 && inv_mapping2[subst[3]] < 0)
                  subst[3] = -1;

               int parity_12 = MoleculeCisTrans::applyMapping(p12, subst, inv_mapping2.ptr());
               mapped_12_type += parity_12 * 10;
            }
         }

         int p21 = mol.cis_trans.getParity(mapped_21);

         if (mapped_21 >= 0 && p21 > 0)
         {
            memcpy(subst, mol.cis_trans.getSubstituents(mapped_21), 4 * sizeof(int));

            if (inv_mapping1[subst[0]] >= 0 && inv_mapping1[subst[2]] >= 0)
            {
               if (subst[1] >= 0 && inv_mapping1[subst[1]] < 0)
                  subst[1] = -1;
               if (subst[3] >= 0 && inv_mapping1[subst[3]] < 0)
                  subst[3] = -1;

               int parity_21 = MoleculeCisTrans::applyMapping(p21, subst, inv_mapping1.ptr());
               mapped_21_type += parity_21 * 10;
            }
         }
      }

      if (type1 > mapped_12_type)
      {
         if (ok1)
         {
            min_diff_beg = beg1;
            min_diff_end = end1;
            diff_sign = 1;
         }
      }
      if (type2 > mapped_21_type)
      {
         if (beg2 < min_diff_beg || (beg2 == min_diff_beg && end2 < min_diff_end))
         {
            min_diff_beg = beg2;
            min_diff_end = end2;
            diff_sign = -1;
         }
      }
   }

   if (diff_sign != 0)
      return diff_sign;

   return self._compareMappedStereocenters(mol, mapping1, mapping2, inv_mapping1, inv_mapping2);
}

int MoleculeAutomorphismSearch::_compareMappedStereocenters (Molecule &mol,
      const Array<int> &mapping1, const Array<int> &mapping2,
      const Array<int> &inv_mapping1, const Array<int> &inv_mapping2) const
{
   MoleculeStereocenters &stereocenters = mol.stereocenters;

   if (stereocenters.size() == 0)
      return 0;

   int max_stereogroup = 0;
   for (int s = stereocenters.begin(); s != stereocenters.end(); s = stereocenters.next(s))
   {
      int atom_idx = stereocenters.getAtomIndex(s);
      max_stereogroup = __max(stereocenters.getGroup(atom_idx), max_stereogroup);
   }

   int groups_count = 2 * (max_stereogroup + 1);
   QS_DEF(Array<int>, stereogroup1_rank);
   QS_DEF(Array<int>, stereogroup2_rank);
   QS_DEF(Array<int>, stereogroup1_parity_mod);
   QS_DEF(Array<int>, stereogroup2_parity_mod);
   stereogroup1_rank.clear_resize(groups_count);
   stereogroup1_rank.fffill();
   stereogroup2_rank.clear_resize(groups_count);
   stereogroup2_rank.fffill();
   stereogroup1_parity_mod.clear_resize(groups_count);
   stereogroup1_parity_mod.fffill();
   stereogroup2_parity_mod.clear_resize(groups_count);
   stereogroup2_parity_mod.fffill();

   for (int i = 0; i < mapping1.size(); i++)
   {
      int type1 = stereocenters.getType(mapping1[i]);
      int type2 = stereocenters.getType(mapping2[i]);
      if (_getStereo(_stereocenter_state[mapping1[i]]) == _INVALID)
         type1 = 0;
      if (_getStereo(_stereocenter_state[mapping2[i]]) == _INVALID)
         type2 = 0;

      if (type1 != type2)
         throw Error("internal: stereocenter types mismatch");

      if (type1 < MoleculeStereocenters::ATOM_AND)
         continue;

      int pyramid1[4], pyramid2[4];

      memcpy(pyramid1, stereocenters.getPyramid(mapping1[i]), 4 * sizeof(int));
      memcpy(pyramid2, stereocenters.getPyramid(mapping2[i]), 4 * sizeof(int));

      int size1 = 0, size2 = 0;

      for (int j = 0; j < 4; j++)
      {
         if (pyramid1[j] >= 0)
         {
            if (inv_mapping1[pyramid1[j]] >= 0)
               size1++;
            else
               pyramid1[j] = -1;
         }
         if (pyramid2[j] >= 0)
         {
            if (inv_mapping2[pyramid2[j]] >= 0)
               size2++;
            else
               pyramid2[j] = -1;
         }
      }

      if (size1 != size2)
         throw Error("internal: stereocenter sizes mismatch");

      bool rigid1 = true, rigid2 = true;

      if (size1 >= 3)
      {
         if (size1 == 3)
            MoleculeStereocenters::moveImplicitHydrogenToEnd(pyramid1);

         for (int j = 0; j < size1; j++)
            pyramid1[j] = inv_mapping1[pyramid1[j]];

         rigid1 = MoleculeStereocenters::isPyramidMappingRigid(pyramid1);
      }
      if (size2 >= 3)
      {
         if (size2 == 3)
            MoleculeStereocenters::moveImplicitHydrogenToEnd(pyramid2);

         for (int j = 0; j < size2; j++)
            pyramid2[j] = inv_mapping2[pyramid2[j]];

         rigid2 = MoleculeStereocenters::isPyramidMappingRigid(pyramid2);
      }

      int group1 = stereocenters.getGroup(mapping1[i]);
      int group2 = stereocenters.getGroup(mapping2[i]);

      // Encode group with type into one index
      int type_group1 = 2 * group1 + (type1 == MoleculeStereocenters::ATOM_AND ? 0 : 1);
      int type_group2 = 2 * group2 + (type2 == MoleculeStereocenters::ATOM_AND ? 0 : 1);

      if (type1 == MoleculeStereocenters::ATOM_AND || type1 == MoleculeStereocenters::ATOM_OR)
      {
         int &parity_mod1 = stereogroup1_parity_mod[type_group1];
         int &parity_mod2 = stereogroup2_parity_mod[type_group2];

         // First stereocenter in group always maps rigid
         if (parity_mod1 == -1)
            parity_mod1 = rigid1 ? 0 : 1;
         if (parity_mod2 == -1)
            parity_mod2 = rigid2 ? 0 : 1;

         if (parity_mod1 == 1)
            // Invert sterecenter
            rigid1 = !rigid1;

         if (parity_mod2 == 1)
            // Invert sterecenter
            rigid2 = !rigid2;
      }

      if (rigid1 && !rigid2)
         return 1;
      if (rigid2 && !rigid1)
         return -1;

      // Compare stereogroups
      if (type1 == MoleculeStereocenters::ATOM_AND || type1 == MoleculeStereocenters::ATOM_OR)
      {
         int &rank1 = stereogroup1_rank[type_group1];
         int &rank2 = stereogroup2_rank[type_group2];

         if (rank1 == -1)
            rank1 = i;
         if (rank2 == -1)
            rank2 = i;

         int diff = rank1 - rank2;
         if (diff != 0)
            return diff;
      }
   }

   return 0;
}


int MoleculeAutomorphismSearch::_validCisTransBond (int idx, const Array<int> &orbits)
{
   Molecule &mol = *(Molecule *)_given_graph;

   int parity = mol.cis_trans.getParity(idx);

   if (parity == 0)
      return _UNDEF;

   const int *subst = mol.cis_trans.getSubstituents(idx);

   // Stereobond can be valid even if substituents are in the same orbit.
   // For example: C\C=C1/C\C(C1)=C\C
   // But if substituents are in the same orbit and have degree 1 then such 
   // cis-trans bond is invalid.
   // To detect invalid stereocenters automorphism group structure shold be investigated.
   if (subst[1] != -1)
      if (orbits[subst[0]] == orbits[subst[1]])
      {
         if (_degree[subst[0]] == 1)
            return _INVALID;
         return _UNDEF;
      }

   if (subst[3] != -1)
      if (orbits[subst[2]] == orbits[subst[3]])
      {
         if (_degree[subst[2]] == 1)
            return _INVALID;
         return _UNDEF;
      }

   return _VALID;
}

int MoleculeAutomorphismSearch::_validStereocenterByAtom (int atom_index, Array<int> &orbits, int *parity)
{
   Molecule &mol = *(Molecule *)_given_graph;

   int type = mol.stereocenters.getType(atom_index);
   if (type == 0)
      return _UNDEF;

   const int *pyramid = mol.stereocenters.getPyramid(atom_index);
   int substituents[4];

   for (int i = 0; i < 4; i++)
      if (pyramid[i] != -1)
         substituents[i] = orbits[pyramid[i]];
      else
         substituents[i] = -1;

   // Check if orbits for substituents are different.
   // Even if substituents are in the same orbit stereocenter can be valid.
   // But if substituents are in the same orbit and have degree 1 then such 
   // stereocenter is invalid.
   for (int i = 0; i < 4; i++)
   {
      if (substituents[i] == -1)
         continue;
      int same_count = 0;
      for (int j = 0; j < 4; j++)
         if (substituents[i] == substituents[j])
            same_count++;

      if (same_count != 1)
      {
         if (_degree[pyramid[i]] == 1)
            return _INVALID;
         else
            return _UNDEF;
      }
   }

   // Detect parity 
   if (parity != NULL)
   {
      if (MoleculeStereocenters::isPyramidMappingRigid(substituents))
         *parity = 1;
      else
         *parity = -1;
   }

   return _VALID;
}


int MoleculeAutomorphismSearch::_validStereocenter (int idx, Array<int> &orbits, int *parity)
{
   Molecule &mol = *(Molecule *)_given_graph;

   int atom_index = mol.stereocenters.getAtomIndex(idx);
   return _validStereocenterByAtom(atom_index, orbits, parity);
}

bool MoleculeAutomorphismSearch::invalidCisTransBond (int idx)
{
   if (_cistrans_bond_state[idx] == _UNDEF)
      throw Error("Internal error: stereocenters state must be know here");
   return (_cistrans_bond_state[idx] == _INVALID);
}

bool MoleculeAutomorphismSearch::invalidStereocenter (int idx)
{
   if (_stereocenter_state[idx] == _UNDEF)
      throw Error("Internal error: stereocenters state must be know here");
   return (_stereocenter_state[idx] == _INVALID);
}

int MoleculeAutomorphismSearch::_compareStereo (Molecule &mol, int v1, int v2, const void *context)
{
   const MoleculeAutomorphismSearch &self = *(MoleculeAutomorphismSearch *)context;

   int diff;

   if (self._fixed_atom != -1)
   {
      int t1 = (self._fixed_atom == v1) ? 1 : 0;
      int t2 = (self._fixed_atom == v2) ? 1 : 0;
      diff = t1 - t2;
      if (diff != 0)
         return diff;
   }

   const MoleculeStereocenters &stereocenters = mol.stereocenters;

   int s1 = self._getStereo(self._stereocenter_state[v1]);
   int s2 = self._getStereo(self._stereocenter_state[v2]);

   diff = s1 - s2;
   if (diff != 0)
      return diff;

   if (s1 != _VALID)
      // Both stereocenters are marked as invalid
      return 0;

   diff = stereocenters.getType(v1) - stereocenters.getType(v2);
   if (diff != 0)
      return diff;

   return 0;
}

void MoleculeAutomorphismSearch::_calculateHydrogensAndDegree (Molecule &mol)
{
   _hcount.clear_resize(mol.vertexEnd());
   _degree.clear_resize(mol.vertexEnd());
   _degree.zerofill();

   for (int i = mol.vertexBegin(); i != mol.vertexEnd(); i = mol.vertexNext(i))
   {
      if (mol.isRSite(i) || mol.isPseudoAtom(i))
         _hcount[i] = 0;
      else
         _hcount[i] = mol.getImplicitH(i);

      if (_hcount[i] < 0)
      {
         if (mol.getAtomAromaticity(i) == ATOM_AROMATIC)
         {
            if (mol.getAtomNumber(i) == ELEM_C && mol.getAtomCharge(i) == 0)
            {
               if (mol.getVertex(i).degree() == 3)
                  _hcount[i] = 0;
               else if (mol.getVertex(i).degree() == 2)
                  _hcount[i] = 1;
            }
            else if (mol.getAtomNumber(i) == ELEM_O && mol.getAtomCharge(i) == 0)
               _hcount[i] = 0;
         }
      }

      if (_hcount[i] < 0)
         throw Error("unsure hydrogen count on atom #%d", i);
      
      const Vertex &vertex = mol.getVertex(i);

      _degree[i] = 0;
      if (ignored_vertices != 0 && ignored_vertices[i])
         continue;

      for (int j = vertex.neiBegin(); j != vertex.neiEnd(); j = vertex.neiNext(j))
      {
         if (mol.getAtomNumber(vertex.neiVertex(j)) == ELEM_H &&
             mol.getAtomIsotope(vertex.neiVertex(j)) == 0)
            _hcount[i]++;

         if (ignored_vertices == 0 || ignored_vertices[vertex.neiVertex(j)] == 0)
            _degree[i]++;
      }
   }
}

void MoleculeAutomorphismSearch::_markValidOrInvalidStereo (bool find_valid, 
                                                            Array<int> &orbits, bool *found)
{
   Molecule &mol = *(Molecule *)_given_graph;

   // If all substituents are belong to the different orbits then
   // such stereocenter is valid
   // If some substituents are belong to the one orbit and has one 
   // degree then such stereocenter is invalid

   for (int i = mol.edgeBegin(); i != mol.edgeEnd(); i = mol.edgeNext(i))
   {
      if (mol.cis_trans.getParity(i) == 0)
         continue;
      int validity = _validCisTransBond(i, orbits);
      if (validity == _UNDEF)
         continue;
      if (find_valid == (validity == _VALID))
      {
         if (_cistrans_bond_state[i] != validity)
         {
            _cistrans_bond_state[i] = validity;
            if (found != 0)
               *found = true;
         }
      }
   }

   const MoleculeStereocenters &stereocenters = mol.stereocenters;
   for (int i = stereocenters.begin(); i != stereocenters.end(); i = stereocenters.next(i))
   {
      int validity = _validStereocenter(i, orbits);
      if (validity == _UNDEF)
         continue;
      if (find_valid == (validity == _VALID))
      {
         int atom_index = stereocenters.getAtomIndex(i);
         if (_stereocenter_state[atom_index] != validity)
         {
            _stereocenter_state[atom_index] = validity;
            if (found != 0)
               *found = true;
         }
      }
   }
}

int MoleculeAutomorphismSearch::_getStereo (int state) const
{
   if (state == _NO_STEREO)
      return _INVALID;
   if (state != _UNDEF)
      return state;
   if (_treat_undef_as != -1)
      return _treat_undef_as;
   return state;
}

void MoleculeAutomorphismSearch::_automorphismCallback (const int *automorphism, void *context)
{
   MoleculeAutomorphismSearch &self = *(MoleculeAutomorphismSearch *)context;
   Molecule &mol = *(Molecule *)self._given_graph;

   const MoleculeStereocenters &stereocenters = mol.stereocenters;

   if (self._target_stereocenter != -1)
      if (!_isStereocenterMappedRigid(stereocenters, self._target_stereocenter, automorphism))
         self._target_stereocenter_parity_inv = true;

   if (self._target_bond != -1)
      if (!_isCisTransBondMappedRigid(mol, self._target_bond, automorphism))
         self._target_bond_parity_inv = true;
}

bool MoleculeAutomorphismSearch::_isStereocenterMappedRigid (const MoleculeStereocenters &stereocenters, 
                                                             int i, const int *mapping)
{
   int idx, type, group;
   int pyramid[4];
   int j;

   stereocenters.get(i, idx, type, group, pyramid);

   if (mapping[idx] == -1)
      return true;

   int size = 0;

   for (j = 0; j < 4; j++)
      if (pyramid[j] >= 0)
      {
         if (mapping[pyramid[j]] >= 0)
            size++;
         else
            pyramid[j] = -1;
      }

   if (size < 3)
      return true;

   if (type < MoleculeStereocenters::ATOM_AND)
      return true;

   if (stereocenters.getType(mapping[idx]) != type)
      throw Error("internal: stereocenter types mismatch");

   int pyra_map[4];

   MoleculeStereocenters::getPyramidMapping(stereocenters, stereocenters, idx, mapping, pyra_map, false);

   if (!MoleculeStereocenters::isPyramidMappingRigid(pyra_map))
      return false;

   return true;
}

bool MoleculeAutomorphismSearch::_isCisTransBondMappedRigid (Molecule &mol, int i, const int *mapping)
{
   const Edge &edge = mol.getEdge(i);
   int parity = mol.cis_trans.getParity(i);
   int subst[4];

   memcpy(subst, mol.cis_trans.getSubstituents(i), sizeof(int) * 4);

   if (mapping[subst[0]] < 0 || mapping[subst[2]] < 0)
      return true;

   if (subst[1] >= 0 && mapping[subst[1]] < 0)
      subst[1] = -1;
   if (subst[3] >= 0 && mapping[subst[3]] < 0)
      subst[3] = -1;

   int i2 = mol.findEdgeIndex(mapping[edge.beg], mapping[edge.end]);
   int parity2 = MoleculeCisTrans::applyMapping(parity, subst, mapping);

   if (mol.cis_trans.getParity(i2) != parity2)
      return false;

   return true;
}

bool MoleculeAutomorphismSearch::_findInvalidStereo (Molecule &mol)
{
   bool invalid_found = false;

   // Mark stereocenters that are invalid
   _treat_undef_as = _VALID;

   /* Optimization is disabled because it is impossible to detect 
    * stereocenter or stereobond validity only by orbits. To detect
    * validity on this step automorphism group should be investigated.

   AutomorphismSearch::process(mol);

   QS_DEF(Array<int>, orbits_stereo_restricted);
   getOrbits(orbits_stereo_restricted);
   _markValidOrInvalidStereo(false, orbits_stereo_restricted, &invalid_found);
   */

   // Check each possible stereocenters to be valid
   QS_DEF(Array<int>, invalid_stereo);

   //
   // Step 1: find valid and invalid stereocenters
   //

   QS_DEF(Array<int>, stereocenters_to_restore);

   invalid_stereo.clear();
   _target_bond = -1;
   MoleculeStereocenters &stereocenters = mol.stereocenters;
   for (int i = stereocenters.begin(); i != stereocenters.end(); i = stereocenters.next(i))
   {
      int atom_index = stereocenters.getAtomIndex(i);

      if (_stereocenter_state[atom_index] != _UNDEF)
         continue;

      _stereocenter_state[atom_index] = _INVALID;

      _target_stereocenter = i;
      _fixed_atom = atom_index;
      _target_stereocenter_parity_inv = false;

      // If AND or OR stereogroup is fixed then mark such stereogroup as ABS
      int type = stereocenters.getType(atom_index);
      int group = stereocenters.getGroup(atom_index);

      stereocenters_to_restore.clear();
      if (type == MoleculeStereocenters::ATOM_AND || type == MoleculeStereocenters::ATOM_OR)
      {
         // Mark whole group as absolute
         for (int j = stereocenters.begin(); j != stereocenters.end(); j = stereocenters.next(j))
         {
            int atom_index2 = stereocenters.getAtomIndex(j);
            if (atom_index2 == atom_index)
               continue;
            if (stereocenters.getGroup(atom_index) == stereocenters.getGroup(atom_index2) &&
                  stereocenters.getType(atom_index2) == type)
            {
               stereocenters.setType(atom_index2, MoleculeStereocenters::ATOM_ABS, -1);
               stereocenters_to_restore.push(atom_index2);
            }
         }
      }

      AutomorphismSearch::process(mol);

      if (_target_stereocenter_parity_inv)
         // Stereocenter is invalid
         invalid_stereo.push(atom_index);
      else
         _stereocenter_state[atom_index] = _UNDEF;

      for (int j = 0; j < stereocenters_to_restore.size(); j++)
      {
         int atom = stereocenters_to_restore[j];
         stereocenters.setType(atom, type, group);
      }
   }

   // Mark invalid stereocenters
   for (int i = 0; i < invalid_stereo.size(); i++)
   {
      int atom_index = invalid_stereo[i];
      _stereocenter_state[atom_index] = _INVALID;
      invalid_found = true;
   }

   //
   // Step 2: find valid and invalid cis-trans bonds
   //

   invalid_stereo.clear();
   _target_stereocenter = -1;
   for (int i = mol.edgeBegin(); i != mol.edgeEnd(); i = mol.edgeNext(i))
   {
      if (_cistrans_bond_state[i] != _UNDEF || mol.cis_trans.getParity(i) == 0)
         continue;

      _cistrans_bond_state[i] = _INVALID;

      _target_bond = i;
      _target_bond_parity_inv = false;
      _fixed_atom = mol.getEdge(i).beg;

      AutomorphismSearch::process(mol);

      if (_target_bond_parity_inv)
         // Stereocenter is invalid
         invalid_stereo.push(i);

      _cistrans_bond_state[i] = _UNDEF;
   }

   // Mark invalid stereocenters
   for (int i = 0; i < invalid_stereo.size(); i++)
   {
      int bond_index = invalid_stereo[i];
      _cistrans_bond_state[bond_index] = _INVALID;
      invalid_found = true;
   }

   return invalid_found;
}

void MoleculeAutomorphismSearch::_markComplicatedStereocentersAsValid (Molecule &mol)
{
   // If there is more then 1 unknown stereocenter in the biconnected 
   // component then validity of such stereocenters cannot be found 
   // with current methods. For example C[C@H]1C[C@@H](C)C[C@H](C)C1 
   // can lead to CC1C[C@H](C)C[C@@H](C)C1 or CC1C[C@@H](C)C[C@H](C)C1, 
   // but canonical codes are different for such molecules.
   QS_DEF(Array<int>, single_bond_bridge_mark);
   single_bond_bridge_mark.resize(mol.edgeEnd());
   single_bond_bridge_mark.fill(1);

   SpanningTree sp_tree(mol, 0);
   sp_tree.markAllEdgesInCycles(single_bond_bridge_mark.ptr(), 0);
   for (int i = mol.edgeBegin(); i != mol.edgeEnd(); i++)
      if (mol.getBondOrder(i) != BOND_SINGLE)
         single_bond_bridge_mark[i] = 0;

   Filter edge_filter(single_bond_bridge_mark.ptr(), Filter::NEQ, 1);
   GraphDecomposer decomposer(mol);
   decomposer.decompose(0, &edge_filter);

   const Array<int> &decomposition = decomposer.getDecomposition();

   QS_DEF(Array<int>, undef_stereo_in_component);
   undef_stereo_in_component.clear();

   for (int v = mol.vertexBegin(); v != mol.vertexEnd(); v = mol.vertexNext(v))
   {
      int comp = decomposition[v];

      if (comp < 0)
         continue;
      while (undef_stereo_in_component.size() <= comp)
         undef_stereo_in_component.push(0);

      if (_stereocenter_state[v] == _UNDEF)
         undef_stereo_in_component[comp]++;
   }

   // Mark stereocenters as valid if there are more then 1 
   // undefined stereocenter in the component
   for (int v = mol.vertexBegin(); v != mol.vertexEnd(); v = mol.vertexNext(v))
   {
      int comp = decomposition[v];

      if (comp < 0)
         continue;
      if (_stereocenter_state[v] == _UNDEF && undef_stereo_in_component[comp] > 1)
         _stereocenter_state[v] = _VALID;
   }
}
