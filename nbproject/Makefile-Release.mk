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
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-MacOSX
CND_DLIB_EXT=dylib
CND_CONF=Release
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/_ext/85813d25/byte_msg_parser.o \
	${OBJECTDIR}/_ext/85813d25/db_connection.o \
	${OBJECTDIR}/_ext/48f68b16/hashtable.o \
	${OBJECTDIR}/_ext/48f68b16/hashtable_itr.o \
	${OBJECTDIR}/_ext/48f68b16/hashtable_utility.o \
	${OBJECTDIR}/distribution_container_manager.o \
	${OBJECTDIR}/main.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/dcr_server

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/dcr_server: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.c} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/dcr_server ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/_ext/85813d25/byte_msg_parser.o: /Users/drew/NetBeansProjects/dcr_server/byte_msg_parser.c
	${MKDIR} -p ${OBJECTDIR}/_ext/85813d25
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/85813d25/byte_msg_parser.o /Users/drew/NetBeansProjects/dcr_server/byte_msg_parser.c

${OBJECTDIR}/_ext/85813d25/db_connection.o: /Users/drew/NetBeansProjects/dcr_server/db_connection.c
	${MKDIR} -p ${OBJECTDIR}/_ext/85813d25
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/85813d25/db_connection.o /Users/drew/NetBeansProjects/dcr_server/db_connection.c

${OBJECTDIR}/_ext/48f68b16/hashtable.o: /Users/drew/NetBeansProjects/dcr_server/hashtable/hashtable.c
	${MKDIR} -p ${OBJECTDIR}/_ext/48f68b16
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/48f68b16/hashtable.o /Users/drew/NetBeansProjects/dcr_server/hashtable/hashtable.c

${OBJECTDIR}/_ext/48f68b16/hashtable_itr.o: /Users/drew/NetBeansProjects/dcr_server/hashtable/hashtable_itr.c
	${MKDIR} -p ${OBJECTDIR}/_ext/48f68b16
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/48f68b16/hashtable_itr.o /Users/drew/NetBeansProjects/dcr_server/hashtable/hashtable_itr.c

${OBJECTDIR}/_ext/48f68b16/hashtable_utility.o: /Users/drew/NetBeansProjects/dcr_server/hashtable/hashtable_utility.c
	${MKDIR} -p ${OBJECTDIR}/_ext/48f68b16
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/48f68b16/hashtable_utility.o /Users/drew/NetBeansProjects/dcr_server/hashtable/hashtable_utility.c

${OBJECTDIR}/distribution_container_manager.o: distribution_container_manager.c
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/distribution_container_manager.o distribution_container_manager.c

${OBJECTDIR}/main.o: main.c
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/main.o main.c

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
