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

#ifndef __molecule_dearom_h__
#define __molecule_dearom_h__

#include "base_cpp/array.h"
#include "base_cpp/exception.h"
#include "base_cpp/tlscont.h"
#include "base_cpp/gray_codes.h"
#include "base_cpp/d_bitset.h"
#include "graph/graph_perfect_matching.h"

namespace indigo {

class BaseMolecule;
class Molecule;
class Scanner;
class Output;

// Storage for dearomatizations
class DearomatizationsStorage
{
   friend class DearomatizationsStorageWrapper;
public:
   DEF_ERROR("Dearomatization storage");

   explicit DearomatizationsStorage ();

   void clear                   (void);
   void clearIndices            (void);
   void clearBondsState         (void);

   void setGroupsCount           (int groupsCount);
   void setGroup                 (int group, int boundsCount, const int *bonds, 
                                  int heteroAtomsCount, const int *hetroAtoms);
   void addGroupDearomatization  (int group, const byte *dearomBondsState);
   void addGroupHeteroAtomsState (int group, const byte *heteroAtomsState);

   int         getGroupDearomatizationsCount (int group) const;
   byte*       getGroupDearomatization       (int group, int dearomatizationIndex);
   const int*  getGroupBonds                 (int group) const;
   int         getGroupBondsCount            (int group) const;

   int         getGroupHeterAtomsStateCount  (int group) const;
   const byte* getGroupHeterAtomsState       (int group, int index) const;
   const int*  getGroupHeteroAtoms           (int group) const;
   int         getGroupHeteroAtomsCount      (int group) const;

   int         getGroupsCount                (void) const;

   void        saveBinary                    (Output &output) const;
   void        loadBinary                    (Scanner &scanner);

   int         getDearomatizationParams      (void)         { return _dearomParams; }
   void        setDearomatizationParams      (int params)   { _dearomParams = params; }

protected:
   struct PseudoArray
   {
      int count;
      int offset;
   };
   struct Group
   {
      PseudoArray aromBondsIndices;
      PseudoArray dearomBondsState;
      PseudoArray heteroAtomsIndices;
      PseudoArray heteroAtomsState;
   };
protected:
   Array<int>   _aromBondsArray;            // Bonds used in this connectivity group
   Array<int>   _heteroAtomsIndicesArray;   // Heteroatoms indices
   Array<Group> _aromaticGroups;

   // Data for I/O
   Array<byte>  _dearomBondsStateArray;     // Array of array of dearomatization configuration
   Array<byte>  _heteroAtomsStateArray;     // States for heteroatoms
   byte         _dearomParams;
};

// Class for handling aromatic groups in molecule (contains helpful functions)
class DearomatizationsGroups
{
public:
   // Constants for Prepare function
   enum 
   {
      GET_HETERATOMS_INDICES  =  0x01,
      GET_VERTICES_FILTER     =  0x02
   };
   struct GROUP_DATA
   {
      Array<int> bonds;
      Array<int> bondsInvMapping;
      Array<int> vertices;
      Array<int> verticesFilter;
      Array<int> heteroAtoms;
      Array<int> heteroAtomsInvMapping;
   };
public:
   DearomatizationsGroups (BaseMolecule &molecule);

   // for flags see GET_***
   void getGroupData            (int group, int flags, GROUP_DATA *data);
   // Construct bondsInvMapping, vertices and heteroAtomsInvMapping
   void getGroupDataFromStorage (DearomatizationsStorage &storage, int group, GROUP_DATA *data);

   int  detectAromaticGroups (const int *atom_external_conn);
   void constructGroups      (DearomatizationsStorage &storage, bool needHeteroAtoms);

   bool* getAcceptDoubleBonds (void);
   bool  isAcceptDoubleBond   (int atom);

   DEF_ERROR("Dearomatization groups");
protected:
   void _detectAromaticGroups (int v_idx, const int *atom_external_conn);

protected:
   BaseMolecule &_molecule;
   int _aromaticGroups;

   // Additional data stored here to prevent reallocatoins
   TL_CP_DECL(Array<int>,  _vertexAromaticGroupIndex);
   TL_CP_DECL(Array<bool>, _vertexIsAcceptDoubleEdge);
   TL_CP_DECL(Array<bool>, _vertexIsAcceptSingleEdge);
   TL_CP_DECL(Array<int>,  _vertexProcessed);

   TL_CP_DECL(Array<int>, _groupVertices);
   TL_CP_DECL(Array<int>, _groupEdges);
   TL_CP_DECL(Array<int>, _groupHeteroAtoms);
   TL_CP_DECL(GROUP_DATA, _groupData);
};

// Molecule dearomatization class.
class Dearomatizer
{
public:
   enum { 
      PARAMS_NO_DEAROMATIZATIONS,
      PARAMS_SAVE_ALL_DEAROMATIZATIONS, // Store all dearomatizations
      PARAMS_SAVE_ONE_DEAROMATIZATION,  // Store just one dearomatization for every heteroatom configuration
      PARAMS_SAVE_JUST_HETERATOMS       // Store just heteroatoms configuration
   }; 
public:
   explicit Dearomatizer (BaseMolecule &molecule, const int *atom_external_conn);
   virtual ~Dearomatizer ();

