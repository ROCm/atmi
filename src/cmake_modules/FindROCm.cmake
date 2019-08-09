# ===--------------------------------------------------------------------------
#               ATMI (Asynchronous Task and Memory Interface)
#
#  This file is distributed under the MIT License. See LICENSE.txt for details.
# ===--------------------------------------------------------------------------

# Below are the variables that will be set at the end of this file
#  ROCM_FOUND
#  ROCM_LIBRARIES
#  ROCM_INCLUDE_DIRS
#  ROCM_VERSION
#  ROCM_VERSION_MAJOR
#  ROCM_VERSION_MINOR
#  ROCM_VERSION_PATCH
#  ROCM_VERSION_STRING

find_path(
  ROCM_INCLUDE_DIRS
    hsa.h
  HINTS
    ${ROC_DIR}/include
    ${ROCR_DIR}/include
    ${ROCR_DIR}/include/hsa
    ${ROCR_DIR}/hsa/include
    ${ROCR_DIR}/hsa/include/hsa
    ${ROCR_DIR}
    /opt/rocm/include
    /opt/rocm/hsa/include
    /usr/local/include
    ENV CPATH
  PATH_SUFFIXES
    hsa
)

find_library(
  ROCR_LIBRARY
    hsa-runtime64
  HINTS
    ${ROC_DIR}/lib
    ${ROCR_DIR}/lib
    ${ROCR_DIR}
    /opt/rocm/lib
    /opt/rocm/hsa/lib
    /usr/local/lib
    /usr/lib/x86_64-linux-gnu
    /usr/lib
    ENV LIBRARY_PATH
    ENV LD_LIBRARY_PATH
)
find_library(
  ROCT_LIBRARY
    hsakmt
  HINTS
    ${ROC_DIR}/lib
    ${ROCT_DIR}/lib
    ${ROCT_DIR}
    /opt/rocm/lib
    /opt/rocm/hsa/lib
    /usr/local/lib
    /usr/lib/x86_64-linux-gnu
    /usr/lib
    ENV LIBRARY_PATH
    ENV LD_LIBRARY_PATH
)
get_filename_component (ROCM_LIBRARIES_DIR ${ROCR_LIBRARY} DIRECTORY)
set(ROCM_LIBRARIES ${ROCR_LIBRARY} ${ROCT_LIBRARY})
#message(STATUS "ROCm libraries: ${ROCM_LIBRARIES}")
#message(STATUS "ROCm libraries dir: ${ROCM_LIBRARIES_DIR}")

if(NOT ROCM_VERSION)
  file(GLOB version_files
      LIST_DIRECTORIES false
      /opt/rocm/.info/version*
      )
  list(GET version_files 0 version_file)
  # Compute the version
  execute_process(
      COMMAND cat ${version_file}
      OUTPUT_VARIABLE _rocm_version
      ERROR_VARIABLE _rocm_error
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_STRIP_TRAILING_WHITESPACE
      )
  if(NOT _rocm_error)
    set(ROCM_VERSION ${_rocm_version} CACHE STRING "Version of ROCm as found in /opt/rocm/.info/version*")
  else()
    set(ROCM_VERSION "0.0.0" CACHE STRING "Version of ROCm set to default")
  endif()
  mark_as_advanced(ROCM_VERSION)
endif()

string(REPLACE "." ";"  _rocm_version_list "${ROCM_VERSION}")
list(GET _rocm_version_list 0 ROCM_VERSION_MAJOR)
list(GET _rocm_version_list 1 ROCM_VERSION_MINOR)
list(GET _rocm_version_list 2 ROCM_VERSION_PATCH)
set(ROCM_VERSION_STRING "${ROCM_VERSION}")

# set ROCM_FOUND to TRUE if the below variables are true,
# i.e. header and lib files are found
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ROCM DEFAULT_MSG
      ROCM_LIBRARIES
      ROCM_INCLUDE_DIRS
      ROCM_VERSION
      ROCM_VERSION_STRING
)

mark_as_advanced(
      ROCM_LIBRARIES
      ROCM_INCLUDE_DIRS
      ROCM_VERSION
      ROCM_VERSION_MAJOR
      ROCM_VERSION_MINOR
      ROCM_VERSION_PATCH
      ROCM_VERSION_STRING
)
