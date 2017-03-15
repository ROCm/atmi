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

string( TOLOWER "${ATMI_C_EXTENSION}" ATMI_C_EXTENSION_VAR )
if(NOT ATMI_C_EXTENSION_VAR MATCHES on )
  libatmi_runtime_say("Not building ATMI C Extension. Use -DATMI_C_EXTENSION=on in your cmake options to enable.")
  return()
endif()

if(ATMI_DEP_LIBHSA_FOUND)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86_64)|(ppc64le)$" AND CMAKE_SYSTEM_NAME MATCHES "Linux")
        set (EXECUTE_COMMAND ${CMAKE_C_COMPILER} -print-file-name=plugin)
        execute_process(COMMAND ${EXECUTE_COMMAND} RESULT_VARIABLE rv OUTPUT_VARIABLE ov)
        # run the command <GCC C compiler> -print-file-name=plugin to determine
        # the location of the GCC Plugin. Strip it out of whitespaces before
        # and after the string to determine if the plugin has been installed or
        # not. If it has been installed, the returned string provides the
        # location of the GCC plugin
        string(STRIP ${ov} new_ov)

        if(new_ov STREQUAL "plugin")
            libatmi_runtime_say("GCC Plugin not found")
            set(PLUGIN_FOUND 0)
        else()
            if(EXISTS "${new_ov}/include/gcc-plugin.h" 
               AND EXISTS "${new_ov}/include/print-tree.h")
                libatmi_runtime_say("GCC Plugin found. Preparing to build ATMI C extensions.")
                include_directories(${new_ov}/include)
                set(PLUGIN_FOUND 1)
            else() 
                libatmi_runtime_say("GCC Plugin (gcc-plugin.h or print-tree.h) not found")
                set(PLUGIN_FOUND 0)
            endif()
        endif()

        if(PLUGIN_FOUND)
            # Enable support for C++11?
            #add_definitions(-std=c++11)
            
            # If building this library in debug mode, we define a macro to enable
            # dumping progress messages at runtime.
            string( TOLOWER "${CMAKE_BUILD_TYPE}" ATMI_CMAKE_BUILD_TYPE)
            if(ATMI_CMAKE_BUILD_TYPE MATCHES debug)
                add_definitions(-DDEBUG)
                add_definitions(-g)
                add_definitions(-O0)
            else()
                add_definitions(-g)
                add_definitions(-O2)
            endif()
            add_definitions(-c)
            add_definitions(-fpic)

            add_library(atmi_cplugin SHARED
                atl_pifgen_plugin.c
                )

            include_directories(${ATMI_DEP_LIBHSA_INCLUDE_DIRS})
            include_directories(${CMAKE_CURRENT_SOURCE_DIR})
            include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
            include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../include)

            # Install plugin under the lib destination folder.
            if(CMAKE_BUILD_TYPE MATCHES Debug)
                install(TARGETS atmi_cplugin LIBRARY DESTINATION "lib-debug" COMPONENT cplugin )
            else()
                install(TARGETS atmi_cplugin LIBRARY DESTINATION "lib" COMPONENT cplugin )
            endif()

            ## CPack component info
            set(CPACK_COMPONENT_CPLUGIN_DISPLAY_NAME "ATMI C Plugin")
            set(CPACK_COMPONENT_CPLUGIN_DEPENDS runtime)


            target_link_libraries(
                atmi_cplugin
                "-Wl,--as-needed"
                -L${ATMI_DEP_LIBHSA_LIBRARIES_DIRS} -lhsa-runtime64 
            )
       else()
            libatmi_runtime_say("Not building ATMI C Extension: GCC Plugin not found.")
       endif()
    else()
        libatmi_runtime_say("Not building ATMI C Extension: only support ATMI in Linux x86_64 or ppc64le hosts.")
    endif()
else()
    libatmi_runtime_say("Not building ATMI C Extension: libhsa-runtime64 not found")
endif()
