# Applications CMake integration
# - Builds libraries (lib*) and executables (apps) found under applications/
# - Reads per-app build.sh to pick up ARTIFACT_NAME, CFLAGS_ADD, LDFLAGS when present

cmake_minimum_required(VERSION 3.20)

# Option: build shared variants of application libraries (default OFF to avoid host CRT issues)
option(GHOST_APPLIBS_SHARED "Build shared variants of application libraries" OFF)

# Collect all library include directories to be available for apps during build
file(GLOB _all_inc_dirs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} */inc)
set(APPLICATIONS_ALL_INC_DIRS)
foreach(_inc ${_all_inc_dirs})
  if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${_inc})
    list(APPEND APPLICATIONS_ALL_INC_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/${_inc})
  endif()
endforeach()

# Track library install targets to ensure apps build after headers/libs are installed
set(_LIB_INSTALL_TARGETS)
set(_LIB_HEADER_INSTALL_TARGETS)

function(parse_build_sh in_file out_artifact out_cflags out_ldflags is_library)
  set(_artifact "")
  set(_cflags "")
  set(_ldflags "")
  set(_is_lib FALSE)
  if(EXISTS ${in_file})
    file(READ ${in_file} _content)
    string(REGEX MATCH "ARTIFACT_NAME=\"([^\"]+)\"" _m1 "${_content}")
    if(_m1)
      set(_artifact "${CMAKE_MATCH_1}")
    endif()
    string(REGEX MATCH "CFLAGS_ADD=\"([^\"]*)\"" _m2 "${_content}")
    if(_m2)
      set(_cflags "${CMAKE_MATCH_1}")
    endif()
    string(REGEX MATCH "LDFLAGS=\"([^\"]*)\"" _m3 "${_content}")
    if(_m3)
      set(_ldflags "${CMAKE_MATCH_1}")
    endif()
    # Determine library vs app by inclusion marker
    if(_content MATCHES "libraries.sh")
      set(_is_lib TRUE)
    endif()
  endif()
  set(${out_artifact} "${_artifact}" PARENT_SCOPE)
  set(${out_cflags}   "${_cflags}"   PARENT_SCOPE)
  set(${out_ldflags}  "${_ldflags}"  PARENT_SCOPE)
  set(${is_library}   ${_is_lib}      PARENT_SCOPE)
endfunction()