   void  enumerateDearomatizations  (DearomatizationsStorage &dearomatizations);

   static void setDearomatizationParams      (int params);

protected:
   class GraphMatchingFixed : public GraphPerfectMatching
   {
   public:
      GraphMatchingFixed (BaseMolecule &molecule);

      void setFixedInfo (const Dbitset *edgesFixed, const Dbitset *verticesFixed);

      virtual bool checkVertex (int v_idx);
      virtual bool checkEdge   (int e_idx);

   protected:
      const Dbitset *_edgesFixed;
      const Dbitset *_verticesFixed;
   };

protected:
   GraphMatchingFixed  _graphMatching;

   BaseMolecule &_molecule;
   int _connectivityGroups;
   int _activeGroup;

   DearomatizationsGroups              _aromaticGroups;
   DearomatizationsStorage            *_dearomatizations;

   TL_CP_DECL(DearomatizationsGroups::GROUP_DATA, _aromaticGroupData);
   /*TL_CP_DECL(*/Dbitset/*,    */_edgesFixed/*)*/;
   /*TL_CP_DECL(*/Dbitset/*,    */_verticesFixed/*)*/;
   TL_CP_DECL(Array<int>, _submoleculeMapping);

protected:
   void _initEdges         (void);
   void _initVertices      (void);

   void _prepareGroup      (int group, Molecule &submolecule);

   void _fixHeteratom       (int atom_idx, bool toFix);
   void _processMatching    (Molecule &submolecule, int group, 
                             const byte* hetroAtomsState);
   void _enumerateMatching  (void);
   void _handleMatching     (void);
};

// Dearomatization matcher with delayed initialization
class DearomatizationMatcher
{
public:
   DEF_ERROR("Dearomatization matcher");

   DearomatizationMatcher (DearomatizationsStorage &dearomatizations, BaseMolecule &molecule,
      const int *atom_external_conn);

   bool isAbleToFixBond (int edge_idx, int type);
   bool fixBond         (int edge_idx, int type);
   void unfixBond       (int edge_idx);
   void unfixBondByAtom (int atom_idx);

protected:
   void _prepare        (void);
   void _prepareGroup   (int group);

   void _generateUsedVertices    (void);
   bool _tryToChangeActiveIndex  (int dearom_idx, int group, byte *groupEdgesPtr, byte *groupEdgesStatePtr);
   bool _fixBondInMatching (int group, int indexInGroup, int type);

protected:
   struct GroupExData
   {
      int offsetInEdgesState;   // Offset in matched edges state
      int activeEdgeState; 
      int offsetInVertices;
      int verticesUsed;
      bool needPrepare;
   };
   // Graph edge matching class to support current dearomatization
   class GraphMatchingEdgeFixed : public GraphPerfectMatching
   {
   public:
      GraphMatchingEdgeFixed (BaseMolecule &molecule);

      void setExtraInfo (byte *edgesEdges);

      virtual bool checkEdge   (int e_idx);
   protected:
      byte *_edgesState;
   };
   // Graph edge matching class to find dearomatization by heteroatoms state
   class GraphMatchingVerticesFixed : public GraphPerfectMatching
   {
   public:
      GraphMatchingVerticesFixed (BaseMolecule &molecule);

      void setVerticesState   (const byte *verticesState);
      void setVerticesMapping (int  *verticesMapping);
      void setVerticesAccept  (bool *verticesAcceptDoubleBond);

      virtual bool checkVertex (int v_idx);
   protected:
      const byte *_verticesState;
      int  *_verticesMapping;
      bool *_verticesAcceptDoubleBond;
   };

protected:
   BaseMolecule &_molecule;
   DearomatizationsStorage &_dearomatizations;
   GraphMatchingEdgeFixed   _graphMatchingFixedEdges;
   DearomatizationsGroups   _aromaticGroups;

   TL_CP_DECL(Array<byte>,        _matchedEdges);      // Edges that have already been matched
   TL_CP_DECL(Array<byte>,        _matchedEdgesState); // State of such edges
   TL_CP_DECL(Array<GroupExData>, _groupExInfo);       // Additional data for group
   TL_CP_DECL(Array<int>,         _verticesInGroup);
   TL_CP_DECL(Dbitset,            _verticesAdded);
   TL_CP_DECL(Array<int>,         _edges2GroupMapping);
   TL_CP_DECL(Array<int>,         _edges2IndexInGroupMapping);
   TL_CP_DECL(Array<byte>,        _correctEdgesArray);
   TL_CP_DECL(Array<int>,         _verticesFixCount);
   TL_CP_DECL(DearomatizationsGroups::GROUP_DATA, _aromaticGroupsData);

   bool              _needPrepare;
   int               _lastAcceptedEdge;
   int               _lastAcceptedEdgeType;
};

class MoleculeDearomatizer
{
public:
   MoleculeDearomatizer (Molecule &mol, DearomatizationsStorage &dearomatizations);

   // Function dearomatizes as much as possible.
   // Returns true if all bonds were dearomatized, false overwise
   static bool dearomatizeMolecule (Molecule &mol);

   void dearomatizeGroup (int group, int dearomatization_index);
private:
   DearomatizationsStorage &_dearomatizations;
   Molecule &_mol;
};

}

#endif // __molecule_dearom_h__
