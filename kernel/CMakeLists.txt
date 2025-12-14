# kernel CMake build

# Sources
file(GLOB_RECURSE KERNEL_CPP CONFIGURE_DEPENDS src/**/*.cpp src/*.cpp)
file(GLOB_RECURSE KERNEL_C   CONFIGURE_DEPENDS src/**/*.c   src/*.c)
file(GLOB_RECURSE KERNEL_ASM CONFIGURE_DEPENDS src/**/*.asm src/*.asm)
# Exclude flat-binary AP startup from ELF64 NASM objects; it's built separately below
list(FILTER KERNEL_ASM EXCLUDE REGEX ".*/ap/ap_startup\.asm$")

set(KERNEL_SOURCES ${KERNEL_CPP} ${KERNEL_C})

# Build nasm-based .asm to elf64 object files
set(NASM_OBJECTS)
foreach(asm ${KERNEL_ASM})
  get_filename_component(name ${asm} NAME)
  string(REPLACE "." "_" name_sanitized ${name})
  set(obj ${CMAKE_CURRENT_BINARY_DIR}/nasm_${name_sanitized}.o)
  add_custom_command(OUTPUT ${obj}
    COMMAND ${CMAKE_ASM_NASM_COMPILER} -f elf64 -s ${asm} -o ${obj}
    DEPENDS ${asm}
    COMMENT "Assembling ${asm} (nasm)")
  list(APPEND NASM_OBJECTS ${obj})
endforeach()

add_executable(kernel ${KERNEL_SOURCES} ${NASM_OBJECTS})

# Includes
target_include_directories(kernel PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/kernel/include
  ${CMAKE_SOURCE_DIR}/libapi/inc
  ${SYSROOT}/system/include
)

# Option: suppress configure-time Limine warning
option(GHOST_SUPPRESS_LIMINE_WARNING "Do not warn when Limine headers are not found during configure" OFF)

# Limine: provide path to limine.h prepared under target/limine-<version>
file(GLOB LIMINE_HEADER_ROOT ${CMAKE_SOURCE_DIR}/target/limine-*/limine.h)
file(GLOB LIMINE_HEADER_PROTO ${CMAKE_SOURCE_DIR}/target/limine-*/common/protos/limine.h)
set(LIMINE_HEADER)
if(LIMINE_HEADER_ROOT)
  list(GET LIMINE_HEADER_ROOT 0 LIMINE_HEADER)
elseif(LIMINE_HEADER_PROTO)
  list(GET LIMINE_HEADER_PROTO 0 LIMINE_HEADER)
endif()
if(LIMINE_HEADER)
  get_filename_component(LIMINE_DIR ${LIMINE_HEADER} DIRECTORY)
  target_include_directories(kernel PRIVATE ${LIMINE_DIR})
endif()
if(TARGET limine)
  add_dependencies(kernel limine)
endif()

# Compile flags
target_compile_options(kernel PRIVATE
  -mcmodel=large -mno-sse -mno-sse2 -mno-mmx -mno-avx -mno-red-zone
  -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector
  -fno-pic -fno-PIE
  -Wall -Wno-unused-but-set-variable
  -D_GHOST_KERNEL_=1
)
set_property(TARGET kernel PROPERTY CXX_STANDARD 11)

# Linker script and flags
set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/extra/link-kernel.ld)
target_link_options(kernel PRIVATE -nostdlib -nostartfiles -T ${LINKER_SCRIPT} -no-pie)

# AP startup flat binary (copied to sysroot)
set(AP_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/ap/ap_startup.asm)
set(AP_OBJ ${CMAKE_CURRENT_BINARY_DIR}/apstartup.o)
add_custom_command(OUTPUT ${AP_OBJ}
  COMMAND ${CMAKE_ASM_NASM_COMPILER} -f bin -o ${AP_OBJ} -s ${AP_SRC}
  DEPENDS ${AP_SRC}
  COMMENT "Building AP startup object (flat bin)")
add_custom_target(kernel-apstartup ALL DEPENDS ${AP_OBJ})
add_custom_target(kernel-apstartup-install
  COMMAND ${CMAKE_COMMAND} -E make_directory ${SYSROOT}/system/lib
  COMMAND ${CMAKE_COMMAND} -E copy ${AP_OBJ} ${SYSROOT}/system/lib/apstartup.o
  DEPENDS kernel-apstartup
  COMMENT "Installing AP startup object into sysroot")
