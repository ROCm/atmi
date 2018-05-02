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


macro(collect_sources name dir)
  set(cuda_sources)
  set(ocl_sources)
  set(llvm_sources)

  foreach(file ${ARGN})
    file(RELATIVE_PATH rfile ${dir} ${file})
    get_filename_component(rdir ${rfile} DIRECTORY)
    get_filename_component(fname ${rfile} NAME_WE)
    get_filename_component(fext ${rfile} EXT)
    #file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${rdir})
    if (fext STREQUAL ".cu")
      set(cfile ${CMAKE_CURRENT_BINARY_DIR}/${rdir}/${fname}.cu)
      list(APPEND cuda_sources ${cfile})
    endif()

    if (fext STREQUAL ".cl")
      set(cfile ${CMAKE_CURRENT_BINARY_DIR}/${rdir}/${fname}.cl)
      list(APPEND ocl_sources ${cfile})
    endif()
    if (fext STREQUAL ".ll")
      list(APPEND csources ${file})
      set(cfile ${CMAKE_CURRENT_BINARY_DIR}/${rdir}/${fname}.ll)
      list(APPEND llvm_sources ${cfile})
    endif()
  endforeach()
endmacro()

macro(add_llvm_bc_library name dir)
  set(ll_files)

  foreach(file ${ARGN})
    file(RELATIVE_PATH rfile ${dir} ${file})
    get_filename_component(rdir ${rfile} DIRECTORY)
    get_filename_component(fname ${rfile} NAME_WE)
    get_filename_component(fext ${rfile} EXT)

    list(APPEND ll_files ${CMAKE_CURRENT_SOURCE_DIR}/${fname}.ll)
  endforeach()

  add_custom_command(
    OUTPUT linkout.llvm.${mcpu}.bc
    COMMAND ${CLANG_BINDIR}/llvm-link ${ll_files} -o linkout.llvm.${mcpu}.bc
    DEPENDS ${ll_files}
    )

  list(APPEND bc_files linkout.llvm.${mcpu}.bc)
endmacro()

