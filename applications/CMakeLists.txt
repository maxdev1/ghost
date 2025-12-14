# Applications CMake integration
# - Builds libraries (lib*) and executables (apps) found under applications/
# - Legacy build.sh scripts were removed; configuration now lives entirely in CMake.

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

# Applications that should be treated as libraries (static + optional shared)
set(APPLICATION_LIBRARY_TARGETS
  libahci
  libdevice
  libinput
  libpci
  libps2
  libps2driver
  libterminal
  libvideo)

# Extra compile definitions/includes per application executable
set(APP_EXTRA_CFLAGS_desktop   "-I${SYSROOT}/system/include/freetype2")
set(APP_EXTRA_CFLAGS_editor    "-I${SYSROOT}/system/include/freetype2")
set(APP_EXTRA_CFLAGS_js        "-DDUK_F_GHOST")
set(APP_EXTRA_CFLAGS_navigator "-I${SYSROOT}/system/include/freetype2")
set(APP_EXTRA_CFLAGS_terminal  "-I${SYSROOT}/system/include/freetype2")

# Extra link flags per application executable
set(APP_EXTRA_LDFLAGS_ahcidriver  "-lpci")
set(APP_EXTRA_LDFLAGS_calculator  "-lfenster")
set(APP_EXTRA_LDFLAGS_desktop     "-lproperties -lfenster -lcairo -lfreetype -lpixman-1 -lpng -lz")
set(APP_EXTRA_LDFLAGS_devicemanager "-lpci")
set(APP_EXTRA_LDFLAGS_editor      "-lproperties -linput -lfenster -lcairo -lfreetype -lpixman-1 -lpng -lz")
set(APP_EXTRA_LDFLAGS_efifbdriver "-ldevice")
set(APP_EXTRA_LDFLAGS_gsh         "-lterminal")
set(APP_EXTRA_LDFLAGS_navigator   "-lproperties -linput -lfenster -lcairo -lfreetype -lpixman-1 -lpng -lz")
set(APP_EXTRA_LDFLAGS_proc        "-lterminal")
set(APP_EXTRA_LDFLAGS_ps2driver   "-lps2")
set(APP_EXTRA_LDFLAGS_terminal    "-lfenster -lterminal -lps2driver -linput -lproperties -lcairo -lfreetype -lpixman-1 -lpng -lz")
set(APP_EXTRA_LDFLAGS_testprogram "-lps2driver -lfenster")
set(APP_EXTRA_LDFLAGS_vmsvgadriver "-lpci -ldevice")

# Artifact overrides (default is "<app>.bin")
set(APP_ARTIFACT_testprogram "tester.bin")

function(get_app_metadata name out_artifact out_cflags out_ldflags)
  set(_artifact "")
  if(DEFINED APP_ARTIFACT_${name})
    set(_artifact "${APP_ARTIFACT_${name}}")
  endif()
  set(_cflags "")
  if(DEFINED APP_EXTRA_CFLAGS_${name})
    set(_cflags "${APP_EXTRA_CFLAGS_${name}}")
  endif()
  set(_ldflags "")
  if(DEFINED APP_EXTRA_LDFLAGS_${name})
    set(_ldflags "${APP_EXTRA_LDFLAGS_${name}}")
  endif()
  set(${out_artifact} "${_artifact}" PARENT_SCOPE)
  set(${out_cflags}   "${_cflags}"   PARENT_SCOPE)
  set(${out_ldflags}  "${_ldflags}"  PARENT_SCOPE)
endfunction()

file(GLOB children RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *)

