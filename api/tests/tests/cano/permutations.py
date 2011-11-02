import itertools;
import random;
import sys
sys.path.append('../../common')
from env_indigo import *

indigo = Indigo()
indigo.setOption("treat-x-as-pseudoatom", "1")
mol_db_names = [
   ("../../data/zinc-slice.sdf", indigo.iterateSDFile),
   ("../../data/thiazolidines.sdf", indigo.iterateSDFile),
   ("../../data/sugars.sdf", indigo.iterateSDFile),
   ("molecules/helma.smi", indigo.iterateSmilesFile),
   ("molecules/cis_trans.smi", indigo.iterateSmilesFile),
   ("molecules/stereo_cis_trans.sdf", indigo.iterateSDFile),
   ("molecules/complicated_cis_trans.sdf", indigo.iterateSDFile),
   ("molecules/complicated_cis_trans_h.sdf", indigo.iterateSDFile),
   ("molecules/test_explicit_h.smi", indigo.iterateSmilesFile),
]
def random_permutation(iterable, r=None):
   "Random selection from itertools.permutations(iterable, r)"
   pool = tuple(iterable)
   if r is None:
      r = len(pool)
   return list(random.sample(pool, r))
   
def testMol(mol):
   mol_for_test = mol.clone()
   base_smiles = mol_for_test.canonicalSmiles()
   print(base_smiles)
   mol_for_test.foldHydrogens()
   base_smiles2 = mol_for_test.canonicalSmiles()
   if base_smiles != base_smiles2:
      msg = "  Canonical smiles has changes after fold hydrogens: %s -> %s" % (base_smiles, base_smiles2)
      print(msg)
      sys.stderr.write(msg + "\n")
   mol_for_test.unfoldHydrogens()
   base_smiles3 = mol_for_test.canonicalSmiles()
   if base_smiles != base_smiles3:
      msg = "  Canonical smiles has changes after fold->unfold hydrogens: %s -> %s" % (base_smiles, base_smiles3)
      print(msg)
      sys.stderr.write(msg + "\n")
   mol_for_test.foldHydrogens()
   base_smiles2 = mol_for_test.canonicalSmiles()
   if base_smiles != base_smiles2:
      msg = "  Canonical smiles has changes after fold->unfold->fold hydrogens: %s -> %s" % (base_smiles, base_smiles2)
      print(msg)
      sys.stderr.write(msg + "\n")
   
   # collect atom indices
   indices = [x.index() for x in mol.iterateAtoms()]
   
   # test some of permutations
   ncount = 20
   for it in range(ncount):
      perm = random_permutation(indices, len(indices))
      #perm_mol = mol.createSubmolecule(perm)
      perm_mol = mol.clone()
      
      need_fold = random.choice([ True, False ])
      need_unfold = random.choice([ True, False ])
      if need_fold:
         perm_mol.foldHydrogens()
      if need_unfold:
         perm_mol.unfoldHydrogens()
         
      need_fold2 = random.choice([ True, False ])
      need_unfold2 = random.choice([ True, False ])
      if need_fold2:
         perm_mol.foldHydrogens()
      if need_unfold2:
         perm_mol.unfoldHydrogens()
         
      perm_cano_sm = perm_mol.canonicalSmiles()
      if perm_cano_sm != base_smiles:
         msg = "  %d, %d, %d, %d: %s -> %s" % (need_fold, need_unfold, need_fold2, need_unfold2, perm, perm_cano_sm)
         print(msg)
         sys.stderr.write(msg + "\n")
   return base_smiles
   
for db_name, load_fund in mol_db_names:
   print("Database: %s" % (db_name))
   idx = 0
   for item in load_fund(db_name):
      try:
         name = item.name()
      except IndigoException, e:
         name = getIndigoExceptionText(e)
      print("%s (#%s)" % (name, idx))
      try:
         testMol(item)
      except IndigoException, e:
         print("Error: %s" % (getIndigoExceptionText(e)))
      idx += 1
