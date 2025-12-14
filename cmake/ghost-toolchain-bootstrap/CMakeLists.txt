# Ghost GCC cross toolchain bootstrap project (no compilers required)
# Purpose: builds the Ghost cross toolchain using the repository's toolchain.sh
# Usage:
#   cmake -S cmake/ghost-toolchain-bootstrap -B build-ghost-toolchain \
#     -DTARGET_TRIPLE=x86_64-ghost \
#     -DTOOLCHAIN_BASE=$PWD/build-ghost/toolchain \
#     -DSYSROOT=$PWD/build-ghost/sysroot
#   cmake --build build-ghost-toolchain --target ghost-toolchain

cmake_minimum_required(VERSION 3.20)
project(GhostToolchain NONE)

# User options
set(TARGET_TRIPLE "${TARGET_TRIPLE}" CACHE STRING "Target triple (e.g. x86_64-ghost)")
set(TOOLCHAIN_BASE "${TOOLCHAIN_BASE}" CACHE PATH "Toolchain installation directory")
set(SYSROOT "${SYSROOT}" CACHE PATH "Sysroot output directory")

# Defaults to safe local paths
if(NOT TARGET_TRIPLE)
  set(TARGET_TRIPLE x86_64-ghost)
endif()
if(NOT TOOLCHAIN_BASE)
  set(TOOLCHAIN_BASE ${CMAKE_BINARY_DIR}/toolchain)
endif()
if(NOT SYSROOT)
  set(SYSROOT ${CMAKE_BINARY_DIR}/sysroot)
endif()

# Paths
set(REPO_ROOT ${CMAKE_SOURCE_DIR}/../..)
set(TOOLCHAIN_SH ${REPO_ROOT}/toolchain.sh)

set(AUTOCONF_269_BIN ${REPO_ROOT}/build-deps/autoconf-2.69/bin)
if(EXISTS ${AUTOCONF_269_BIN})
  set(_toolchain_path "${AUTOCONF_269_BIN}:$ENV{PATH}")
else()
  set(_toolchain_path "$ENV{PATH}")
endif()

add_custom_target(ghost-toolchain
  COMMAND ${CMAKE_COMMAND} -E env
          TARGET=${TARGET_TRIPLE}
          TOOLCHAIN_BASE=${TOOLCHAIN_BASE}
          SYSROOT=${SYSROOT}
          PATH=${_toolchain_path}
          bash ${TOOLCHAIN_SH}
  WORKING_DIRECTORY ${REPO_ROOT}
  USES_TERMINAL
  COMMENT "Building Ghost cross toolchain into ${TOOLCHAIN_BASE} with sysroot ${SYSROOT}")
