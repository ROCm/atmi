# ===--------------------------------------------------------------------------
#               ATMI (Asynchronous Task and Memory Interface)
#
#  This file is distributed under the MIT License. See LICENSE.txt for details.
# ===--------------------------------------------------------------------------

if(ROCM_FOUND)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86_64)|(ppc64le)|(aarch64)$" AND CMAKE_SYSTEM_NAME MATCHES "Linux")
        libatmi_runtime_say("Preparing to build runtime/core")
        add_subdirectory(core) 
        add_subdirectory(interop) 
    else()
        libatmi_runtime_say("Not building ATMI Runtime: only support ATMI in Linux x86_64 or ppc64le hosts.")
    endif()
else()
    libatmi_runtime_say("Not building ATMI Runtime: libhsa-runtime64 not found")
endif()