set(FENSTER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/fenster)
if(EXISTS ${FENSTER_DIR}/Makefile)
  if(IS_DIRECTORY ${FENSTER_DIR}/libfenster/inc)
    list(APPEND APPLICATIONS_ALL_INC_DIRS ${FENSTER_DIR}/libfenster/inc)
  endif()
  if(IS_DIRECTORY ${FENSTER_DIR}/libproperties/inc)
    list(APPEND APPLICATIONS_ALL_INC_DIRS ${FENSTER_DIR}/libproperties/inc)
  endif()
  set(_fenster_env
    "target=ghost"
    "SYSROOT=${SYSROOT}"
    "CXX=${CMAKE_CXX_COMPILER}"
    "AR=${CMAKE_AR}"
    "CXXFLAGS=--sysroot=${SYSROOT} -I${SYSROOT}/system/include/freetype2"
    "LDFLAGS=--sysroot=${SYSROOT} -L${SYSROOT}/system/lib -Wl,-rpath-link,${SYSROOT}/system/lib"
  )
  add_custom_target(fenster
    COMMAND ${CMAKE_COMMAND} -E env ${_fenster_env} make
    WORKING_DIRECTORY ${FENSTER_DIR}
    USES_TERMINAL
    COMMENT "Building fenster (libproperties/libfenster/server)")
  add_custom_target(fenster-install-headers
    COMMAND ${CMAKE_COMMAND} -E env ${_fenster_env} make install-headers
    WORKING_DIRECTORY ${FENSTER_DIR}
    USES_TERMINAL
    COMMENT "Installing fenster headers")
  add_dependencies(fenster fenster-install-headers)
  if(TARGET ports)
    add_dependencies(fenster ports)
  endif()
