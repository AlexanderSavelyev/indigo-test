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

#include "molecule/molecule_rgroups.h"
#include "base_cpp/auto_ptr.h"
#include "molecule/query_molecule.h"

using namespace indigo;

RGroup::RGroup () : if_then(0), rest_h(0)
{
}

RGroup::~RGroup ()
{
}

void RGroup::clear()
{
   if_then = 0;
   rest_h = 0;
   occurrence.clear();
   fragments.clear();
}

void RGroup::copy (RGroup &other)
{
   if_then = other.if_then;
   rest_h = other.rest_h;
   occurrence.copy(other.occurrence);
   for (int i = 0; i < other.fragments.size(); i++)
   {
      AutoPtr<QueryMolecule> new_fragment(new QueryMolecule());

      new_fragment->clone(*other.fragments[i], 0, 0);
      fragments.add(new_fragment.release());
   }
}

bool RGroup::occurrenceSatisfied (int value) const
{
   for (int i = 0; i < occurrence.size(); i++)
      if (value >= (occurrence[i] >> 16) && value <= (occurrence[i] & 0xFFFF))
         return true;
   return false;
}

MoleculeRGroups::MoleculeRGroups ()
{
}

MoleculeRGroups::~MoleculeRGroups ()
{
}

void MoleculeRGroups::copyRGroupsFromMolecule (MoleculeRGroups &other)
{
   int n_rgroups = other.getRGroupCount();

   for (int i = 1; i <= n_rgroups; i++)
   {
      RGroup &rgroup = other.getRGroup(i);

      if (rgroup.fragments.size() > 0)
         getRGroup(i).copy(rgroup);
   }
}

void MoleculeRGroups::clear ()
{
   _rgroups.clear();
}

RGroup & MoleculeRGroups::getRGroup (int idx)
{
   if (_rgroups.size() < idx)
      _rgroups.resize(idx);

   return _rgroups[idx - 1];
}

int MoleculeRGroups::getRGroupCount () const
{
   return _rgroups.size();
}

void MoleculeRGroupFragment::addAttachmentPoint (int order, int index)
{
   if (_attachment_index.size() <= order)
      _attachment_index.resize(order + 1);

   _attachment_index[order].push(index);
}

void MoleculeRGroupFragment::removeAttachmentPoint (int index)
{
   int i, j;

   for (i = 0; i < _attachment_index.size(); i++)
      if ((j = _attachment_index[i].find(index)) != -1)
      {
         if (j == _attachment_index[i].size() - 1)
            _attachment_index[i].pop();
         else
            _attachment_index[i][j] = _attachment_index[i].pop();
      }
}