macro(add_ocl_bc_library name dir)
  set(cl_cmd ${CLANG_BINDIR}/clang
    -S -emit-llvm
    -DCL_VERSION_2_0=200 -D__OPENCL_C_VERSION__=200
    -fblocks
    -x cl -Xclang -cl-std=CL2.0 -Xclang -finclude-default-header
    -Dcl_khr_subgroups
    -Dcl_khr_fp64 -Dcl_khr_fp16
    -Dcl_khr_int64_base_atomics -Dcl_khr_int64_extended_atomics
    -target ${AMDGPU_TARGET_TRIPLE}
    -mcpu=${mcpu}
    -I${ATMI_DE_DEP_LIBHSA_INCLUDE_DIRS}
    -I${CMAKE_CURRENT_SOURCE_DIR}
    -I${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${CLANG_OPTIONS_APPEND})

  set(ll_files)

  foreach(file ${ARGN})
    file(RELATIVE_PATH rfile ${dir} ${file})
    get_filename_component(rdir ${rfile} DIRECTORY)
    get_filename_component(fname ${rfile} NAME_WE)
    get_filename_component(fext ${rfile} EXT)

    set(ll_filename ${fname}.${mcpu}.ll)

    file(GLOB h_files 
      "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
      "${CMAKE_CURRENT_SOURCE_DIR}/../../include/*.h")

    add_custom_command(
      OUTPUT ${ll_filename}
      COMMAND ${cl_cmd} ${CMAKE_CURRENT_SOURCE_DIR}/${fname}.cl -o ${ll_filename}
      DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${fname}.cl" ${h_files}
      )

    list(APPEND ll_files ${ll_filename})
  endforeach()

  add_custom_command(
    OUTPUT linkout.ocl.${mcpu}.bc
    COMMAND ${CLANG_BINDIR}/llvm-link ${ll_files} -o linkout.ocl.${mcpu}.bc
    DEPENDS ${ll_files}
    )

  list(APPEND bc_files linkout.ocl.${mcpu}.bc)
endmacro()

macro(add_cuda_bc_library name dir)
  set(cu_cmd ${CLANG_BINDIR}/clang++
    -S -emit-llvm
    --cuda-device-only
    -nocudalib
    -O${optimization_level}
    -DGPUCC_AMDGCN
    --cuda-gpu-arch=${mcpu}
    ${CUDA_DEBUG}
    -I${CMAKE_CURRENT_SOURCE_DIR})

  set(ll_files)

  foreach(file ${ARGN})
    file(RELATIVE_PATH rfile ${dir} ${file})
    get_filename_component(rdir ${rfile} DIRECTORY)
    get_filename_component(fname ${rfile} NAME_WE)
    get_filename_component(fext ${rfile} EXT)

    set(ll_filename ${fname}.${mcpu}.ll)

    file(GLOB h_files "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

    add_custom_command(
      OUTPUT ${ll_filename}
      COMMAND ${cu_cmd} ${CMAKE_CURRENT_SOURCE_DIR}/${fname}.cu -o ${ll_filename}
      DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${fname}.cu" ${h_files}
      )

    list(APPEND ll_files ${ll_filename})
  endforeach()

  add_custom_command(
    OUTPUT linkout.cuda.${mcpu}.bc
    COMMAND ${CLANG_BINDIR}/llvm-link ${ll_files} -o linkout.cuda.${mcpu}.bc
    DEPENDS ${ll_files}
    )

  list(APPEND bc_files linkout.cuda.${mcpu}.bc)
endmacro()

macro(add_bc_library name dir)
  set(bc_files)

  collect_sources(${name} ${dir} ${ARGN})

  if (llvm_sources)
    add_llvm_bc_library(${name} ${dir} ${llvm_sources})
  else()
    #message(STATUS "No LLVM IR source.")
  endif()
  if (ocl_sources)
    add_ocl_bc_library(${name} ${dir} ${ocl_sources})
  else()
    #message(STATUS "No OpenCL source.")
  endif()
  if (cuda_sources)
    add_cuda_bc_library(${name} ${dir} ${cuda_sources})
  else()
    #message(STATUS "No CUDA source.")
  endif()

  set(device_libs)
  if(${ROCM_DEVICE_PATH} MATCHES .*amdgcn.*)
  else()
    list(APPEND device_libs ${ROCM_DEVICE_PATH}/lib/opencl.amdgcn.bc)
    list(APPEND device_libs ${ROCM_DEVICE_PATH}/lib/ocml.amdgcn.bc)
    list(APPEND device_libs ${ROCM_DEVICE_PATH}/lib/ockl.amdgcn.bc)
    list(APPEND device_libs ${ROCM_DEVICE_PATH}/lib/oclc_correctly_rounded_sqrt_off.amdgcn.bc)
    list(APPEND device_libs ${ROCM_DEVICE_PATH}/lib/oclc_daz_opt_off.amdgcn.bc)
    list(APPEND device_libs ${ROCM_DEVICE_PATH}/lib/oclc_finite_only_off.amdgcn.bc)
    list(APPEND device_libs ${ROCM_DEVICE_PATH}/lib/oclc_isa_version_${GFXNUM}.amdgcn.bc)
    list(APPEND device_libs ${ROCM_DEVICE_PATH}/lib/oclc_unsafe_math_off.amdgcn.bc)
    list(APPEND device_libs ${ROCM_DEVICE_PATH}/lib/irif.amdgcn.bc)
  endif()

  add_custom_command(
    OUTPUT linkout.${mcpu}.bc
    COMMAND ${CLANG_BINDIR}/llvm-link ${bc_files} ${device_libs} -o linkout.${mcpu}.bc
    DEPENDS ${bc_files}
    )
  add_custom_command(
    OUTPUT optout.${mcpu}.bc
    COMMAND ${CLANG_BINDIR}/opt -O${optimization_level} linkout.${mcpu}.bc -o optout.${mcpu}.bc
    DEPENDS linkout.${mcpu}.bc
    )

  if(${ROCM_DEVICE_PATH} MATCHES .*amdgcn.*)
    add_custom_command(
      OUTPUT lib${name}-${mcpu}.bc
      COMMAND ${CMAKE_CURRENT_BINARY_DIR}/../prepare-builtins optout.${mcpu}.bc -o ${OUTPUTDIR}/lib${name}-${mcpu}.bc
      DEPENDS optout.${mcpu}.bc prepare-builtins
    )
    add_custom_target(lib${name}-${mcpu} ALL DEPENDS lib${name}-${mcpu}.bc)
  else()
    add_custom_command(
      OUTPUT atmi.amdgcn.bc
      COMMAND /bin/cp optout.${mcpu}.bc ${ATMI_RUNTIME_PATH}/lib/atmi.amdgcn.bc
      DEPENDS optout.${mcpu}.bc
    )
    add_custom_target(atmi.amdgcn ALL DEPENDS atmi.amdgcn.bc)
  endif()
endmacro()

