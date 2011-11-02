import sys
sys.path.append('../../common')
from env_indigo import *

indigo = Indigo()
indigo.setOption("molfile-saving-skip-date", "1");
def testReactionInstrumentation ():
  rxn = indigo.loadReaction("[PH5].CN>CC>")
  rxn.addReactant(indigo.loadMolecule("Oc1ccccc1"))
  rxn.addProduct(indigo.loadMolecule("n1ccccc1"))
  cat = indigo.createMolecule()
  cat.addAtom("2.  acid")
  rxn.addCatalyst(cat)
  print rxn.smiles()
  print rxn.rxnfile()
  print rxn.countReactants(), "reactants"
  print rxn.countCatalysts(), "catalysts"
  print rxn.countProducts(), "products"
  print rxn.countMolecules(), "molecules"
  for mol in rxn.iterateMolecules():
    print "MOLECULE:", mol.canonicalSmiles()
  for mol in rxn.iterateReactants():
    print "REACTANT:", mol.canonicalSmiles()
  for mol in rxn.iterateCatalysts():
    print "CATALYST:", mol.canonicalSmiles()
  for mol in rxn.iterateProducts():
    print "PRODUCT: ", mol.canonicalSmiles()
  print "\nREMOVING"
  rxn.iterateReactants().next().remove()
  rxn.iterateProducts().next().remove()
  print rxn.countReactants(), "reactants"
  print rxn.countCatalysts(), "catalysts"
  print rxn.countProducts(), "products"
  print rxn.countMolecules(), "molecules"
  for mol in rxn.iterateMolecules():
    print "MOLECULE:", mol.canonicalSmiles()
  for mol in rxn.iterateReactants():
    print "REACTANT:", mol.canonicalSmiles()
  for mol in rxn.iterateCatalysts():
    print "CATALYST:", mol.canonicalSmiles()
  for mol in rxn.iterateProducts():
    print "PRODUCT: ", mol.canonicalSmiles()
testReactionInstrumentation()
