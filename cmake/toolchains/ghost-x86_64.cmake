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
set(CROSS_GCC   ${TARGET_TRIPLE}-gcc)
set(CROSS_GXX   ${TARGET_TRIPLE}-g++)
set(CROSS_AS    ${TARGET_TRIPLE}-as)
set(CROSS_AR    ${TARGET_TRIPLE}-ar)
set(CROSS_RANLIB ${TARGET_TRIPLE}-ranlib)
set(CROSS_LD    ${TARGET_TRIPLE}-ld)

# Detect availability early and emit a helpful message if missing
find_program(_FOUND_GCC NAMES ${CROSS_GCC})
if(NOT _FOUND_GCC)
  message(FATAL_ERROR "Cross compiler '${CROSS_GCC}' not found.\n"
    "Bootstrap the toolchain first using:\n"
    "  cmake -S cmake/ghost-toolchain-bootstrap -B build-ghost-toolchain \\\n      -DTARGET_TRIPLE=${TARGET_TRIPLE} \\\n      -DTOOLCHAIN_BASE=$PWD/build-ghost/toolchain \\\n      -DSYSROOT=$PWD/build-ghost/sysroot\n"
    "  cmake --build build-ghost-toolchain --target ghost-toolchain\n"
    "Then re-run your original CMake configure command.")
endif()

set(CMAKE_C_COMPILER   ${CROSS_GCC})
set(CMAKE_CXX_COMPILER ${CROSS_GXX})
set(CMAKE_ASM_COMPILER ${CROSS_AS})
set(CMAKE_AR           ${CROSS_AR} CACHE FILEPATH "Archiver")
set(CMAKE_RANLIB       ${CROSS_RANLIB} CACHE FILEPATH "Ranlib")
set(CMAKE_LINKER       ${CROSS_LD} CACHE FILEPATH "Linker")

# Sysroot
set(SYSROOT "${SYSROOT}" CACHE PATH "Ghost sysroot path")
if(NOT SYSROOT)
  # Default to a sysroot dir inside the build tree to avoid writing to '/'
  set(SYSROOT ${CMAKE_BINARY_DIR}/sysroot CACHE PATH "Ghost sysroot path" FORCE)
endif()

# Important: tell CMake this is a freestanding cross environment
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Use system includes/lib from the sysroot only
set(CMAKE_SYSROOT ${SYSROOT})

# Use NASM for .asm where needed
set(CMAKE_ASM_NASM_COMPILER nasm)
