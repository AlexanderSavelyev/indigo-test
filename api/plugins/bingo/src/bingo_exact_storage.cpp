#include "bingo_exact_storage.h"

#include "molecule/elements.h"
#include "base_cpp/crc32.h"
#include "bingo_mmf_storage.h"

#include "graph/subgraph_hash.h"

using namespace bingo;

ExactStorage::ExactStorage ()
{
}

size_t ExactStorage::create()
{
   _hashes_ptr.allocate();
   new (_hashes_ptr.ptr()) BingoArray<dword>();
         
   return (size_t)_hashes_ptr;
}

void ExactStorage::load( size_t offset )
{
   _hashes_ptr = BingoPtr< BingoArray<dword> >(offset);
}

size_t ExactStorage::getOffset ()
{
   return (size_t)_hashes_ptr;
}


void ExactStorage::add( dword hash, int id )
{
   BingoArray<dword> &_molecule_hashes = _hashes_ptr.ref();

   if (_molecule_hashes.size() <= id)
      _molecule_hashes.resize((int)(id + 1000));

   _molecule_hashes[id] = hash;
}

void ExactStorage::findCandidates( dword query_hash, Array<int> &candidates )
{
   BingoArray<dword> &_molecule_hashes = _hashes_ptr.ref();

   for (int i = 0; i < _molecule_hashes.size(); i++)
   {
      dword hash = _molecule_hashes[i];

      if (hash == query_hash)
         candidates.push(i);
   }
}

dword ExactStorage::calculateMolHash (Molecule &mol)
{
   QS_DEF(Molecule, mol_without_h);
   QS_DEF(Array<int>, vertices);
   int i;

   vertices.clear();
   
   for (i = mol.vertexBegin(); i != mol.vertexEnd(); i = mol.vertexNext(i))
      if (mol.getAtomNumber(i) != ELEM_H)
         vertices.push(i);

   mol_without_h.makeSubmolecule(mol, vertices, 0);

   QS_DEF(Array<int>, vertex_codes);
   vertex_codes.clear_resize(mol_without_h.vertexEnd());

   SubgraphHash hh(mol_without_h);

   for (int v = mol_without_h.vertexBegin(); v != mol_without_h.vertexEnd(); v = mol_without_h.vertexNext(v))
      vertex_codes[v] = _vertexCode(mol_without_h, v);
   hh.vertex_codes = &vertex_codes;
   hh.max_iterations = (mol_without_h.edgeCount() + 1) / 2;

   return hh.getHash();
}

dword ExactStorage::calculateRxnHash (Reaction &rxn)
{
   QS_DEF(Molecule, mol_without_h) ;
   QS_DEF(Array<int>, vertices);
   int i, j;
   dword hash = 0;

   for (j = rxn.begin(); j != rxn.end(); j = rxn.next(j))
   {
      Molecule &mol = rxn.getMolecule(j);

      vertices.clear();

      for (i = mol.vertexBegin(); i != mol.vertexEnd(); i = mol.vertexNext(i))
         if (mol.getAtomNumber(i) != ELEM_H)
            vertices.push(i);

      mol_without_h.makeSubmolecule(mol, vertices, 0);
      SubgraphHash hh(mol_without_h);
      hash += hh.getHash();
   }

   return hash;
}

int ExactStorage::_vertexCode (Molecule &mol, int vertex_idx)
{
   if (mol.isPseudoAtom(vertex_idx))
      return indigo::CRC32::get(mol.getPseudoAtom(vertex_idx));

   if (mol.isRSite(vertex_idx))
      return ELEM_RSITE;

   return mol.getAtomNumber(vertex_idx);
}
