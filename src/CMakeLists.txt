# ===--------------------------------------------------------------------------
#               ATMI (Asynchronous Task and Memory Interface)
#
#  This file is distributed under the MIT License. See LICENSE.txt for details.
# ===--------------------------------------------------------------------------

cmake_minimum_required (VERSION 2.8 FATAL_ERROR)

set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

list ( APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules" )
include ( utils )
include (FindPackageHandleStandardArgs)

project (atmi)

# Set default libdir to be "lib" for ROCm, distros will override this anyway:
set(CMAKE_INSTALL_LIBDIR "lib" CACHE STRING "Library install directory")
include(GNUInstallDirs)
macro(libatmi_runtime_say_and_exit message_to_user)
  message(FATAL_ERROR "ATMI: ${message_to_user}")
endmacro()

macro(libatmi_runtime_say message_to_user)
  message(STATUS "ATMI: ${message_to_user}")
endmacro()

# The version number.
set (atmi_VERSION_MAJOR 0)
set (atmi_VERSION_MINOR 5)

################################################################################
# Default install path to ROCM and not /usr/local
################################################################################
if ( NOT DEFINED CMAKE_INSTALL_PREFIX )
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set (CMAKE_INSTALL_PREFIX "/opt/rocm" CACHE PATH "default install path" FORCE )
endif()
endif()

option(FILE_REORG_BACKWARD_COMPATIBILITY "Enable File Reorg with backward compatibility" ON)
#TODO: The string replace statements can be removed once build scripts changes are merged
if(FILE_REORG_BACKWARD_COMPATIBILITY)
  if(CMAKE_INSTALL_PREFIX)
    string(REPLACE "/atmi" "" CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
  endif()
  if(CPACK_PACKAGING_INSTALL_PREFIX)
    string(REPLACE "/atmi" "" CPACK_PACKAGING_INSTALL_PREFIX ${CPACK_PACKAGING_INSTALL_PREFIX})
  endif()
endif()
################################################################################
# Looking for ROCM...
################################################################################
#pkg_check_modules(ATMI_SEARCH_LIBHSA QUIET libhsa-runtime64)

if(ROCM_DIR)
  set(ROC_DIR ${ROCM_DIR})
else()
  if(HSA_DIR)
    set(ROC_DIR ${HSA_DIR})
  else()
    set(ROC_DIR "/opt/rocm")
  endif()
endif()

# Find ROCm
find_package(ROCm REQUIRED)
# libatmi_runtime_say("ROCm version: ${ROCM_VERSION_STRING}")
set(ATMI_RUNTIME_PATH ${CMAKE_CURRENT_BINARY_DIR})

# create configure file
file(WRITE ${ATMI_RUNTIME_PATH}/atmi-config.mak "ATMI_RUNTIME_PATH=${ATMI_RUNTIME_PATH}\n")

add_subdirectory(runtime)
add_subdirectory(compiler)
add_subdirectory(device_runtime)

if(FILE_REORG_BACKWARD_COMPATIBILITY)
  include(atmi-backward-compat.cmake)
endif()
# make examples available in local build
add_custom_command(
  OUTPUT examples
  COMMAND /usr/bin/rsync -rl ${CMAKE_CURRENT_SOURCE_DIR}/../examples .
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../examples/*
  )
add_custom_target(example ALL DEPENDS examples)

# make bin available in local build
add_custom_command(
  OUTPUT bin
  COMMAND /usr/bin/rsync -rl ${CMAKE_CURRENT_SOURCE_DIR}/../bin .
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../bin/*
  )
add_custom_target(script ALL DEPENDS bin)

## Get the package version. The defaults to 0.0.1.
get_version ( "0.0.1" )

set ( BUILD_VERSION_MAJOR ${VERSION_MAJOR} )
set ( BUILD_VERSION_MINOR ${VERSION_MINOR} )
set ( BUILD_VERSION_PATCH ${VERSION_PATCH} )
if ( ${VERSION_BUILD} )
    set ( BUILD_VERSION_PATCH "${BUILD_VERSION_PATCH}-${VERSION_BUILD}" )
endif ()
set ( BUILD_VERSION_STRING "${BUILD_VERSION_MAJOR}.${BUILD_VERSION_MINOR}.${BUILD_VERSION_PATCH}" )

## Packaging directives
set ( CPACK_PACKAGE_NAME "atmi" )
set ( CPACK_PACKAGE_VENDOR "Advanced Micro Devices, Inc." )
set ( CPACK_PACKAGE_VERSION_MAJOR ${BUILD_VERSION_MAJOR} )
set ( CPACK_PACKAGE_VERSION_MINOR ${BUILD_VERSION_MINOR} )
set ( CPACK_PACKAGE_VERSION_PATCH ${BUILD_VERSION_PATCH} )
set ( CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}" )
set ( CPACK_PACKAGE_CONTACT "ATMI Support <atmi.support@amd.com>" )
set ( CPACK_PACKAGE_DESCRIPTION_SUMMARY "Asynchronous Task and Memory Interface" )
set ( CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE.txt" )
install( FILES ${CPACK_RESOURCE_FILE_LICENSE} DESTINATION ${CMAKE_INSTALL_DOCDIR} COMPONENT runtime )

if ( NOT DEFINED CPACK_PACKAGING_INSTALL_PREFIX )
    set ( CPACK_PACKAGING_INSTALL_PREFIX /opt/rocm )
endif()
set ( CPACK_GENERATOR "RPM;DEB" CACHE STRING "CPACK Generator to use, e.g. DEB;RPM")

if ( DEFINED ENV{ROCM_LIBPATCH_VERSION} )
  set ( CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION}.$ENV{ROCM_LIBPATCH_VERSION}" )
  message ( "Using CPACK_PACKAGE_VERSION ${CPACK_PACKAGE_VERSION}" )
endif()

# Debian package specific variables
if ( DEFINED ENV{CPACK_DEBIAN_PACKAGE_RELEASE} )
  set ( CPACK_DEBIAN_PACKAGE_RELEASE $ENV{CPACK_DEBIAN_PACKAGE_RELEASE} )
else()
  set ( CPACK_DEBIAN_PACKAGE_RELEASE "local" )
endif()
message ( "Using CPACK_DEBIAN_PACKAGE_RELEASE ${CPACK_DEBIAN_PACKAGE_RELEASE}" )
set ( CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT" )

set ( CPACK_DEBIAN_PACKAGE_DEPENDS "hsa-rocr-dev, libc6" )
set ( CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/RadeonOpenCompute/atmi" )
set ( CPACK_DEBIAN_PACKAGE_DESCRIPTION "Asynchronous Task and Memory Interface, or ATMI, is a runtime framework
 for efficient task management in heterogeneous CPU-GPU systems." )
set ( CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/DEBIAN/postinst;${CMAKE_CURRENT_SOURCE_DIR}/DEBIAN/prerm"
)
# set ( CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON )
set ( CPACK_DEB_COMPONENT_INSTALL ON)

## RPM package specific variables
if ( DEFINED ENV{CPACK_RPM_PACKAGE_RELEASE} )
  set ( CPACK_RPM_PACKAGE_RELEASE $ENV{CPACK_RPM_PACKAGE_RELEASE} )
else()
  set ( CPACK_RPM_PACKAGE_RELEASE "local" )
endif()
message ( "Using CPACK_RPM_PACKAGE_RELEASE ${CPACK_RPM_PACKAGE_RELEASE}" )

## 'dist' breaks manual builds on debian systems due to empty Provides
execute_process( COMMAND rpm --eval %{?dist}
                 RESULT_VARIABLE PROC_RESULT
                 OUTPUT_VARIABLE EVAL_RESULT
                 OUTPUT_STRIP_TRAILING_WHITESPACE )
message("RESULT_VARIABLE ${PROC_RESULT} OUTPUT_VARIABLE: ${EVAL_RESULT}")

if ( PROC_RESULT EQUAL "0" AND NOT EVAL_RESULT STREQUAL "" )
  string ( APPEND CPACK_RPM_PACKAGE_RELEASE "%{?dist}" )
endif()
set ( CPACK_RPM_FILE_NAME "RPM-DEFAULT" )
message("CPACK_RPM_PACKAGE_RELEASE: ${CPACK_RPM_PACKAGE_RELEASE}")

set ( CPACK_RPM_PACKAGE_REQUIRES "hsa-rocr-dev" )
set ( CPACK_RPM_PRE_INSTALL_SCRIPT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/RPM/rpm_post" )
set ( CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/RPM/rpm_postun" )
set ( CPACK_RPM_COMPONENT_INSTALL ON)
set ( CPACK_RPM_PACKAGE_LICENSE "MIT" )

## Set components
set ( CPACK_COMPONENTS_ALL runtime cplugin device_runtime )
set ( CPACK_COMPONENTS_ALL_IN_ONE_PACKAGE 1 )

## Include packaging
include ( CPack )
