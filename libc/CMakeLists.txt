# libc CMake build

# Sources
file(GLOB_RECURSE LIBC_C_SOURCES CONFIGURE_DEPENDS
  src/*.c
  src/*.cpp)
list(FILTER LIBC_C_SOURCES EXCLUDE REGEX ".*/_conformity\\.c$")

# GAS assembly (.s/.S)
file(GLOB_RECURSE LIBC_ASM_SOURCES CONFIGURE_DEPENDS
  src/*.s
  src/*.S)

# Exclude CRT sources from main libc libraries; they are built via dedicated rules below
list(FILTER LIBC_C_SOURCES EXCLUDE REGEX ".*/crt/.*")
list(FILTER LIBC_ASM_SOURCES EXCLUDE REGEX ".*/crt/.*")

# Filter architecture-specific sources: exclude i386 when targeting x86_64
if(TARGET_TRIPLE MATCHES "^x86_64")
  list(FILTER LIBC_C_SOURCES EXCLUDE REGEX ".*/i386/.*")
  list(FILTER LIBC_ASM_SOURCES EXCLUDE REGEX ".*/i386/.*")
elseif(TARGET_TRIPLE MATCHES "^(i.86|i386)")
  list(FILTER LIBC_C_SOURCES EXCLUDE REGEX ".*/x86_64/.*")
  list(FILTER LIBC_ASM_SOURCES EXCLUDE REGEX ".*/x86_64/.*")
endif()

add_library(c_static STATIC ${LIBC_C_SOURCES} ${LIBC_ASM_SOURCES})

# Include dirs
set(LIBC_INC ${CMAKE_CURRENT_SOURCE_DIR}/inc)
set(LIBAPI_INC ${CMAKE_CURRENT_SOURCE_DIR}/../libapi/inc)
set(KERNEL_INC ${CMAKE_CURRENT_SOURCE_DIR}/../kernel/inc)
set(MUSL_INC ${CMAKE_CURRENT_SOURCE_DIR}/src/musl)
foreach(tgt c_static)
  target_include_directories(${tgt} PUBLIC ${LIBC_INC} ${LIBAPI_INC} ${KERNEL_INC} ${MUSL_INC})
  target_compile_options(${tgt} PRIVATE -fPIC -Wno-narrowing)
  # Ensure musl features macros (weak_alias etc.) are available in all units
  target_compile_options(${tgt} PRIVATE -include ${MUSL_INC}/features.h)
  set_property(TARGET ${tgt} PROPERTY C_STANDARD 11)
endforeach()

set(LIBC_SHARED ${CMAKE_CURRENT_BINARY_DIR}/libc.so)
add_custom_command(OUTPUT ${LIBC_SHARED}
  COMMAND ${CMAKE_C_COMPILER} --sysroot=${SYSROOT} -shared -shared-libgcc
          -o ${LIBC_SHARED}
          -Wl,--whole-archive $<TARGET_FILE:c_static> -Wl,--no-whole-archive
  DEPENDS c_static
  COMMENT "Linking libc.so from libc.a")
add_custom_target(c_shared ALL DEPENDS ${LIBC_SHARED})

# CRT objects (assembled with GAS)
set(CRT_NAMES crt0 crti crtn)
set(CRT_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/crtobj)
file(MAKE_DIRECTORY ${CRT_OUTPUT_DIR})
set(CRT_OBJECTS)
foreach(crt ${CRT_NAMES})
  set(src ${CMAKE_CURRENT_SOURCE_DIR}/crt/x86_64/${crt}.S)
  set(obj ${CRT_OUTPUT_DIR}/${crt}.o)
  add_custom_command(OUTPUT ${obj}
    COMMAND ${CMAKE_C_COMPILER} -c -x assembler-with-cpp ${src} -o ${obj}
    DEPENDS ${src}
    COMMENT "Assembling ${crt}.S with C preprocessor")
  list(APPEND CRT_OBJECTS ${obj})
endforeach()
add_custom_target(libc-crt ALL DEPENDS ${CRT_OBJECTS})

# Install into Ghost sysroot
add_custom_target(libc-install
  COMMAND ${CMAKE_COMMAND} -E make_directory ${SYSROOT}/system/lib
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:c_static> ${SYSROOT}/system/lib/libc.a
  COMMAND ${CMAKE_COMMAND} -E copy ${LIBC_SHARED} ${SYSROOT}/system/lib/libc.so
  COMMAND ${CMAKE_COMMAND} -E copy ${CRT_OBJECTS} ${SYSROOT}/system/lib/
  # Create empty libm.a archive
  COMMAND ${CMAKE_AR} cr ${SYSROOT}/system/lib/libm.a
  COMMENT "Installing libc to ${SYSROOT}/system/lib" DEPENDS c_static c_shared libc-crt)

# Headers install
add_custom_target(libc-install-headers
  COMMAND ${CMAKE_COMMAND} -E make_directory ${SYSROOT}/system/include
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${LIBC_INC}/ ${SYSROOT}/system/include
  COMMENT "Installing libc headers")
