PROJECT(FloPoCo C CXX)

MESSAGE("Trying to build FloPoCo on " ${CMAKE_SYSTEM_NAME} ", compiler is " ${CMAKE_C_COMPILER})

# CMake's way of handling various versions
cmake_minimum_required(VERSION 2.4)
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
  message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()
#####################For packaging (source file distrib so far)#######
# Use  make package_source to build a release
# Currently it doesn't work at all

SET(CPACK_SOURCE_GENERATOR "TGZ")
SET(CPACK_PACKAGE_VERSION_MAJOR "0")
SET(CPACK_PACKAGE_VERSION_MINOR "9")
SET(CPACK_PACKAGE_VERSION_PATCH "0")

INCLUDE(CPack)
# 
#Compilation flags
#SET(CMAKE_CXX_FLAGS_DEBUG "-Wall")
# Uncomment for profiling
SET(CMAKE_CXX_FLAGS_DEBUG "-g -pg -Wall")
#SET(CMAKE_CXX_FLAGS_DEBUG "-pg -Wall")
# Do you want an executable of 30MB ?
SET(CMAKE_BUILD_TYPE DEBUG)
#SET(CMAKE_BUILD_TYPE release)



# Detect GMP / GMPXX
FIND_PATH(GMPXX_H
  gmpxx.h
  )

FIND_LIBRARY(GMP_LIB
  NAMES gmp gmp.lib
  DOC "Directory of the GMP library")

FIND_PATH(GMP_H
  gmp.h
  )

FIND_LIBRARY(GMPXX_LIB
  NAMES gmpxx gmpxx.lib
  DOC "Directory of the GMPXX library")

IF (GMPXX_H AND GMPXX_LIB AND GMP_H AND GMP_LIB)
  MESSAGE(STATUS "Found gmp and gmpxx: ${GMP_H},${GMP_LIB},${GMPXX_H},${GMPXX_LIB}")
ELSE (GMPXX_H AND GMPXX_LIB AND GMP_H AND GMP_LIB)
  MESSAGE(FATAL_ERROR "Could not find gmp and/or gmpxx (found: ${GMP_H},${GMP_LIB},${GMPXX_H},${GMPXX_LIB})")
ENDIF (GMPXX_H AND GMPXX_LIB AND GMP_H AND GMP_LIB)


# Detect MPFR

FIND_PATH(MPFR_H
  mpfr.h
  HINTS "C:/Program Files/Microsoft Visual Studio 9.0/VC/include"
  DOC "Path of mpfr.h, the include file for GNU MPFR library"
  )

FIND_LIBRARY(MPFR_LIB
  NAMES mpfr mpfr.lib
  HINTS "C:/Program Files/Microsoft Visual Studio 9.0/VC/lib"
  DOC "Directory of the MPFR library")

IF (MPFR_H AND MPFR_LIB)
  MESSAGE(STATUS "Found mpfr: ${MPFR_H},${MPFR_LIB}")
ELSE (MPFR_H AND MPFR_LIB)
  MESSAGE(FATAL_ERROR "Could not find MPFR (found: ${MPFR_H},${MPFR_LIB}).\nOn Debian-based systems type: 'sudo apt-get install libmpfr-dev'")
ENDIF (MPFR_H AND MPFR_LIB)


# Detect MPFI
FIND_PATH(MPFI_H
  mpfi.h
  HINTS "C:/Program Files/Microsoft Visual Studio 9.0/VC/include"
  DOC "Path of mpfi.h"
  )

FIND_LIBRARY(MPFI_LIB
  NAMES mpfi mpfi.lib
  HINTS "C:/Program Files/Microsoft Visual Studio 9.0/VC/lib"
  DOC "Directory of the MPFI library")

IF (MPFI_H AND MPFI_LIB)
  MESSAGE(STATUS "Found mpfi: ${MPFI_H},${MPFI_LIB}")
ELSE (MPFI_H AND MPFI_LIB)
  MESSAGE(FATAL_ERROR "Could not find MPFI (found: ${MPFI_H},${MPFI_LIB})")
ENDIF (MPFI_H AND MPFI_LIB)

# Detect Python
FIND_PATH(PYTHON_H
  python2.7/Python.h
  DOC "Path of Python.h"
  )

