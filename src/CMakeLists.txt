cmake_minimum_required (VERSION 2.8 FATAL_ERROR)
include (FindPackageHandleStandardArgs)

project (atmi)

set (CMAKE_C_COMPILER g++)
# The version number.
set (atmi_VERSION_MAJOR 0)
set (atmi_VERSION_MINOR 3)

################################################################################
# Looking for ROCM...
################################################################################
#pkg_check_modules(ATMI_SEARCH_LIBHSA QUIET libhsa-runtime64)

find_path (
  ATMI_DEP_LIBHSA_INCLUDE_DIRS
  NAMES
  hsa.h
  PATHS
  $ENV{HSA_RUNTIME_PATH}/include
  /opt/rocm/include/hsa
  /usr/local/include
  )

find_path (
  ATMI_DEP_LIBHSA_LIBRARIES_DIRS
  NAMES
  libhsa-runtime64.so
  PATHS
  $ENV{HSA_RUNTIME_PATH}/lib
  /opt/rocm/lib/
  /usr/local/lib
  )

find_package_handle_standard_args(
  ATMI_DEP_LIBHSA
  DEFAULT_MSG
  ATMI_DEP_LIBHSA_LIBRARIES_DIRS
  ATMI_DEP_LIBHSA_INCLUDE_DIRS)

mark_as_advanced(
  ATMI_DEP_LIBHSA_INCLUDE_DIRS
  ATMI_DEP_LIBHSA_LIBRARIES_DIRS)
 
add_subdirectory(runtime) 
#add_subdirectory(compiler) 
