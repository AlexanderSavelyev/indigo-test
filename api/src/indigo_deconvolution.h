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

#ifndef __indigo_deconvolution__
#define __indigo_deconvolution__

#include "indigo_internal.h"
#include "graph/graph_highlighting.h"
#include "molecule/query_molecule.h"
#include "molecule/molecule.h"

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable:4251)
#endif

class DLLEXPORT IndigoDeconvolution : public IndigoObject {
private:
   enum {
      SHIFT_IDX = 2
   };
public:
   class Item {
   public:
      Item(Molecule& mol):mol_in(mol) {}

      Molecule & mol_in;
      Molecule   mol_out;
      GraphHighlighting highlight;
      QueryMolecule rgroup_mol;
      RedBlackStringObjMap< Array<char> > properties;
   private:
      Item(const Item&);
   };
   IndigoDeconvolution(bool aromatize);
   virtual ~IndigoDeconvolution();

   void addMolecule(Molecule& mol, RedBlackStringObjMap< Array<char> >* props);

   void makeRGroups (Molecule& scaffold);

   Molecule& getDecomposedScaffold();
   ObjArray<Item>& getItems ();

   int flags;

   static bool matchBonds (Graph &g1, Graph &g2, int i, int j, void* userdata);
   static bool matchAtoms (Graph &g1, Graph &g2, const int *core_sub, int i, int j, void* userdata);

   ObjArray<Molecule> scaffolds;

   int (*cbEmbedding) (const int *sub_vert_map, const int *sub_edge_map, const void* info, void* userdata);
   void *embeddingUserdata;


private:
   class EmbContext {
   public:
       EmbContext(int flag):flags(flag) {}
       Array<int> visitedAtoms;
       Array<int> lastMapping;
       Array<int> lastInvMapping;
       ObjArray< Array<int> > attachmentOrder;
       ObjArray< Array<int> > attachmentIndex;
       int flags;

       int getRgroupNumber() const { return attachmentIndex.size()-1;}

       void renumber(Array<int>& map, Array<int>& inv_map);
   private:
       EmbContext(const EmbContext&); //no implicit copy
   };
   void _makeRGroup (Item& elem);
   void _createRgroups(Molecule& molecule_set, QueryMolecule& r_molecule, EmbContext& emb_context);
   void _parseOptions(const char* options);
   int _findOrAddFullRGroup(Array<int>& att_order, Array<int>& att_idx, QueryMolecule& qmol, Array<int>& map);

   static int _rGroupsEmbedding(Graph &g1, Graph &g2, int *core1, int *core2, void *userdata);

   bool _aromatic;

   Molecule _scaffold;
   Molecule _fullScaffold;
   ObjArray<Item> _deconvolutionItems;

   DEF_ERROR("R-Group deconvolution");
};

class DLLEXPORT IndigoDeconvolutionIter : public IndigoObject {
public:

   IndigoDeconvolutionIter(ObjArray<IndigoDeconvolution::Item>& items);
   virtual ~IndigoDeconvolutionIter();

   virtual IndigoObject * next ();
   virtual bool hasNext ();

protected:
   int _index;
   ObjArray<IndigoDeconvolution::Item>& _items;
};

class DLLEXPORT IndigoDeconvolutionElem : public IndigoObject
{
public:
   IndigoDeconvolutionElem (IndigoDeconvolution::Item &item, int index);
   ~IndigoDeconvolutionElem ();

   virtual int getIndex ();

   IndigoDeconvolution::Item &item;
   int idx;
};

#ifdef _WIN32
#pragma warning(pop)
#endif

#endif
