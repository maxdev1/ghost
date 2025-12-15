# libapi CMake build

file(GLOB_RECURSE LIBAPI_SOURCES CONFIGURE_DEPENDS
  src/*.c
  src/*.cpp)

add_library(ghostapi_static STATIC ${LIBAPI_SOURCES})

# Include dirs
set(LIBAPI_INC ${CMAKE_CURRENT_SOURCE_DIR}/inc)
target_include_directories(ghostapi_static PUBLIC ${LIBAPI_INC})

# Flags similar to build.sh
target_compile_options(ghostapi_static PRIVATE -fPIC -std=c++11)
set_property(TARGET ghostapi_static PROPERTY CXX_STANDARD 11)

set(GHOSTAPI_SHARED ${CMAKE_CURRENT_BINARY_DIR}/libghostapi.so)
add_custom_command(OUTPUT ${GHOSTAPI_SHARED}
  COMMAND ${CMAKE_CXX_COMPILER} --sysroot=${SYSROOT} -shared -shared-libgcc
          -o ${GHOSTAPI_SHARED}
          -Wl,--whole-archive $<TARGET_FILE:ghostapi_static> -Wl,--no-whole-archive
  DEPENDS ghostapi_static
  COMMENT "Linking libghostapi.so from libghostapi.a")
add_custom_target(ghostapi ALL DEPENDS ${GHOSTAPI_SHARED})

# Install into Ghost sysroot
add_custom_target(libapi-install
  COMMAND ${CMAKE_COMMAND} -E make_directory ${SYSROOT}/system/lib
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:ghostapi_static> ${SYSROOT}/system/lib/libghostapi.a
  COMMAND ${CMAKE_COMMAND} -E copy ${GHOSTAPI_SHARED}             ${SYSROOT}/system/lib/libghostapi.so
  COMMENT "Installing libapi to ${SYSROOT}/system/lib" DEPENDS ghostapi ghostapi_static)

# Headers install
add_custom_target(libapi-install-headers
  COMMAND ${CMAKE_COMMAND} -E make_directory ${SYSROOT}/system/include
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${LIBAPI_INC}/ ${SYSROOT}/system/include
  COMMENT "Installing libapi headers")
