# pkgconf-generate-pc.cmake
#
# Generate a pkg-config file for a CMake target.
#
# pkgconf-config.cmake includes this file. It may also be included directly;
# generating a file does not require the pkgconf executable.
#
#   pkgconf_generate_pc(<target>
#                       [NAME <name>] [DESCRIPTION <text>] [VERSION <ver>]
#                       [URL <url>] [LICENSE <spdx>] [MAINTAINER <text>]
#                       [COPYRIGHT <text>]
#                       [REQUIRES <spec>...] [REQUIRES_PRIVATE <spec>...]
#                       [CONFLICTS <spec>...] [PROVIDES <spec>...]
#                       [CFLAGS <flag>...] [CFLAGS_PRIVATE <flag>...]
#                       [LIBS <flag>...] [LIBS_PRIVATE <flag>...]
#                       [VARIABLES <name=value>...]
#                       [PREFIX <dir>] [LIBDIR <dir>] [INCLUDEDIR <dir>]
#                       [RELOCATABLE]
#                       [NO_INFER_REQUIRES]
#                       [NO_DEFAULT_INCLUDE] [NO_DEFAULT_LIBS]
#                       [OUTPUT <path>]
#                       [INSTALL] [INSTALL_DESTINATION <dir>])
#
# The following values are inferred when their arguments are omitted:
#
#   Name          target name
#   Description   target DESCRIPTION, PROJECT_DESCRIPTION, or Name
#   Version       target VERSION or PROJECT_VERSION
#   -l<name>      target OUTPUT_NAME or target name
#
# The remaining keyword arguments correspond to pkg-config fields. For example,
# CFLAGS_PRIVATE produces Cflags.private. Each file defines prefix, exec_prefix,
# libdir, and includedir. By default, it also contains:
#
#   Cflags: -I${includedir}
#   Libs:   -L${libdir} -l<name>    (skipped for header-only targets)
#
# CFLAGS and LIBS are appended to these defaults. NO_DEFAULT_INCLUDE and
# NO_DEFAULT_LIBS suppress the corresponding default.
#
# PREFIX defaults to CMAKE_INSTALL_PREFIX. LIBDIR and INCLUDEDIR use
# CMAKE_INSTALL_LIBDIR and CMAKE_INSTALL_INCLUDEDIR when set, or "lib" and
# "include" otherwise. Relative directories are based on exec_prefix and
# prefix, respectively. Absolute directories are written unchanged.
#
# RELOCATABLE expresses prefix relative to ${pcfiledir}. This requires the
# pkg-config directory to have a fixed location relative to the install prefix.
#
# Linked pkgconf::<module> targets created by pkgconf-config.cmake are added to
# Requires or Requires.private. NO_INFER_REQUIRES disables this inference.
#
# OUTPUT defaults to ${CMAKE_CURRENT_BINARY_DIR}/<name>.pc. INSTALL installs the
# generated file in INSTALL_DESTINATION, which defaults to <libdir>/pkgconfig.

cmake_minimum_required(VERSION 3.13)

# Escape characters that are significant to pkgconf's fragment parser. This is
# used for generated paths; caller-supplied CFLAGS and LIBS remain unchanged.
function(_pkgconf_pc_escape _out_var _value)
  # Escape backslashes before adding the other escapes.
  string(REPLACE "\\" "\\\\" _value "${_value}")
  string(REPLACE " " "\\ " _value "${_value}")
  string(REPLACE "\"" "\\\"" _value "${_value}")
  string(REPLACE "'" "\\'" _value "${_value}")
  set(${_out_var} "${_value}" PARENT_SCOPE)
endfunction()

# Join a CMake list into a pkg-config fragment list.
function(_pkgconf_pc_join _out_var _list)
  list(JOIN _list " " _joined)
  set(${_out_var} "${_joined}" PARENT_SCOPE)
endfunction()

# Return the module specification attached to a pkgconf imported target.
function(_pkgconf_pc_module_of _item _out_var)
  set(${_out_var} "" PARENT_SCOPE)
  if(TARGET ${_item})
    get_target_property(_mods ${_item} PKGCONF_MODULES)
    if(_mods)
      set(${_out_var} "${_mods}" PARENT_SCOPE)
    endif()
  endif()
endfunction()

