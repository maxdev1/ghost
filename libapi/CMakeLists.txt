# libapi CMake build

file(GLOB_RECURSE LIBAPI_SOURCES CONFIGURE_DEPENDS
  src/*.c
  src/*.cpp)

add_library(ghostapi_static STATIC ${LIBAPI_SOURCES})
add_library(ghostapi SHARED ${LIBAPI_SOURCES})

# Include dirs
set(LIBAPI_INC ${CMAKE_CURRENT_SOURCE_DIR}/inc)
target_include_directories(ghostapi_static PUBLIC ${LIBAPI_INC})
target_include_directories(ghostapi       PUBLIC ${LIBAPI_INC})

# Flags similar to build.sh
foreach(tgt ghostapi ghostapi_static)
  target_compile_options(${tgt} PRIVATE -fPIC -std=c++11)
  # Avoid default c++17 if parent set
  set_property(TARGET ${tgt} PROPERTY CXX_STANDARD 11)
endforeach()

# Shared link flags
target_link_options(ghostapi PRIVATE -shared -shared-libgcc)

# Install into Ghost sysroot
add_custom_target(libapi-install
  COMMAND ${CMAKE_COMMAND} -E make_directory ${SYSROOT}/system/lib
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:ghostapi_static> ${SYSROOT}/system/lib/libghostapi.a
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:ghostapi>        ${SYSROOT}/system/lib/libghostapi.so
  COMMENT "Installing libapi to ${SYSROOT}/system/lib" DEPENDS ghostapi ghostapi_static)

# Headers install
add_custom_target(libapi-install-headers
  COMMAND ${CMAKE_COMMAND} -E make_directory ${SYSROOT}/system/include
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${LIBAPI_INC}/ ${SYSROOT}/system/include
  COMMENT "Installing libapi headers")
