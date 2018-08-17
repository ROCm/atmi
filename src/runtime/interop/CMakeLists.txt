# ===--------------------------------------------------------------------------
#               ATMI (Asynchronous Task and Memory Interface)
#
#  This file is distributed under the MIT License. See LICENSE.txt for details.
# ===--------------------------------------------------------------------------

# compile HSA interop layer if specified explicitly
string( TOLOWER "${ATMI_HSA_INTEROP}" ATMI_HSA_INTEROP_VAR )
if(ATMI_HSA_INTEROP_VAR MATCHES on )
    add_subdirectory(hsa) 
endif()