FIND_LIBRARY(PYTHON_LIB
  NAMES libpython2.7.so libpython2.7.dylib
  HINTS "/usr/lib/x86_64-linux-gnu"
  DOC "Directory of the Python library"
  )

IF ( PYTHON_H AND PYTHON_LIB )
  SET(PYTHON_FOUND TRUE)
ENDIF ( PYTHON_H AND PYTHON_LIB )

IF ( PYTHON_FOUND )
  MESSAGE ( STATUS "Found Python : ${PYTHON_H},${PYTHON_LIB}" )
ELSE ( PYTHON_FOUND )
  MESSAGE ( STATUS "Could not find (found: ${PYTHON_H},${PYTHON_LIB}), no flopoco_soaktest will be available")
ENDIF ( PYTHON_FOUND )

# Detect Sollya
FIND_LIBRARY( SOLLYA_LIB
  NAMES libsollya.so libsollya.dylib
  HINTS "/usr/lib/x86_64-linux-gnu"
  )

FIND_PATH(SOLLYA_H
  sollya.h
  )

IF (SOLLYA_H AND SOLLYA_H)
  MESSAGE(STATUS "Found Sollya: ${SOLLYA_LIB},${SOLLYA_H}")
ELSE (SOLLYA_H AND SOLLYA_H)
  MESSAGE(FATAL_ERROR "Sollya not found (found: ${SOLLYA_LIB},${SOLLYA_H})")
ENDIF (SOLLYA_H AND SOLLYA_H)


FIND_PATH(WCPG_H
  wcpg.h
  DOC "Path of wcpg.h, the include file for GNU MPFR library"
  )

FIND_LIBRARY(WCPG_LIB
  NAMES wcpg
  DOC "Directory of the WCPG library"
  )

IF (WCPG_H AND WCPG_LIB)
  SET(WCPG_FOUND TRUE)
ENDIF (WCPG_H AND WCPG_LIB)

IF ( WCPG_FOUND )
  MESSAGE ( STATUS "Found WCPG : ${WCPG_H},${WCPG_LIB}" )
  ADD_DEFINITIONS(-DHAVE_WCPG)
ELSE ( WCPG_FOUND )
  MESSAGE ( STATUS "Could not find WCPG (found: ${WCPG_H},${WCPG_LIB}), this will affect the FixIIR operator" )
ENDIF ( WCPG_FOUND )

# dectect if scalp is available
FIND_PATH(SCALP_H
  ScaLP/Solver.h
  HINTS "/usr/local/include"
  DOC "Path of main header of scalp"
  )
FIND_LIBRARY(SCALP_LIB
  NAMES libScaLP libScaLP.so  libScaLP.dylib
  HINTS "/usr/local/lib64/"
  DOC "Directory of the SCALP main library")

FIND_LIBRARY(SCALP_CPLEX_LIB
  NAMES libScaLP-CPLEX libScaLP-CPLEX.so libScaLP-CPLEX.dylib
  HINTS "/usr/local/lib64/"
  DOC "Directory of the SCALP library for cplex solver backend")

FIND_LIBRARY(SCALP_GUROBI_LIB
  NAMES libScaLP-Gurobi libScaLP-Gurobi.so libScaLP-Gurobi.dylib
  HINTS "/usr/local/lib64/"
  DOC "Directory of the SCALP library for Gurobi solver backend")

FIND_LIBRARY(SCALP_SCIP_LIB
  NAMES libScaLP-SCIP libScaLP-SCIP.so libScaLP-SCIP.dylib
  HINTS "/usr/local/lib64/"
  DOC "Directory of the SCALP library for SCIP solver backend")

FIND_LIBRARY(SCALP_LPSOLVE_LIB
  NAMES libScaLP-LPSOLVE libScaLP-LPSOLVE.so libScaLP-LPSOLVE.dylib
  HINTS "/usr/local/lib64/"
  DOC "Directory of the SCALP library for LPSOLVE solver backend")

IF (SCALP_H AND SCALP_LIB AND (SCALP_CPLEX_LIB OR SCALP_GUROBI_LIB OR SCALP_SCIP_LIB OR SCALP_LPSOLVE_LIB))
  SET(SCALP_FOUND TRUE)
