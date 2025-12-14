# CMake integration for target utilities (e.g., Limine)

cmake_minimum_required(VERSION 3.20)

set(LIMINE_VERSION "${LIMINE_VERSION}" CACHE STRING "Limine version")
if(NOT LIMINE_VERSION)
  set(LIMINE_VERSION 9.2.0)
endif()
set(LIMINE_SOURCE "https://ghostkernel.org/repository/limine/limine-${LIMINE_VERSION}.tar.gz")

# Where to place limine directory (inside repo's target dir)
set(LIMINE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/limine-${LIMINE_VERSION})
set(LIMINE_TAR ${CMAKE_CURRENT_BINARY_DIR}/limine-${LIMINE_VERSION}.tar.gz)
set(LIMINE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(DOWNLOAD_LIMINE_SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/download-limine.sh)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/download-limine.sh.in ${DOWNLOAD_LIMINE_SCRIPT} @ONLY)

# Build limine only if the directory is not present
add_custom_target(limine
  COMMAND ${CMAKE_COMMAND} -E echo "Preparing Limine ${LIMINE_VERSION}"
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash ${DOWNLOAD_LIMINE_SCRIPT}
  USES_TERMINAL
  COMMENT "Download and build Limine (${LIMINE_VERSION})")
