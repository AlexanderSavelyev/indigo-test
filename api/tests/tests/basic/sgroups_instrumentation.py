import os
import sys
sys.path.append('../../common')
from env_indigo import *

indigo = Indigo()
indigo.setOption("molfile-saving-skip-date", True)

if not os.path.exists(joinPath("out")):
   os.makedirs(joinPath("out"))
saver = indigo.createFileSaver(joinPath("out/sgroups-instrumentation.sdf"), "sdf")
   
def testSGroupsInstrumentation ():
    mol = indigo.loadMolecule("c1ccccc1.CCC.O.N.P")
    mol.layout()
    saver.append(mol)
    sgroup1 = mol.addDataSGroup([6, 7, 8], [6, 7], "SG", "a")
    sgroup2 = mol.addDataSGroup([9], [], "ID", "b")
    sgroup3 = mol.addDataSGroup([10], [], "ID", "c")
    sgroup4 = mol.addDataSGroup([11], [], "ID", "d")
    print(mol.molfile())
    saver.append(mol)

    mol2 = indigo.unserialize(mol.serialize())

    print(mol2.molfile())
    saver.append(mol2)

    sgroup2.setDataSGroupXY(13, 1)
    sgroup3.setDataSGroupXY(.3, .3, "relative")
    sgroup4.setDataSGroupXY(5, 6, "absolute")
    print(mol.molfile())
    saver.append(mol)

    mol2 = indigo.unserialize(mol.serialize())

    print(mol2.molfile())
    saver.append(mol2)
    
testSGroupsInstrumentation()
