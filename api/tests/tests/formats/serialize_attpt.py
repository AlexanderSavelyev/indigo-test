import sys
sys.path.append('../../common')
from env_indigo import *

indigo = Indigo()
indigo.setOption("ignore-stereochemistry-errors", True)
indigo.setOption("molfile-saving-skip-date", True)
def testSerializeAttachmentPoints (filename):
  print relativePath(filename)
  mol = indigo.loadMoleculeFromFile(filename)
  for rgp in mol.iterateRGroups():
    for frag in rgp.iterateRGroupFragments():
      print "  rgroup", rgp.index(), "frag", frag.index()
      buf = frag.serialize()
      print '  fragment serialized to', len(buf), 'bytes'
      print "    " + frag.canonicalSmiles()
      frag2 = indigo.unserialize(buf)
      print "    " + frag2.canonicalSmiles()
      if frag.canonicalSmiles() != frag2.canonicalSmiles():
        print "    MISMATCH!!!"
testSerializeAttachmentPoints(joinPath("molecules/recursive1.mol"))
testSerializeAttachmentPoints(joinPath("molecules/recursive2.mol"))
testSerializeAttachmentPoints(joinPath("molecules/r_occur.mol"))
testSerializeAttachmentPoints(joinPath("molecules/r_occur_2.mol"))
testSerializeAttachmentPoints(joinPath("molecules/sub_mar_q01.mol"))
