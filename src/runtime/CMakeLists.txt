# add path for include files
#include_directories("${PROJECT_SOURCE_DIR}/../include")
#include_directories("${PROJECT_SOURCE_DIR}/runtime/include")

#target_include_directories (atmi PUBLIC ${PROJECT_SOURCE_DIR}/../include)

if(ATMI_DEP_LIBHSA_FOUND)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86_64)|(ppc64le)$" AND CMAKE_SYSTEM_NAME MATCHES "Linux")
        # Enable support for C++11.
        add_definitions(-std=c++11)
        
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

        add_library(atmi_runtime SHARED
            atl_bindthread.c 
            atl_cputask.c    
            atl_profile.c    
            atl_rt.c         
            atl_synckernel.c 
            ATLData.cpp      
            ATLMachine.cpp   
            ATLQueue.cpp
            )

        include_directories(${ATMI_DEP_LIBHSA_INCLUDE_DIRS})
        include_directories(${CMAKE_CURRENT_SOURCE_DIR})
        include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
        include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../include)

        # Install plugin under the lib destination folder.
        if(CMAKE_BUILD_TYPE MATCHES Debug)
          install(TARGETS atmi_runtime LIBRARY DESTINATION "lib-debug")
        else()
          install(TARGETS atmi_runtime LIBRARY DESTINATION "lib")
        endif()

        target_link_libraries(
          atmi_runtime
          "-Wl,--as-needed"
          -L${ATMI_DEP_LIBHSA_LIBRARIES_DIRS} -lhsa-runtime64 
          )
    else()
        libatmi_runtime_say("Not building ATMI Runtime: only support ATMI in Linux x86_64 or ppc64le hosts.")
    endif()
else()
    libatmi_runtime_say("Not building ATMI Runtime: libhsa-runtime64 not found")
endif()
