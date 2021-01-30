# ===--------------------------------------------------------------------------
#               ATMI (Asynchronous Task and Memory Interface)
#
#  This file is distributed under the MIT License. See LICENSE.txt for details.
# ===--------------------------------------------------------------------------

string( TOLOWER "${ATMI_DEVICE_RUNTIME}" ATMI_DEVICE_RUNTIME_VAR )
if(NOT ATMI_DEVICE_RUNTIME_VAR MATCHES on )
  libatmi_runtime_say("Not building ATMI Device Runtime. Use -DATMI_DEVICE_RUNTIME=on in your cmake options to enable.")
  return()
endif()

#set(amdgcn_mcpus fiji kaveri carrizo hawaii)
#set(amdgcn_mcpus gfx803 gfx700 gfx801 gfx701 gfx800 gfx900 gfx901)
set(amdgcn_mcpus gfx803)

# Target triple
if (NOT DEFINED AMDGPU_TARGET_TRIPLE)
  set(AMDGPU_TARGET_TRIPLE "amdgcn-amd-amdhsa")
endif (NOT DEFINED AMDGPU_TARGET_TRIPLE)

#Find clang
find_package(LLVM QUIET REQUIRED CONFIG PATHS ${ROC_DIR} ${LLVM_DIR} NO_DEFAULT_PATH)

if(LLVM_FOUND)
  message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}. Configure: ${LLVM_DIR}/LLVMConfig.cmake")

  #LLVM Compiler info
  set(CLANG_INSTALL_PREFIX ${LLVM_INSTALL_PREFIX})
  set(CLANG_MAIN_INCDIR ${LLVM_BUILD_MAIN_INCLUDE_DIR})

  if (CLANG_INSTALL_PREFIX)
    set(CLANG_BINDIR ${CLANG_INSTALL_PREFIX}/bin)
    set(CLANG_INCDIR ${CLANG_INSTALL_PREFIX}/include)
    set(CLANG_LIBDIR ${CLANG_INSTALL_PREFIX}/lib)
    set(AMDLLVM ${CLANG_INSTALL_PREFIX})
  else()
    set(CLANG_BINDIR ${LLVM_BUILD_BINARY_DIR}/bin)
    set(CLANG_INCDIR ${LLVM_BUILD_BINARY_DIR}/include)
    set(CLANG_LIBDIR ${LLVM_LIBRARY_DIRS})
    set(AMDLLVM ${LLVM_BUILD_BINARY_DIR})
  endif()
else()
    libatmi_runtime_say("Not building ATMI Device Runtime: LLVM not found or path not set using LLVM_DIR.")
    return()
endif()

#Cmake pattern for checking envrionment
set(GFXVAR $ENV{GFXLIST})

#Set mcpus
if (NOT GFXVAR)
  execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../../bin/mygpu
    OUTPUT_VARIABLE MYGPU
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  #message(STATUS "      GPU detected: ${MYGPU}")
  set(mcpus ${MYGPU})
else()
  set(mcpus $ENV{GFXLIST})
endif()
separate_arguments(mcpus)

# Find AMD device libs needed for device runtime
find_package(AMDDeviceLibs REQUIRED CONFIG PATHS ${ROC_DIR})
find_path(ATMI_DE_DEP_LIBHSA_LIBRARIES
  NAMES
    "ockl.bc"
    "ockl.amdgcn.bc"
  PATHS
    "${AMD_DEVICE_LIBS_PREFIX}/amdgcn/bitcode"
    "${AMD_DEVICE_LIBS_PREFIX}/lib/bitcode"
    "${AMD_DEVICE_LIBS_PREFIX}/lib"
  NO_DEFAULT_PATH)

get_filename_component (ATMI_DE_DEP_LIBHSA_LIBRARIES_DIRS ${ATMI_DE_DEP_LIBHSA_LIBRARIES} DIRECTORY)

find_package_handle_standard_args(
  ATMI_DE_DEP_LIB
  "One of these files not found: {ockl.bc}"
  ATMI_DE_DEP_LIBHSA_LIBRARIES_DIRS)

if(ATMI_DE_DEP_LIB_FOUND)
  libatmi_runtime_say("Preparing to build device_runtime with GFX_VER=${GFX_VER}")
else()
  libatmi_runtime_say("Not building ATMI Device Runtime: ROCm Device Libs not found.")
  return()
endif()

# Build
mark_as_advanced(
  ATMI_DE_DEP_LIBHSA_LIBRARIES_DIRS)

get_filename_component (ROCM_DEVICE_PATH
  ${ATMI_DE_DEP_LIBHSA_LIBRARIES_DIRS} DIRECTORY
  )

include(bc.cmake)

file(GLOB sources
  ${CMAKE_CURRENT_SOURCE_DIR}/*.ll
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cl
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cu
  )

# set output dir for .h files
set (OUTPUT_INC_DIRECTORY ${ATMI_RUNTIME_PATH}/include)

# create output dir for include files
execute_process(COMMAND "/bin/mkdir" "-p" "${OUTPUT_INC_DIRECTORY}")
add_custom_command(
  OUTPUT ${OUTPUT_INC_DIRECTORY}/atmi_kl.h
  COMMAND /usr/bin/rsync ${CMAKE_CURRENT_SOURCE_DIR}/../../include/atmi_kl.h ${OUTPUT_INC_DIRECTORY}/atmi_kl.h
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../../include/*.h
  )
add_custom_target(device_header ALL DEPENDS ${OUTPUT_INC_DIRECTORY}/atmi_kl.h)

# append to the configure file
file(APPEND ${ATMI_RUNTIME_PATH}/atmi-config.mak "AMDLLVM=${AMDLLVM}\n")
file(APPEND ${ATMI_RUNTIME_PATH}/atmi-config.mak "AMDGPU_TARGET_TRIPLE=${AMDGPU_TARGET_TRIPLE}\n")
# cloc assumes the /path/to/rocm/lib (with the /lib suffix in the path)
file(APPEND ${ATMI_RUNTIME_PATH}/atmi-config.mak "ROCM_DEVICE_PATH=${ROCM_DEVICE_PATH}/amdgcn/bitcode\n")

# create bc files
foreach(mcpu ${mcpus})
  execute_process(COMMAND /bin/bash "-c" "/bin/echo ${mcpu} | /bin/sed 's/gfx//'"
    OUTPUT_VARIABLE GFXNUM
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  libatmi_runtime_say("build device_runtime with GFXNUM=${GFXNUM}")

  set(optimization_level 2)

  # set output dir for .bc files
  set(OUTPUTDIR ${ATMI_RUNTIME_PATH}/lib)

  # create output dir for include files
  execute_process(COMMAND "/bin/mkdir" "-p" "${OUTPUTDIR}")

  add_bc_library(atmi ${CMAKE_CURRENT_SOURCE_DIR} ${sources})

  target_sources(atmi_runtime PUBLIC
    ${PROJECT_SOURCE_DIR}/device_runtime/device_rt.cpp 
    )
  include_directories(${ROCM_INCLUDE_DIRS})
  include_directories(${CMAKE_CURRENT_SOURCE_DIR})
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../include)
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../runtime/include)

  install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/atmi_kl.h
    DESTINATION include COMPONENT device_runtime)

  if(CMAKE_BUILD_TYPE MATCHES Debug)
    install(FILES
      ${OUTPUTDIR}/atmi-${mcpu}.bc
      DESTINATION lib-debug COMPONENT device_runtime
      )
  else()
    install(FILES
      ${OUTPUTDIR}/atmi-${mcpu}.bc
      DESTINATION lib COMPONENT device_runtime
      )
  endif()
endforeach()