endif()
set(_APP_INSTALL_TARGETS)
foreach(child ${children})
  if(child STREQUAL "core")
    # core utilities are handled separately (multiple sub-apps)
    continue()
  endif()
  # Skip libinput build if libproperties is missing
  if(child STREQUAL "libinput")
    set(_libprops_dir ${CMAKE_CURRENT_SOURCE_DIR}/libproperties/inc/libproperties/properties.hpp)
    set(_fenster_libprops_dir ${CMAKE_CURRENT_SOURCE_DIR}/fenster/libproperties/inc/libproperties/properties.hpp)
    if(NOT EXISTS ${_libprops_dir} AND NOT EXISTS ${_fenster_libprops_dir})
      message(WARNING "Skipping libinput: missing libproperties headers")
      continue()
    endif()
  endif()
  if(NOT IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${child})
    continue()
  endif()
  if(child STREQUAL "." OR child STREQUAL "..")
    continue()
  endif()

  set(dir ${CMAKE_CURRENT_SOURCE_DIR}/${child})
  if(EXISTS ${dir}/src)
    # Determine whether this entry is a library or executable
    list(FIND APPLICATION_LIBRARY_TARGETS ${child} _lib_index)
    set(is_lib FALSE)
    if(NOT _lib_index EQUAL -1)
      set(is_lib TRUE)
    endif()

    set(artifact_name "")
    set(extra_cflags "")
    set(extra_ldflags "")
    get_app_metadata(${child} artifact_name extra_cflags extra_ldflags)

    if(is_lib)
      file(GLOB_RECURSE lib_src CONFIGURE_DEPENDS ${dir}/src/*.c ${dir}/src/*.cpp)
      if(lib_src)
        # Determine base output name (strip leading lib / extension so we don't end up with liblibfoo.a)
        set(base_name ${child})
        if(artifact_name)
          string(REGEX REPLACE "^lib" "" base_name "${artifact_name}")
          string(REGEX REPLACE "\\.(a|so)$" "" base_name "${base_name}")
        else()
          string(REGEX REPLACE "^lib" "" base_name "${base_name}")
        endif()
        if(base_name STREQUAL "")
          set(base_name ${child})
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
      file(GLOB_RECURSE app_src CONFIGURE_DEPENDS ${dir}/src/*.c ${dir}/src/*.cpp)
      if(app_src)
        set(target_name app_${child})
        add_executable(${target_name} ${app_src})
        target_include_directories(${target_name} PRIVATE ${dir}/src ${SYSROOT}/system/include ${APPLICATIONS_ALL_INC_DIRS} ${CMAKE_SOURCE_DIR}/libapi/inc)

        if(extra_cflags)
          separate_arguments(extra_cflags_list NATIVE_COMMAND "${extra_cflags}")
          target_compile_options(${target_name} PRIVATE ${extra_cflags_list})
        endif()

        target_link_options(${target_name} PRIVATE -L${SYSROOT}/system/lib -Wl,-rpath-link,${SYSROOT}/system/lib)
        if(extra_ldflags)
          separate_arguments(extra_ldflags_list NATIVE_COMMAND "${extra_ldflags}")
          set(_ld_opts)
          set(_ld_libs)
          foreach(token ${extra_ldflags_list})
            if(token MATCHES "^-l.+")
              list(APPEND _ld_libs ${token})
            else()
              list(APPEND _ld_opts ${token})
            endif()
          endforeach()
          if(_ld_opts)
            target_link_options(${target_name} PRIVATE ${_ld_opts})
          endif()
          if(_ld_libs)
            target_link_libraries(${target_name} PRIVATE ${_ld_libs})
          endif()
        endif()

        set(artifact_out "${child}.bin")
        if(artifact_name)
          set(artifact_out "${artifact_name}")
        endif()
        add_custom_target(${child}-install
          COMMAND ${CMAKE_COMMAND} -E make_directory ${SYSROOT}/applications
          COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${target_name}> ${SYSROOT}/applications/${artifact_out}
          DEPENDS ${target_name}
          COMMENT "Installing application ${child} -> ${SYSROOT}/applications/${artifact_out}")

        add_dependencies(${target_name} applications-libs)
        list(APPEND _APP_INSTALL_TARGETS ${child}-install)
      endif()
    endif()
  endif()
endforeach()

if(TARGET fenster)
  if(TARGET libinput-install)
    add_dependencies(fenster libinput-install)
  endif()
  if(TARGET libinput-install-headers)
    add_dependencies(fenster libinput-install-headers)
  endif()
  foreach(_dep libvideo libps2driver)
    if(TARGET ${_dep}-install)
      add_dependencies(fenster ${_dep}-install)
    endif()
    if(TARGET ${_dep}-install-headers)
      add_dependencies(fenster ${_dep}-install-headers)
    endif()
  endforeach()
endif()

# Core shell utilities (applications/core) - each subdirectory src-<name> builds a dedicated binary
set(_CORE_APPS echo lines ls read reverse write)
foreach(core_app ${_CORE_APPS})
  set(core_src_dir ${CMAKE_CURRENT_SOURCE_DIR}/core/src-${core_app})
  if(EXISTS ${core_src_dir})
    file(GLOB_RECURSE core_src CONFIGURE_DEPENDS ${core_src_dir}/*.c ${core_src_dir}/*.cpp)
    if(core_src)
      set(target_name app_core_${core_app})
      add_executable(${target_name} ${core_src})
      target_include_directories(${target_name} PRIVATE ${core_src_dir} ${SYSROOT}/system/include ${APPLICATIONS_ALL_INC_DIRS} ${CMAKE_SOURCE_DIR}/libapi/inc)
      target_link_options(${target_name} PRIVATE -L${SYSROOT}/system/lib -Wl,-rpath-link,${SYSROOT}/system/lib)

      set(core_artifact "${core_app}.bin")
      add_custom_target(core-${core_app}-install
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SYSROOT}/applications
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${target_name}> ${SYSROOT}/applications/${core_artifact}
        DEPENDS ${target_name}
        COMMENT "Installing core utility ${core_app} -> ${SYSROOT}/applications/${core_artifact}")

      add_dependencies(${target_name} applications-libs)
      list(APPEND _APP_INSTALL_TARGETS core-${core_app}-install)
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
  if(TARGET fenster)
    add_dependencies(applications-libs fenster)
  endif()
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
