# Ghost OS cross-compilation toolchain file for CMake
# Usage:
#   cmake -S . -B build-ghost -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/ghost-x86_64.cmake \
#         -DTARGET_TRIPLE=x86_64-ghost -DTOOLCHAIN_BASE=$PWD/build-ghost/toolchain -DSYSROOT=$PWD/build-ghost/sysroot

cmake_minimum_required(VERSION 3.20)

# Allow user overrides
set(TARGET_TRIPLE "${TARGET_TRIPLE}" CACHE STRING "Target triple (e.g. x86_64-ghost)")
set(TOOLCHAIN_BASE "${TOOLCHAIN_BASE}" CACHE PATH "Ghost toolchain base path")

# Defaults focused on local (non-root) paths
if(NOT TARGET_TRIPLE)
  set(TARGET_TRIPLE x86_64-ghost CACHE STRING "Target triple" FORCE)
endif()
if(NOT TOOLCHAIN_BASE)
  # Default to a toolchain dir inside the build tree to avoid writing to '/'
  set(TOOLCHAIN_BASE ${CMAKE_BINARY_DIR}/toolchain CACHE PATH "Ghost toolchain base path" FORCE)
endif()

# Cross tools
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Compilers and binutils
set(_TOOLCHAIN_BIN ${TOOLCHAIN_BASE}/bin)
list(PREPEND CMAKE_PROGRAM_PATH ${_TOOLCHAIN_BIN})
set(CROSS_GCC   ${TARGET_TRIPLE}-gcc)
set(CROSS_GXX   ${TARGET_TRIPLE}-g++)
set(CROSS_AS    ${TARGET_TRIPLE}-as)
set(CROSS_AR    ${TARGET_TRIPLE}-ar)
set(CROSS_RANLIB ${TARGET_TRIPLE}-ranlib)
set(CROSS_LD    ${TARGET_TRIPLE}-ld)

function(_ghost_find_tool out_var name)
  set(_cache_var "_ghost_tool_${out_var}")
  find_program(${_cache_var} NAMES ${name})
  if(NOT ${_cache_var})
    message(FATAL_ERROR "Cross tool '${name}' not found (looked in ${_TOOLCHAIN_BIN}).\n"
      "Bootstrap the toolchain first using:\n"
      "  cmake -S cmake/ghost-toolchain-bootstrap -B build-ghost-toolchain \\\n      -DTARGET_TRIPLE=${TARGET_TRIPLE} \\\n      -DTOOLCHAIN_BASE=$PWD/build-ghost/toolchain \\\n      -DSYSROOT=$PWD/build-ghost/sysroot\n"
      "  cmake --build build-ghost-toolchain --target ghost-toolchain\n"
      "Then re-run your original CMake configure command.")
  endif()
  set(${out_var} ${${_cache_var}} PARENT_SCOPE)
endfunction()

_ghost_find_tool(CROSS_GCC_PATH   ${CROSS_GCC})
_ghost_find_tool(CROSS_GXX_PATH   ${CROSS_GXX})
_ghost_find_tool(CROSS_AS_PATH    ${CROSS_AS})
_ghost_find_tool(CROSS_AR_PATH    ${CROSS_AR})
_ghost_find_tool(CROSS_RANLIB_PATH ${CROSS_RANLIB})
_ghost_find_tool(CROSS_LD_PATH    ${CROSS_LD})

set(CMAKE_C_COMPILER   ${CROSS_GCC_PATH})
set(CMAKE_CXX_COMPILER ${CROSS_GXX_PATH})
set(CMAKE_ASM_COMPILER ${CROSS_GCC_PATH})
set(CMAKE_AR           ${CROSS_AR_PATH} CACHE FILEPATH "Archiver" FORCE)
set(CMAKE_RANLIB       ${CROSS_RANLIB_PATH} CACHE FILEPATH "Ranlib" FORCE)
set(CMAKE_LINKER       ${CROSS_LD_PATH} CACHE FILEPATH "Linker" FORCE)
set(CMAKE_C_STANDARD_LIBRARIES "-static-libgcc")
set(CMAKE_CXX_STANDARD_LIBRARIES "-static-libgcc")

# Shared library support (CMake does not provide defaults for Generic toolchains)
set(CMAKE_SHARED_LIBRARY_PREFIX "lib")
set(CMAKE_SHARED_LIBRARY_SUFFIX ".so")
set(CMAKE_SHARED_LIBRARY_SUPPORTED TRUE)
set_property(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS TRUE)
set(CMAKE_SHARED_LIBRARY_RUNTIME_C_FLAG "")
set(CMAKE_SHARED_LIBRARY_RUNTIME_CXX_FLAG "")
set(CMAKE_SHARED_LIBRARY_C_FLAGS "-fPIC")
set(CMAKE_SHARED_LIBRARY_CXX_FLAGS "-fPIC")
set(CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS "-shared")
set(CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS "-shared")
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")
set(CMAKE_CXX_CREATE_SHARED_LIBRARY "<CMAKE_CXX_COMPILER> <CMAKE_SHARED_LIBRARY_CXX_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")

# Sysroot
set(SYSROOT "${SYSROOT}" CACHE PATH "Ghost sysroot path")
if(NOT SYSROOT)
  # Default to a sysroot dir inside the build tree to avoid writing to '/'
  set(SYSROOT ${CMAKE_BINARY_DIR}/sysroot CACHE PATH "Ghost sysroot path" FORCE)
endif()
set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES TARGET_TRIPLE TOOLCHAIN_BASE SYSROOT)

# Important: tell CMake this is a freestanding cross environment
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Use system includes/lib from the sysroot only
set(CMAKE_SYSROOT ${SYSROOT})

# Use NASM for .asm where needed
set(CMAKE_ASM_NASM_COMPILER nasm)