ENDIF (SCALP_H AND SCALP_LIB AND (SCALP_CPLEX_LIB OR SCALP_GUROBI_LIB OR SCALP_SCIP_LIB OR SCALP_LPSOLVE_LIB))

IF (SCALP_FOUND)
  ADD_DEFINITIONS(-DHAVE_SCALP)
  set(SCALP_LIBS ${SCALP_LIBS} ${SCALP_LIB}) 
  IF (SCALP_CPLEX_LIB)
	set(SCALP_LIBS ${SCALP_LIBS} ${SCALP_CPLEX_LIB})
  ENDIF (SCALP_CPLEX_LIB)
  IF (SCALP_GUROBI_LIB)
	set(SCALP_LIBS ${SCALP_LIBS} ${SCALP_GUROBI_LIB})
  ENDIF (SCALP_GUROBI_LIB)
  
  IF (SCALP_SCIP_LIB)
	set(SCALP_LIBS ${SCALP_LIBS} ${SCALP_SCIP_LIB})
  ENDIF (SCALP_SCIP_LIB)
  
  IF (SCALP_LPSOLVE_LIB)
	set(SCALP_LIBS ${SCALP_LIBS} ${SCALP_LPSOLVE_LIB})
  ENDIF (SCALP_LPSOLVE_LIB)
  
  MESSAGE(STATUS "Found SCALP: ${SCALP_H},${SCALP_LIB}")
  MESSAGE(STATUS "      Using the following ILP solver libraries: ${SCALP_LIBS}")  
ELSE (SCALP_FOUND)
  MESSAGE(STATUS "Could not find SCALP, no ILP solver available (used for advanced bit heap compression and large multipliers), download and compile from https://digidev.digi.e-technik.uni-kassel.de/scalp/ (found: ${SCALP_H},${SCALP_LIB} and one of the following: ${SCALP_CPLEX_LIB},${SCALP_GUROBI_LIB},${SCALP_SCIP_LIB},${SCALP_LPSOLVE_LIB})")
ENDIF (SCALP_FOUND)

ADD_DEFINITIONS(-DHAVE_LNS)

#
# Create custom command for flex++/lex (note the outputs)
FIND_PROGRAM(FLEXPP_EXECUTABLE
  NAMES flex
  PATHS /usr/bin
  DOC "Flex++ executable")

IF(FLEXPP_EXECUTABLE)
  MESSAGE(STATUS "Found lex: ${FLEXPP_EXECUTABLE}")
ELSE(FLEXPP_EXECUTABLE)
  MESSAGE(FATAL_ERROR "Could not find LEX.\nOn Debian-based systems install flex++")
ENDIF(FLEXPP_EXECUTABLE)

# necessary to include generated files
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/src/FPExpressions ${GMP_H} ${GMPXX_H} ${MPFI_H} ${MPFR_H} ${SOLLYA_H})

ADD_CUSTOM_COMMAND(
  OUTPUT VHDLLexer.cpp
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/VHDLLexerRule.l
  COMMAND ${FLEXPP_EXECUTABLE} 
  ARGS -o ${CMAKE_CURRENT_BINARY_DIR}/VHDLLexer.cpp 
  ${CMAKE_CURRENT_SOURCE_DIR}/src/VHDLLexerRule.l
  )

#SET_SOURCE_FILES_PROPERTIES(VHDLLexer GENERATED) 


# Create custom command for bison/yacc (note the outputs)
FIND_PROGRAM(BISON_EXECUTABLE
  NAMES bison
  PATHS /usr/bin
  DOC "Bison executable")

IF(BISON_EXECUTABLE)
  MESSAGE(STATUS "Found bison: ${BISON_EXECUTABLE}")
ELSE(BISON_EXECUTABLE)
  MESSAGE(FATAL_ERROR "Could not find Bison.\nOn Debian-based systems install bison")
ENDIF(BISON_EXECUTABLE)

ADD_CUSTOM_COMMAND(
  OUTPUT ExpressionParser.cpp
  COMMAND ${BISON_EXECUTABLE}
  ARGS -y -d --defines=${CMAKE_CURRENT_BINARY_DIR}/ExpressionParser.h --output=${CMAKE_CURRENT_BINARY_DIR}/ExpressionParser.cpp --name-prefix=FlopocoExpression
  ${CMAKE_CURRENT_SOURCE_DIR}/src/FPExpressions/ExpressionParser.y
  )

