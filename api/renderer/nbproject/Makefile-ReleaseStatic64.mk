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
CND_CONF=ReleaseStatic64
CND_DISTDIR=dist

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=build/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/src/indigo_render2d.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=-m64 -fPIC
CXXFLAGS=-m64 -fPIC

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-ReleaseStatic64.mk dist/ReleaseStatic64/GNU-Linux-x86/libindigo-renderer.a

dist/ReleaseStatic64/GNU-Linux-x86/libindigo-renderer.a: ${OBJECTFILES}
	${MKDIR} -p dist/ReleaseStatic64/GNU-Linux-x86
	${RM} dist/ReleaseStatic64/GNU-Linux-x86/libindigo-renderer.a
	${AR} -rv ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libindigo-renderer.a ${OBJECTFILES} 
	$(RANLIB) dist/ReleaseStatic64/GNU-Linux-x86/libindigo-renderer.a

${OBJECTDIR}/src/indigo_render2d.o: src/indigo_render2d.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -O3 -Wall -s -I. -Isrc -I.. -I../.. -I../../common -I.. -I../src -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/indigo_render2d.o src/indigo_render2d.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r build/ReleaseStatic64
	${RM} dist/ReleaseStatic64/GNU-Linux-x86/libindigo-renderer.a

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
