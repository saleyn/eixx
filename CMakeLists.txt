# vim:ts=2:sw=2:et
cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

# Overrides - must be placed before 'project'
set(CMAKE_USER_MAKE_RULES_OVERRIDE "${CMAKE_SOURCE_DIR}/build-aux/CMakeInit.txt")

project(eixx VERSION 1.4)

#===============================================================================
# CMAKE options customization
#===============================================================================
option(VERBOSE                 "Turn verbosity on|off"                      OFF)
option(EIXX_MARSHAL_ONLY       "Limit build to eterm marshal lib on|off"    OFF)

if(VERBOSE)
  set(CMAKE_VERBOSE_MAKEFILE ON)
endif()
if(WITH_ENUM_SERIALIZATION)
  set(UTXX_ENUM_SUPPORT_SERIALIZATION ON)
endif()

string(TOLOWER ${TOOLCHAIN} toolchain)
string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE)

# Custom extensions
include(${CMAKE_CURRENT_SOURCE_DIR}/build-aux/CMakeEx.txt)

#-------------------------------------------------------------------------------
# Toolchain
#-------------------------------------------------------------------------------
# See also build/CMakeInit.txt
if("${toolchain}" STREQUAL "gcc")
  if(NOT CMAKE_C_COMPILER)
    set(CMAKE_C_COMPILER   "gcc")
  endif()
  if(NOT CMAKE_CXX_COMPILER)
    set(CMAKE_CXX_COMPILER "g++")
  endif()
  add_definitions(-fopenmp -Wall -Wextra -Wpedantic -Wshadow -Wno-strict-aliasing -Wconversion -Wsign-conversion)

  if("${CMAKE_BUILD_TYPE}" STREQUAL "release")
    add_definitions(-flto -funroll-loops -fomit-frame-pointer)

    # The following will omit all symbol information from the build:
    #add_definitions(-Wl,-s)
    #set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -s")
  endif()
elseif("${toolchain}" STREQUAL "intel")
  set(CMAKE_C_COMPILER   "icc")
  set(CMAKE_CXX_COMPILER "icpc")
  add_definitions(-openmp)

elseif("${toolchain}" STREQUAL "clang")
  set(CMAKE_C_COMPILER   "clang")
  set(CMAKE_CXX_COMPILER "clang++")
  add_definitions(-Wall -Wextra -Wpedantic -Wshadow -Wno-strict-aliasing -Wconversion -Wsign-conversion)
else()
  message(FATAL_ERROR "Invalid toolchain: ${TOOLCHAIN}")
endif()

# Append "_d" to the library when doing debug build
if (CMAKE_BUILD_TYPE STREQUAL "debug")
  set(LIB_SUFFIX "_d")
endif()

# Note: explicit c++14 definitions done in CMakeInit.txt.
# Alternative is to set for each target:
# target_compile_features(${PROJECT_NAME} PRIVATE cxx_lambda_init_captures)

add_definitions(
	-D_REENTRANT
	-Wno-unused-local-typedefs
  -Wno-deprecated-declarations
  -Wpedantic
	-DBOOST_SYSTEM_NO_DEPRECATED
  -DBOOST_BIND_GLOBAL_PLACEHOLDERS
)

message(STATUS "Configuring for the "
               "${BoldMagenta}${TOOLCHAIN}${ClrReset} toolchain "
               "${BoldMagenta}${CMAKE_BUILD_TYPE}${ClrReset} build")

#-------------------------------------------------------------------------------
# Policies
#-------------------------------------------------------------------------------

# RPATH configuration
# ===================
# Don't skip the full RPATH for the build tree
set(CMAKE_SKIP_BUILD_RPATH  FALSE)
# When building, don't use the install RPATH already
# (but later on when installing)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "${CMAKE_BINARY_DIR}/src:${CMAKE_INSTALL_PREFIX}/lib")
# Add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

#-------------------------------------------------------------------------------
# Custom cmake libs
#-------------------------------------------------------------------------------
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/build-aux)

#include(${CMAKE_ROOT}/Modules/CheckTypeSize.cmake)
#include(${CMAKE_ROOT}/Modules/CheckFunctionExists.cmake)
#include(${CMAKE_ROOT}/Modules/CheckStructHasMember.cmake)
include(${CMAKE_ROOT}/Modules/CheckIncludeFile.cmake)
include(${CMAKE_SOURCE_DIR}/build-aux/AlignOf.cmake)
#include(${CMAKE_SOURCE_DIR}/build-aux/FindErlang.cmake)

