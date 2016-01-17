# vim:ts=2:sw=2:et
cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
project(eixx VERSION 1.4)

#===============================================================================
# CMAKE options customization
#===============================================================================
option(VERBOSE                 "Turn verbosity on|off"                      OFF)
#set(DEBUG "vars")

if(VERBOSE)
  set(CMAKE_VERBOSE_MAKEFILE ON)
endif()
if(WITH_ENUM_SERIALIZATION)
  set(UTXX_ENUM_SUPPORT_SERIALIZATION ON)
endif()

string(TOLOWER ${TOOLCHAIN} toolchain)
string(TOUPPER "${PROJECT_NAME}-${TOOLCHAIN}" BUILD_TYPE)

# Custom extensions
include(${CMAKE_CURRENT_SOURCE_DIR}/build-aux/CMakeEx.txt)

#-------------------------------------------------------------------------------
# Toolchain
#-------------------------------------------------------------------------------
# See also build/CMakeInit.txt
if("${toolchain}" STREQUAL "gcc")
  set(CMAKE_C_COMPILER   "gcc")
  set(CMAKE_CXX_COMPILER "g++")
  add_definitions(-Wno-strict-aliasing -fopenmp -Wall)

  if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    add_definitions(-flto -funroll-loops -fomit-frame-pointer)

    # The following will omit all symbol information from the build:
    #add_definitions(-Wl,-s)
    #set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -s")
  endif()

  #if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
  #    add_definitions(-flto -funroll-loops -fomit-frame-pointer -Wl,-s)
  #    set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -s")
  #endif()
elseif("${toolchain}" STREQUAL "intel")
  set(CMAKE_C_COMPILER   "icc")
  set(CMAKE_CXX_COMPILER "icpc")
  add_definitions(-openmp)

elseif("${toolchain}" STREQUAL "clang")
  set(CMAKE_C_COMPILER   "clang")
  set(CMAKE_CXX_COMPILER "clang++")
  add_definitions(-Wall)
else()
  message(FATAL_ERROR "Invalid toolchain: ${TOOLCHAIN}")
endif()

# Note: explicit c++14 definitions done in CMakeInit.txt.
# Alternative is to set for each target:
# target_compile_features(${PROJECT_NAME} PRIVATE cxx_lambda_init_captures)

add_definitions(
	-D_REENTRANT
	-Wno-unused-local-typedefs
  -Wno-deprecated-declarations
	-DBOOST_SYSTEM_NO_DEPRECATED
)

message(STATUS "${ClrBold}Configuring for the ${TOOLCHAIN} toolchain${ClrReset}")

#-------------------------------------------------------------------------------
# Policies
#-------------------------------------------------------------------------------
# Don't curse at non-existing dependencies (since we use code generation)
cmake_policy(SET CMP0046 OLD)

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
# Dependent packages and their directory locations
#-------------------------------------------------------------------------------
find_package(PkgConfig)
find_program(ERL   erl)
find_package(OpenSSL REQUIRED)

set(PKG_ROOT_DIR "/opt/pkg" CACHE STRING "Package root directory")

# Python
find_package(PythonInterp)

# Boost (with local modifications):
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREAD ON)
set(Boost_NO_SYSTEM_PATHS ON)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/build")

find_package(Boost 1.55.0 REQUIRED COMPONENTS
             system filesystem date_time program_options thread regex
             unit_test_framework timer)

if(Boost_FOUND)
  #include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
  #link_directories(${Boost_LIBRARY_DIRS})
  set(UTXX_HAVE_BOOST_TIMER_TIMER_HPP 1)
	message(STATUS "Found boost: ${Boost_LIBRARY_DIRS}")
endif()

set(Boost_LIBRARIES
  ${Boost_SYSTEM_LIBRARY}
  ${Boost_THREAD_LIBRARY}
  ${Boost_REGEX_LIBRARY}
  ${Boost_DATE_TIME_LIBRARY}
  ${Boost_FILESYSTEM_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  ${Boost_IOSTREAMS_LIBRARY}
)

execute_process(
  COMMAND ${ERL} -eval "io:format(\"~s\", [code:root_dir()]), halt()."
                 -noshell -noinput
  OUTPUT_VARIABLE Erl_DIR
)
execute_process(
  COMMAND ${ERL} -eval "io:format(\"~s\", [code:lib_dir(erl_interface)]), halt()."
                 -noshell -noinput
  OUTPUT_VARIABLE Erl_ERL_INTERFACE_DIR
)
message(STATUS "Erl interface: ${Erl_ERL_INTERFACE_DIR}")

set(AddCleanFiles)  # Additional clean files

#-------------------------------------------------------------------------------
# MAKE options
#-------------------------------------------------------------------------------

#add_custom_target(vars
#  COMMAND ${CMAKE_COMMAND} -H${CMAKE_SOURCE_DIR} -B${CMAKE_BINARY_DIR} -LA
#)

include_directories(
  SYSTEM
  ${Boost_INCLUDE_DIRS}
  ${OPENSSL_INCLUDE_DIR}
  ${Erl_DIR}/usr/include
  ${Erl_ERL_INTERFACE_DIR}/src
)
include_directories(
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_BINARY_DIR}/include
)
link_directories(
  ${Boost_LIBRARY_DIRS}
  ${Erl_DIR}/usr/lib
)

set(EIXX_LIBS
  ei
  ${OPENSSL_LIBRARIES}  # For MD5 support
  pthread
)

#-------------------------------------------------------------------------------
# Platform-specific checks
#-------------------------------------------------------------------------------
#include(${CMAKE_ROOT}/Modules/CheckTypeSize.cmake)
#include(${CMAKE_ROOT}/Modules/CheckFunctionExists.cmake)
#include(${CMAKE_ROOT}/Modules/CheckStructHasMember.cmake)
include(${CMAKE_ROOT}/Modules/CheckIncludeFile.cmake)

set(CMAKE_REQUIRED_INCLUDES ${Erl_ERL_INTERFACE_DIR}/src)
CHECK_INCLUDE_FILE(epmd/ei_epmd.h HAVE_EI_EPMD)
# Needed for pcap.hpp tests
#CHECK_STRUCT_HAS_MEMBER("struct tcphdr" th_flags netinet/tcp.h UTXX_HAVE_TCPHDR_TH_FLAGS_H)

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

add_subdirectory(src)
add_subdirectory(test)

#===============================================================================
# Installation
#===============================================================================
install(
  DIRECTORY   ${CMAKE_SOURCE_DIR}/include/${PROJECT_NAME}
  DESTINATION ${CMAKE_INSTALL_PREFIX}/include
  FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp" PATTERN "*.ipp" PATTERN "*.x??"
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
