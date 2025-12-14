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

# Build limine only if the directory is not present
add_custom_target(limine
  COMMAND ${CMAKE_COMMAND} -E echo "Preparing Limine ${LIMINE_VERSION}"
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash -c "if [ ! -d ${LIMINE_DIR} ]; then \
    echo 'Downloading limine...'; \
    curl -k -L ${LIMINE_SOURCE} -o ${LIMINE_TAR} && \
    ${CMAKE_COMMAND} -E tar xzf ${LIMINE_TAR} -C ${CMAKE_CURRENT_SOURCE_DIR} && \
    (cd ${LIMINE_DIR} && ./configure --enable-bios-cd --enable-uefi-cd && make); \
  else \
    echo 'Limine already prepared at ${LIMINE_DIR}'; \
  fi"
  USES_TERMINAL
  COMMENT "Download and build Limine (${LIMINE_VERSION})")