# Collect pkgconf dependencies from a target's public and private link
# interfaces. CMake wraps private entries propagated for static linking in
# $<LINK_ONLY:...>.
function(_pkgconf_pc_collect_requires _target _public_var _private_var)
  set(_public "")
  set(_private "")

  get_target_property(_iface ${_target} INTERFACE_LINK_LIBRARIES)
  if(_iface)
    foreach(_item IN LISTS _iface)
      if(_item MATCHES "^\\$<LINK_ONLY:(.+)>$")
        _pkgconf_pc_module_of("${CMAKE_MATCH_1}" _mod)
        if(_mod)
          list(APPEND _private ${_mod})
        endif()
      else()
        _pkgconf_pc_module_of("${_item}" _mod)
        if(_mod)
          list(APPEND _public ${_mod})
        endif()
      endif()
    endforeach()
  endif()

  get_target_property(_own ${_target} LINK_LIBRARIES)
  if(_own)
    foreach(_item IN LISTS _own)
      _pkgconf_pc_module_of("${_item}" _mod)
      if(_mod AND NOT _mod IN_LIST _public)
        list(APPEND _private ${_mod})
      endif()
    endforeach()
  endif()

  if(_public)
    list(REMOVE_DUPLICATES _public)
  endif()
  if(_private)
    list(REMOVE_DUPLICATES _private)
  endif()
  set(${_public_var} "${_public}" PARENT_SCOPE)
  set(${_private_var} "${_private}" PARENT_SCOPE)
endfunction()

set(_pkgconf_generate_options
  RELOCATABLE NO_INFER_REQUIRES NO_DEFAULT_INCLUDE NO_DEFAULT_LIBS INSTALL)
set(_pkgconf_generate_one_value_args
  NAME DESCRIPTION VERSION URL LICENSE MAINTAINER COPYRIGHT
  PREFIX LIBDIR INCLUDEDIR OUTPUT INSTALL_DESTINATION)
set(_pkgconf_generate_multi_value_args
  REQUIRES REQUIRES_PRIVATE CONFLICTS PROVIDES
  CFLAGS CFLAGS_PRIVATE LIBS LIBS_PRIVATE VARIABLES)

