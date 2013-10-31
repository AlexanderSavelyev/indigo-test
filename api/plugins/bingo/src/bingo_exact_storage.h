#ifndef __bingo_exact_storage__
#define __bingo_exact_storage__

#include "molecule/molecule.h"
#include "reaction/reaction.h"
#include "bingo_ptr.h"

using namespace indigo;

namespace bingo
{
   class ExactStorage
   {
   public:
      ExactStorage ();

      static size_t create(BingoPtr<ExactStorage> &exact_ptr);

      static void load (BingoPtr<ExactStorage> &exact_ptr, size_t offset);

      size_t getOffset ();

      void add (dword hash, int id);

      void findCandidates (dword query_hash, Array<int> &candidates, int part_id = -1, int part_count = -1);

      static dword calculateMolHash (Molecule &mol);

      static dword calculateRxnHash (Reaction &rxn);

   private:
      BingoArray<dword> _molecule_hashes;

      static int _vertexCode (Molecule &mol, int vertex_idx);
   };
}

#endif //__bingo_exact_storage__