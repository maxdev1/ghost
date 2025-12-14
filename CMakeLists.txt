cmake_minimum_required(VERSION 3.20)
project(GhostOS LANGUAGES C CXX ASM ASM_NASM)

# Options
option(GHOST_BUILD_LIBAPI "Build libapi" ON)
option(GHOST_BUILD_LIBC   "Build libc" ON)
option(GHOST_BUILD_KERNEL "Build kernel" ON)
option(GHOST_BUILD_APPS   "Build applications via legacy scripts" ON)
option(GHOST_BUILD_PORTS  "Build ports (zlib, pixman, libpng, freetype, cairo)" OFF)
option(GHOST_ENABLE_PACK  "Enable pack target to produce ISO via scripts" ON)

# Defaults similar to ghost.sh
set(TARGET_TRIPLE "${TARGET_TRIPLE}" CACHE STRING "Target triple (e.g. x86_64-ghost)")
if(NOT TARGET_TRIPLE)
  set(TARGET_TRIPLE x86_64-ghost)
endif()
set(TOOLCHAIN_BASE "${TOOLCHAIN_BASE}" CACHE PATH "Ghost toolchain base path")
if(NOT TOOLCHAIN_BASE)
  # Use a local path to avoid writing to '/'
  set(TOOLCHAIN_BASE ${CMAKE_BINARY_DIR}/toolchain)
endif()
set(SYSROOT "${SYSROOT}" CACHE PATH "Ghost sysroot path")
if(NOT SYSROOT)
  # Default sysroot inside build tree (safe, non-root)
  set(SYSROOT ${CMAKE_BINARY_DIR}/sysroot)
endif()

# Common compile settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

# Handy function for header installation into sysroot
function(ghost_install_headers hdr_dir)
  add_custom_target(install-headers-${hdr_dir}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SYSROOT}/system/include
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${hdr_dir} ${SYSROOT}/system/include
    BYPRODUCTS ${SYSROOT}/system/include
    COMMENT "Installing headers from ${hdr_dir} to ${SYSROOT}/system/include")
endfunction()

# Subdirectories
if(GHOST_BUILD_LIBAPI)
  add_subdirectory(libapi)
endif()
if(GHOST_BUILD_LIBC)
  add_subdirectory(libc)
endif()
if(GHOST_BUILD_KERNEL)
  add_subdirectory(kernel)
endif()

# Applications via CMake (automatic conversion)
if(GHOST_BUILD_APPS)
  add_subdirectory(applications)
endif()

# Ports wrapper (optional, disabled by default). We keep this as a script-driven
# step but fully managed by CMake and safe env paths.
if(GHOST_BUILD_PORTS)
  add_custom_target(ports ALL
    COMMAND ${CMAKE_COMMAND} -E env SYSROOT=${SYSROOT} TOOLCHAIN_BASE=${TOOLCHAIN_BASE} bash -c "\
      cd ${CMAKE_SOURCE_DIR}/patches/ports && \
      bash port.sh zlib/1.2.8 && \
      bash port.sh pixman/0.38.0 && \
      bash port.sh libpng/1.6.34 && \
      bash port.sh freetype/2.5.3 && \
      bash port.sh cairo/1.12.18"
    USES_TERMINAL
    COMMENT "Building ports using existing scripts (env SYSROOT, TOOLCHAIN_BASE overridden)")
endif()

# Target utilities (limine)
add_subdirectory(target)

# Pack/ISO wrapper
if(GHOST_ENABLE_PACK)
  add_custom_target(pack
    COMMAND ${CMAKE_COMMAND} -E env SH=bash SYSROOT=${SYSROOT} TOOLCHAIN_BASE=${TOOLCHAIN_BASE} KERNEL_BIN=$<TARGET_FILE:kernel> bash ${CMAKE_SOURCE_DIR}/target/build.sh pack
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/target
    USES_TERMINAL
    COMMENT "Packing ISO via target/build.sh (env SYSROOT, TOOLCHAIN_BASE, KERNEL_BIN overridden)")
  # Ensure kernel and libs are built before pack if enabled
  if(TARGET kernel)
    add_dependencies(pack kernel)
  endif()
  if(TARGET libc-install)
    add_dependencies(pack libc-install)
  endif()
  if(TARGET libapi-install)
    add_dependencies(pack libapi-install)
  endif()
  if(TARGET kernel-apstartup-install)
    add_dependencies(pack kernel-apstartup-install)
  endif()
  add_dependencies(pack limine)
endif()

message(STATUS "Target triple: ${TARGET_TRIPLE}")
message(STATUS "Sysroot: ${SYSROOT}")