file(GLOB children RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *)
set(_APP_INSTALL_TARGETS)
foreach(child ${children})
  # Skip libinput build if libproperties is missing
  if(${child} STREQUAL "libinput")
    if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/libproperties/inc/libproperties/properties.hpp)
      message(WARNING "Skipping libinput: missing libproperties headers")
      continue()
    endif()
  endif()
  if(NOT IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${child})
    continue()
  endif()
  if(${child} STREQUAL "." OR ${child} STREQUAL "..")
    continue()
  endif()

  set(dir ${CMAKE_CURRENT_SOURCE_DIR}/${child})
  if(EXISTS ${dir}/src)
    # Parse build.sh for hints
    set(artifact_name "")
    set(extra_cflags "")
    set(extra_ldflags "")
    set(is_lib FALSE)
    parse_build_sh(${dir}/build.sh artifact_name extra_cflags extra_ldflags is_lib)

    if(is_lib)
      # Build shared + static libs
      file(GLOB_RECURSE lib_src CONFIGURE_DEPENDS ${dir}/src/*.c ${dir}/src/*.cpp)
      if(lib_src)
        # Derive base name
        set(base_name ${child})
        # If ARTIFACT_NAME like libfoo.a is provided, prefer that base
        if(artifact_name)
          string(REGEX REPLACE "^lib" "" base_name "${artifact_name}")
          string(REGEX REPLACE "\\.(a|so)$" "" base_name "${base_name}")
        endif()

        add_library(${child}_static STATIC ${lib_src})
        set_target_properties(${child}_static PROPERTIES OUTPUT_NAME ${base_name})

        if(GHOST_APPLIBS_SHARED)
          add_library(${child} SHARED ${lib_src})
          set_target_properties(${child} PROPERTIES OUTPUT_NAME ${base_name})
        endif()

        target_include_directories(${child}_static PUBLIC ${dir}/inc ${SYSROOT}/system/include ${CMAKE_SOURCE_DIR}/libapi/inc ${APPLICATIONS_ALL_INC_DIRS})
        target_compile_options(${child}_static PRIVATE -fPIC -std=c++17)
        if(TARGET ${child})
          target_include_directories(${child} PUBLIC ${dir}/inc ${SYSROOT}/system/include ${CMAKE_SOURCE_DIR}/libapi/inc ${APPLICATIONS_ALL_INC_DIRS})
          target_compile_options(${child} PRIVATE -fPIC -std=c++17)
        endif()

        # Install to sysroot (split into static/shared and aggregate)
        add_custom_target(${child}-install-static
          COMMAND ${CMAKE_COMMAND} -E make_directory ${SYSROOT}/system/lib
          COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${child}_static> ${SYSROOT}/system/lib/lib${base_name}.a
          DEPENDS ${child}_static
          COMMENT "Installing ${child} static to ${SYSROOT}/system/lib")

        if(TARGET ${child})
          add_custom_target(${child}-install-shared
            COMMAND ${CMAKE_COMMAND} -E make_directory ${SYSROOT}/system/lib
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${child}> ${SYSROOT}/system/lib/lib${base_name}.so
            DEPENDS ${child}
            COMMENT "Installing ${child} shared to ${SYSROOT}/system/lib")
          add_custom_target(${child}-install DEPENDS ${child}-install-static ${child}-install-shared)
        else()
          add_custom_target(${child}-install DEPENDS ${child}-install-static)
        endif()

        # Headers install
        if(EXISTS ${dir}/inc)
          add_custom_target(${child}-install-headers
            COMMAND ${CMAKE_COMMAND} -E make_directory ${SYSROOT}/system/include
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${dir}/inc/ ${SYSROOT}/system/include
            COMMENT "Installing ${child} headers")
          list(APPEND _LIB_HEADER_INSTALL_TARGETS ${child}-install-headers)
        endif()
        list(APPEND _LIB_INSTALL_TARGETS ${child}-install)
      endif()
    else()
      # Build application executable
      file(GLOB_RECURSE app_src CONFIGURE_DEPENDS ${dir}/src/*.c ${dir}/src/*.cpp)
      if(app_src)
        set(target_name app_${child})
        add_executable(${target_name} ${app_src})
        target_include_directories(${target_name} PRIVATE ${dir}/src ${SYSROOT}/system/include ${APPLICATIONS_ALL_INC_DIRS} ${CMAKE_SOURCE_DIR}/libapi/inc)
        # Apply extra CFLAGS (e.g., -I${SYSROOT}/system/include/freetype2)
        if(extra_cflags)
          separate_arguments(extra_cflags_list NATIVE_COMMAND "${extra_cflags}")
          target_compile_options(${target_name} PRIVATE ${extra_cflags_list})
        endif()
        # Link flags: ensure sysroot lib path + any LDFLAGS from build.sh
        target_link_options(${target_name} PRIVATE -L${SYSROOT}/system/lib)
        if(extra_ldflags)
          separate_arguments(extra_ldflags_list NATIVE_COMMAND "${extra_ldflags}")
          target_link_options(${target_name} PRIVATE ${extra_ldflags_list})
        endif()
        # Determine artifact install name
        set(artifact_out "${child}.bin")
        if(artifact_name)
          set(artifact_out "${artifact_name}")
        endif()
        # Install
        add_custom_target(${child}-install
          COMMAND ${CMAKE_COMMAND} -E make_directory ${SYSROOT}/applications
          COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${target_name}> ${SYSROOT}/applications/${artifact_out}
          DEPENDS ${target_name}
          COMMENT "Installing application ${child} -> ${SYSROOT}/applications/${artifact_out}")
        # Ensure libraries (and their headers) are installed before building apps
        add_dependencies(${target_name} applications-libs)
        list(APPEND _APP_INSTALL_TARGETS ${child}-install)
      endif()
    endif()
  endif()
endforeach()

# Aggregate target to ensure libraries (and their headers) are installed before building apps
if(NOT TARGET applications-libs)
  add_custom_target(applications-libs)
endif()
if(TARGET applications-libs)
  if(_LIB_INSTALL_TARGETS)
    add_dependencies(applications-libs ${_LIB_INSTALL_TARGETS})
  endif()
  if(_LIB_HEADER_INSTALL_TARGETS)
    add_dependencies(applications-libs ${_LIB_HEADER_INSTALL_TARGETS})
  endif()
  # Also ensure core headers from libapi/libc are installed first if available
  if(TARGET libapi-install-headers)
    add_dependencies(applications-libs libapi-install-headers)
  endif()
  if(TARGET libc-install-headers)
    add_dependencies(applications-libs libc-install-headers)
  endif()
endif()

# Convenience target to install all apps
if(_APP_INSTALL_TARGETS)
  if(NOT TARGET applications-install)
    add_custom_target(applications-install)
  endif()
  add_dependencies(applications-install ${_APP_INSTALL_TARGETS})
endif()
