cd ../graph
dmake -m serial -f ../graph/Makefile.sun.32 $1
cd ../molecule
dmake -m serial -f ../molecule/Makefile.sun.32 $1
cd ../reaction
dmake -m serial -f ../reaction/Makefile.sun.32 $1
cd ../layout
dmake -m serial -f ../layout/Makefile.sun.32 $1
cd ../bingo
dmake -m serial -f Makefile.sun.32 $1
