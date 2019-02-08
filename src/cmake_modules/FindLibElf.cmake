# ===--------------------------------------------------------------------------
#               ATMI (Asynchronous Task and Memory Interface)
#
#  This file is distributed under the MIT License. See LICENSE.txt for details.
# ===--------------------------------------------------------------------------

# Below are the variables that will be set at the end of this file
#  LIBELF_FOUND 
#  LIBELF_INCLUDE_DIRS
#  LIBELF_LIBRARIES


find_path (LIBELF_INCLUDE_DIRS
    NAMES
    libelf.h
    PATHS
    /usr/include
    /usr/local/include
    ENV CPATH)

find_library (LIBELF_LIBRARIES
    NAMES
    elf
    PATHS
    /usr/lib/x86_64-linux-gnu
    /usr/lib
    /usr/local/lib
    ENV LIBRARY_PATH
    ENV LD_LIBRARY_PATH)

# set LIBELF_FOUND to TRUE if the below variables are true, 
# i.e. header and lib files are found
include (FindPackageHandleStandardArgs)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(LibElf DEFAULT_MSG
      LIBELF_LIBRARIES
      LIBELF_INCLUDE_DIRS)

mark_as_advanced(LIBELF_INCLUDE_DIRS LIBELF_LIBRARIES)