function(pkgconf_generate_pc _target)
  cmake_parse_arguments(_arg
    "${_pkgconf_generate_options}"
    "${_pkgconf_generate_one_value_args}"
    "${_pkgconf_generate_multi_value_args}"
    ${ARGN})

  if(NOT TARGET ${_target})
    message(FATAL_ERROR "pkgconf_generate_pc: \"${_target}\" is not a target")
  endif()

  # Package metadata and library name.
  if(_arg_NAME)
    set(_name "${_arg_NAME}")
  else()
    set(_name "${_target}")
  endif()

  if(_arg_DESCRIPTION)
    set(_description "${_arg_DESCRIPTION}")
  else()
    get_target_property(_description ${_target} DESCRIPTION)
    if(NOT _description)
      if(PROJECT_DESCRIPTION)
        set(_description "${PROJECT_DESCRIPTION}")
      else()
        set(_description "${_name}")
      endif()
    endif()
  endif()

  if(_arg_VERSION)
    set(_version "${_arg_VERSION}")
  else()
    get_target_property(_version ${_target} VERSION)
    if(NOT _version)
      set(_version "${PROJECT_VERSION}")
    endif()
  endif()
  if(NOT _version)
    message(FATAL_ERROR
      "pkgconf_generate_pc: no version for \"${_target}\"; pass VERSION, set "
      "the target's VERSION property, or give the project() a VERSION")
  endif()

  get_target_property(_link_name ${_target} OUTPUT_NAME)
  if(NOT _link_name)
    set(_link_name "${_target}")
  endif()

  # Installation directories.
  if(_arg_PREFIX)
    set(_prefix "${_arg_PREFIX}")
  else()
    set(_prefix "${CMAKE_INSTALL_PREFIX}")
  endif()

  if(_arg_LIBDIR)
    set(_libdir "${_arg_LIBDIR}")
  elseif(CMAKE_INSTALL_LIBDIR)
    set(_libdir "${CMAKE_INSTALL_LIBDIR}")
  else()
    set(_libdir "lib")
  endif()

  if(_arg_INCLUDEDIR)
    set(_includedir "${_arg_INCLUDEDIR}")
  elseif(CMAKE_INSTALL_INCLUDEDIR)
    set(_includedir "${CMAKE_INSTALL_INCLUDEDIR}")
  else()
    set(_includedir "include")
  endif()

  # The destination is also used to calculate a relocatable prefix.
  if(_arg_INSTALL_DESTINATION)
    set(_destination "${_arg_INSTALL_DESTINATION}")
  else()
    set(_destination "${_libdir}/pkgconfig")
  endif()

  if(_arg_RELOCATABLE)
    get_filename_component(_prefix_abs "${_prefix}" ABSOLUTE)
    if(IS_ABSOLUTE "${_destination}")
      set(_pcdir_abs "${_destination}")
    else()
      set(_pcdir_abs "${_prefix_abs}/${_destination}")
    endif()
    file(RELATIVE_PATH _rel "${_pcdir_abs}" "${_prefix_abs}")
    _pkgconf_pc_escape(_rel "${_rel}")
    if(_rel STREQUAL "")
      set(_prefix_value "\${pcfiledir}")
    else()
      set(_prefix_value "\${pcfiledir}/${_rel}")
    endif()
  else()
    _pkgconf_pc_escape(_prefix_value "${_prefix}")
  endif()

  if(IS_ABSOLUTE "${_libdir}")
    _pkgconf_pc_escape(_libdir_value "${_libdir}")
  else()
    _pkgconf_pc_escape(_libdir_value "${_libdir}")
    set(_libdir_value "\${exec_prefix}/${_libdir_value}")
  endif()

  if(IS_ABSOLUTE "${_includedir}")
    _pkgconf_pc_escape(_includedir_value "${_includedir}")
  else()
    _pkgconf_pc_escape(_includedir_value "${_includedir}")
    set(_includedir_value "\${prefix}/${_includedir_value}")
  endif()

  # Compiler and linker flags.
  set(_cflags "")
  if(NOT _arg_NO_DEFAULT_INCLUDE)
    list(APPEND _cflags "-I\${includedir}")
  endif()
  if(_arg_CFLAGS)
    list(APPEND _cflags ${_arg_CFLAGS})
  endif()

  get_target_property(_type ${_target} TYPE)
  set(_libs "")
  if(NOT _arg_NO_DEFAULT_LIBS AND NOT _type STREQUAL "INTERFACE_LIBRARY")
    list(APPEND _libs "-L\${libdir}" "-l${_link_name}")
  endif()
  if(_arg_LIBS)
    list(APPEND _libs ${_arg_LIBS})
  endif()

  # Keep fields in the conventional pkg-config order.
  set(_lines "# Generated by pkgconf's CMake support (pkgconf_generate_pc())\; do not edit.")
  list(APPEND _lines "")
  list(APPEND _lines "prefix=${_prefix_value}")
  list(APPEND _lines "exec_prefix=\${prefix}")
  list(APPEND _lines "libdir=${_libdir_value}")
  list(APPEND _lines "includedir=${_includedir_value}")
  foreach(_var IN LISTS _arg_VARIABLES)
    list(APPEND _lines "${_var}")
  endforeach()
  list(APPEND _lines "")

  list(APPEND _lines "Name: ${_name}")
  list(APPEND _lines "Description: ${_description}")
  if(_arg_URL)
    list(APPEND _lines "URL: ${_arg_URL}")
  endif()
  list(APPEND _lines "Version: ${_version}")
  if(_arg_MAINTAINER)
    list(APPEND _lines "Maintainer: ${_arg_MAINTAINER}")
  endif()
  if(_arg_LICENSE)
    list(APPEND _lines "License: ${_arg_LICENSE}")
  endif()
  if(_arg_COPYRIGHT)
    list(APPEND _lines "Copyright: ${_arg_COPYRIGHT}")
  endif()

  # Explicit requirements precede requirements inferred from linked targets.
  set(_derived_requires "")
  set(_derived_requires_private "")
  if(NOT _arg_NO_INFER_REQUIRES)
    _pkgconf_pc_collect_requires(${_target} _derived_requires _derived_requires_private)
  endif()
  set(_requires_list ${_arg_REQUIRES} ${_derived_requires})
  if(_requires_list)
    _pkgconf_pc_join(_requires "${_requires_list}")
    list(APPEND _lines "Requires: ${_requires}")
  endif()
  set(_requires_private_list ${_arg_REQUIRES_PRIVATE} ${_derived_requires_private})
  if(_requires_private_list)
    _pkgconf_pc_join(_requires_private "${_requires_private_list}")
    list(APPEND _lines "Requires.private: ${_requires_private}")
  endif()
  _pkgconf_pc_join(_conflicts "${_arg_CONFLICTS}")
  if(_conflicts)
    list(APPEND _lines "Conflicts: ${_conflicts}")
  endif()
  _pkgconf_pc_join(_provides "${_arg_PROVIDES}")
  if(_provides)
    list(APPEND _lines "Provides: ${_provides}")
  endif()

  _pkgconf_pc_join(_cflags_str "${_cflags}")
  if(_cflags_str)
    list(APPEND _lines "Cflags: ${_cflags_str}")
  endif()
  _pkgconf_pc_join(_cflags_priv "${_arg_CFLAGS_PRIVATE}")
  if(_cflags_priv)
    list(APPEND _lines "Cflags.private: ${_cflags_priv}")
  endif()
  _pkgconf_pc_join(_libs_str "${_libs}")
  if(_libs_str)
    list(APPEND _lines "Libs: ${_libs_str}")
  endif()
  _pkgconf_pc_join(_libs_priv "${_arg_LIBS_PRIVATE}")
  if(_libs_priv)
    list(APPEND _lines "Libs.private: ${_libs_priv}")
  endif()

  if(_arg_OUTPUT)
    set(_output "${_arg_OUTPUT}")
  else()
    set(_output "${CMAKE_CURRENT_BINARY_DIR}/${_name}.pc")
  endif()

  list(JOIN _lines "\n" _content)
  file(WRITE "${_output}" "${_content}\n")

  if(_arg_INSTALL)
    install(FILES "${_output}" DESTINATION "${_destination}")
  endif()
endfunction()
