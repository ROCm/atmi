#
#MIT License 
#
#Copyright © 2016 Advanced Micro Devices, Inc.  
#
#Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software
#without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
#persons to whom the Software is furnished to do so, subject to the following conditions:
#
#The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
#
#THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
#PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
#OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#

cmake_minimum_required (VERSION 2.8 FATAL_ERROR)
include (FindPackageHandleStandardArgs)

project (atmi)

macro(libatmi_runtime_say message_to_user)
  message(STATUS "ATMI: ${message_to_user}")
endmacro()

set (CMAKE_C_COMPILER g++)
# The version number.
set (atmi_VERSION_MAJOR 0)
set (atmi_VERSION_MINOR 3)

################################################################################
# Default install path to ROCM and not /usr/local
################################################################################
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set (CMAKE_INSTALL_PREFIX "/opt/rocm/libatmi" CACHE PATH "default install path" FORCE )
endif()
################################################################################
# Looking for ROCM...
################################################################################
#pkg_check_modules(ATMI_SEARCH_LIBHSA QUIET libhsa-runtime64)

find_file (
  ATMI_DEP_LIBHSA_INCLUDE
  hsa/hsa.h
  PATHS
  /opt/rocm/include
  /usr/local/include
  )
get_filename_component (ATMI_DEP_LIBHSA_INCLUDE_DIRS ${ATMI_DEP_LIBHSA_INCLUDE} DIRECTORY)

find_library (
  ATMI_DEP_LIBHSA_LIBRARIES
  libhsa-runtime64.so
  PATHS
  /opt/rocm/lib
  /usr/local/lib
  )
get_filename_component (ATMI_DEP_LIBHSA_LIBRARIES_DIRS ${ATMI_DEP_LIBHSA_LIBRARIES} DIRECTORY)

find_package_handle_standard_args(
  ATMI_DEP_LIBHSA
  DEFAULT_MSG
  ATMI_DEP_LIBHSA_LIBRARIES_DIRS
  ATMI_DEP_LIBHSA_INCLUDE_DIRS)

mark_as_advanced(
  ATMI_DEP_LIBHSA_INCLUDE_DIRS
  ATMI_DEP_LIBHSA_LIBRARIES_DIRS)
 
add_subdirectory(runtime) 
add_subdirectory(compiler) 