#-------------------------------------------------------------------------------
# Dependent packages and their directory locations
#-------------------------------------------------------------------------------
find_package(PkgConfig)
find_package(OpenSSL REQUIRED)
find_package(Erlang  REQUIRED)

set(PKG_ROOT_DIR "/opt/pkg" CACHE STRING "Package root directory")

# Boost (with local modifications):
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREAD ON)
set(Boost_NO_SYSTEM_PATHS ON)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/build")

find_package(Boost 1.55.0 REQUIRED COMPONENTS system thread)

if(Boost_FOUND)
  #include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
  #link_directories(${Boost_LIBRARY_DIRS})
  set(UTXX_HAVE_BOOST_TIMER_TIMER_HPP 1)
	message(STATUS "Found boost: ${Boost_LIBRARY_DIRS}")
endif()

#-------------------------------------------------------------------------------
# Platform-specific checks
#-------------------------------------------------------------------------------
set(CMAKE_REQUIRED_INCLUDES ${ERLANG_EI_PATH}/src)
CHECK_INCLUDE_FILE(epmd/ei_epmd.h HAVE_EI_EPMD)
unset(CMAKE_REQUIRED_INCLUDES)
ALIGNOF(uint64_t cpp UINT64_T)

#-------------------------------------------------------------------------------
# MAKE options
#-------------------------------------------------------------------------------

include_directories(
  SYSTEM
  ${Boost_INCLUDE_DIRS}
  ${OPENSSL_INCLUDE_DIR}
  ${Erlang_EI_INCLUDE_DIR}
  ${Erlang_EI_DIR}/src
)
include_directories(
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_BINARY_DIR}/include
)
link_directories(
  ${Boost_LIBRARY_DIRS}
  ${Erlang_EI_LIBRARY_DIR}
)

set(EIXX_LIBS
  ${Erlang_EI_LIBRARIES}
  ${OPENSSL_LIBRARIES}  # For MD5 support
  pthread
)

#-------------------------------------------------------------------------------
# Configure files
#-------------------------------------------------------------------------------
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
               "${CMAKE_CURRENT_BINARY_DIR}/include/${PROJECT_NAME}/config.h"
 							 @ONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.pc.in"
               "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc" @ONLY)
#-------------------------------------------------------------------------------
# Srcs and Targets:
#-------------------------------------------------------------------------------

add_subdirectory(${CMAKE_SOURCE_DIR}/src)
add_subdirectory(${CMAKE_SOURCE_DIR}/test)

#===============================================================================
# Installation
#===============================================================================
install(
  DIRECTORY   ${CMAKE_SOURCE_DIR}/include/${PROJECT_NAME}
  DESTINATION ${CMAKE_INSTALL_PREFIX}/include
  FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp" PATTERN "*.hxx"
)
install(
  FILES       ${CMAKE_CURRENT_BINARY_DIR}/include/${PROJECT_NAME}/config.h
  DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}
)
install(
	FILES 		  ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc
  DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/pkgconfig
)
install(
	FILES 		  ${CMAKE_SOURCE_DIR}/LICENSE
						  ${CMAKE_SOURCE_DIR}/README.md
	DESTINATION ${CMAKE_INSTALL_PREFIX}/share 
)

#===============================================================================
# Uninstallation
# Prereq: copy the uninstall.cmake file to the appropriate CMAKE_MODULE_PATH.
#===============================================================================
set_directory_properties(
  PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${AddCleanFiles}"
)

#add_custom_target(
#  uninstall "${CMAKE_COMMAND}" -P "${CMAKE_MODULE_PATH}/uninstall.cmake"
#)

#===============================================================================
# CTEST options
#===============================================================================
enable_testing()

add_test(test-eixx test/test-eterm -l message)

#===============================================================================
# Documentation options
#===============================================================================
# add a target to generate API documentation with Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/build-aux/Doxyfile.in
                 ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
                 @ONLY)
  add_custom_target(doc
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "${ClrBold}Generating API documentation with Doxygen${ClrReset}"
    VERBATIM
  )
endif()

# Post-install script (installation of symlinks):
install(SCRIPT ${CMAKE_SOURCE_DIR}/build-aux/install-symlinks.cmake)