SET_SOURCE_FILES_PROPERTIES(ExpressionParser GENERATED) 

# Create custom command for flex/lex (note the outputs)
FIND_PROGRAM(FLEX_EXECUTABLE
  NAMES flex
  PATHS /usr/bin
  DOC "Flex executable")

IF(FLEX_EXECUTABLE)
  MESSAGE(STATUS "Found flex: ${FLEX_EXECUTABLE}")
ELSE(FLEX_EXECUTABLE)
  MESSAGE(FATAL_ERROR "Could not find LEX.\nOn Debian-based systems install flex++")
ENDIF(FLEX_EXECUTABLE)

ADD_CUSTOM_COMMAND(
  OUTPUT ExpressionScanner.cpp
  COMMAND ${FLEX_EXECUTABLE} 
  ARGS --prefix=FlopocoExpression --header-file=${CMAKE_CURRENT_BINARY_DIR}/FlopocoScanner.h -o ${CMAKE_CURRENT_BINARY_DIR}/ExpressionScanner.cpp 
  ${CMAKE_SOURCE_DIR}/src/FPExpressions/ExpressionScanner.l
  )

SET_SOURCE_FILES_PROPERTIES(ExpressionScanner GENERATED)

#All operators active in flopoco (not only the ones with an interface)
include(ActiveOperators.txt)

set_target_properties(FloPoCoLib PROPERTIES OUTPUT_NAME "FloPoCo")

TARGET_LINK_LIBRARIES(
  FloPoCoLib 
  ${GMP_LIB} ${GMPXX_LIB} ${MPFI_LIB} ${MPFR_LIB} #xml2 ??xml2 not necessary??
  )

IF (SOLLYA_LIB)
  TARGET_LINK_LIBRARIES(
	FloPoCoLib
	${SOLLYA_LIB}
	)
ENDIF (SOLLYA_LIB)

IF (PYTHON_LIB)
  TARGET_LINK_LIBRARIES(
	FloPoCoLib
	${PYTHON_LIB}
	)
ENDIF (PYTHON_LIB)

IF (WCPG_FOUND)
  TARGET_LINK_LIBRARIES(
	FloPoCoLib
	${WCPG_LIB}
	)
  INCLUDE_DIRECTORIES(${WCPG_H})  
ENDIF (WCPG_FOUND)

IF (SCALP_FOUND)
  TARGET_LINK_LIBRARIES(
	FloPoCoLib
	${SCALP_LIBS}
	)
  INCLUDE_DIRECTORIES(${SCALP_H})
ENDIF (SCALP_FOUND)

ADD_EXECUTABLE(flopoco
  src/main 
  )

TARGET_LINK_LIBRARIES(
  flopoco  
  FloPoCoLib 
  )

# unplugged for now to speed up compilation, to revive some day
#IF (PYTHON_FOUND)
#ADD_EXECUTABLE(flopoco_soaktest
# src/main_soaktest
#)
#TARGET_LINK_LIBRARIES(
#  flopoco_soaktest  
#	${FLOPOCO_LIB}
#)
#INCLUDE_DIRECTORIES(${PYTHON_H})  
#ENDIF (PYTHON_FOUND)



ADD_EXECUTABLE(fp2bin src/Tools/fp2bin  src/utils)
TARGET_LINK_LIBRARIES(fp2bin ${MPFR_LIB} ${GMP_LIB} ${GMPXX_LIB})

ADD_EXECUTABLE(bin2fp src/Tools/bin2fp  src/utils)
TARGET_LINK_LIBRARIES(bin2fp ${MPFR_LIB} ${GMP_LIB} ${GMPXX_LIB})

ADD_EXECUTABLE(longacc2fp src/Tools/longacc2fp  src/utils)
TARGET_LINK_LIBRARIES(longacc2fp ${MPFR_LIB} ${GMP_LIB} ${GMPXX_LIB})



#add_subdirectory(src/random)
#target_link_libraries(FloPoCoLib flopoco_random_lib)
