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

if(ATMI_DEP_LIBHSA_FOUND)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86_64)|(ppc64le)$" AND CMAKE_SYSTEM_NAME MATCHES "Linux")
        libatmi_runtime_say("Preparing to build runtime/core")
        add_subdirectory(core) 

        # compile interop layer if specified explicitly
        string( TOLOWER "${ATMI_HSA_INTEROP}" ATMI_HSA_INTEROP_VAR )
        if(ATMI_HSA_INTEROP_VAR MATCHES on )
            add_subdirectory(interop) 
        endif()

    else()
        libatmi_runtime_say("Not building ATMI Runtime: only support ATMI in Linux x86_64 or ppc64le hosts.")
    endif()
else()
    libatmi_runtime_say("Not building ATMI Runtime: libhsa-runtime64 not found")
endif()
