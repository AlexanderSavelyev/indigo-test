#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=
AS=as

# Macros
CND_PLATFORM=GNU-Linux-x86
CND_CONF=Release32
CND_DISTDIR=dist

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=build/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/main.o


# C Compiler Flags
CFLAGS=-m32

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=../../api/dist/ReleaseStatic32/GNU-Linux-x86/libindigo.a ../../layout/dist/Release32/GNU-Linux-x86/liblayout.a ../../reaction/dist/Release32/GNU-Linux-x86/libreaction.a ../../molecule/dist/Release32/GNU-Linux-x86/libmolecule.a ../../graph/dist/Release32/GNU-Linux-x86/libgraph.a -lpthread -lz -lstdc++

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-Release32.mk dist/Release32/GNU-Linux-x86/indigo-cano

dist/Release32/GNU-Linux-x86/indigo-cano: ../../api/dist/ReleaseStatic32/GNU-Linux-x86/libindigo.a

dist/Release32/GNU-Linux-x86/indigo-cano: ../../layout/dist/Release32/GNU-Linux-x86/liblayout.a

dist/Release32/GNU-Linux-x86/indigo-cano: ../../reaction/dist/Release32/GNU-Linux-x86/libreaction.a

dist/Release32/GNU-Linux-x86/indigo-cano: ../../molecule/dist/Release32/GNU-Linux-x86/libmolecule.a

dist/Release32/GNU-Linux-x86/indigo-cano: ../../graph/dist/Release32/GNU-Linux-x86/libgraph.a

dist/Release32/GNU-Linux-x86/indigo-cano: ${OBJECTFILES}
	${MKDIR} -p dist/Release32/GNU-Linux-x86
	${LINK.c} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/indigo-cano ${OBJECTFILES} ${LDLIBSOPTIONS} 

${OBJECTDIR}/main.o: main.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -Wall -s -I../../api -MMD -MP -MF $@.d -o ${OBJECTDIR}/main.o main.c

# Subprojects
.build-subprojects:
	cd ../../api && ${MAKE}  -f Makefile CONF=ReleaseStatic32
	cd ../../layout && ${MAKE}  -f Makefile CONF=Release32
	cd ../../reaction && ${MAKE}  -f Makefile CONF=Release32
	cd ../../molecule && ${MAKE}  -f Makefile CONF=Release32
	cd ../../graph && ${MAKE}  -f Makefile CONF=Release32

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r build/Release32
	${RM} dist/Release32/GNU-Linux-x86/indigo-cano

# Subprojects
.clean-subprojects:
	cd ../../api && ${MAKE}  -f Makefile CONF=ReleaseStatic32 clean
	cd ../../layout && ${MAKE}  -f Makefile CONF=Release32 clean
	cd ../../reaction && ${MAKE}  -f Makefile CONF=Release32 clean
	cd ../../molecule && ${MAKE}  -f Makefile CONF=Release32 clean
	cd ../../graph && ${MAKE}  -f Makefile CONF=Release32 clean

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
