# Exercise pkgconf-config.cmake against the installed libpkgconf.pc.
#
# CI configures pkgconf with -Dwith-system-libdir=/some-marker-that-cannot-exist
# so libpkgconf's own libdir stays in --libs-only-L output. pkgconf otherwise
# drops -L for dirs matching its compiled-in SYSTEM_LIBDIR, which defaults to
# the install prefix's libdir -- where this test installs libpkgconf.

cmake_minimum_required(VERSION 3.13)
project(pkgconf_cmake_macros_test C)

find_package(pkgconf REQUIRED CONFIG)
message(STATUS "pkgconf_VERSION = ${pkgconf_VERSION}")
if(NOT pkgconf_VERSION)
  message(FATAL_ERROR "pkgconf_VERSION was not set by find_package(pkgconf CONFIG)")
endif()

pkgconf_check_modules(LIBPKGCONF REQUIRED IMPORTED_TARGET libpkgconf)
if(NOT LIBPKGCONF_FOUND)
  message(FATAL_ERROR "pkgconf_check_modules() failed to find libpkgconf")
endif()
if(NOT LIBPKGCONF_INCLUDE_DIRS)
  message(FATAL_ERROR "LIBPKGCONF_INCLUDE_DIRS was not populated")
endif()
if(NOT LIBPKGCONF_LIBRARIES)
  message(FATAL_ERROR "LIBPKGCONF_LIBRARIES was not populated")
endif()

add_executable(consumer consumer.c)
target_link_libraries(consumer PRIVATE PkgConf::LIBPKGCONF)

# The search should skip the missing alternative.
pkgconf_search_module(LIBPKGCONF2 REQUIRED this-module-does-not-exist-xyz libpkgconf)
if(NOT LIBPKGCONF2_MODULE_NAME STREQUAL "libpkgconf")
  message(FATAL_ERROR "pkgconf_search_module() picked the wrong module: \"${LIBPKGCONF2_MODULE_NAME}\"")
endif()

# A missing optional module must not stop configuration.
pkgconf_check_modules(MISSING QUIET this-module-does-not-exist-xyz)
if(MISSING_FOUND)
  message(FATAL_ERROR "pkgconf_check_modules() reported FOUND for a module that does not exist")
endif()

pkgconf_get_variable(LIBPKGCONF_PCFILEDIR libpkgconf pcfiledir)
if(NOT LIBPKGCONF_PCFILEDIR)
  message(FATAL_ERROR "pkgconf_get_variable() returned an empty value")
endif()

pkgconf_get_variable(LIBPKGCONF_PREFIX_OVERRIDE libpkgconf prefix DEFINE_VARIABLE prefix=/pkgconf-cmake-override)
if(NOT LIBPKGCONF_PREFIX_OVERRIDE STREQUAL "/pkgconf-cmake-override")
  message(FATAL_ERROR "pkgconf_get_variable(... DEFINE_VARIABLE ...) did not override prefix: \"${LIBPKGCONF_PREFIX_OVERRIDE}\"")
endif()

pkgconf_get_path(LIBPKGCONF_PATH libpkgconf)
if(NOT LIBPKGCONF_PATH MATCHES "libpkgconf[.]pc$")
  message(FATAL_ERROR "pkgconf_get_path() returned unexpected path: \"${LIBPKGCONF_PATH}\"")
endif()

pkgconf_check_version(LIBPKGCONF_ATLEAST_0 libpkgconf ATLEAST 0)
if(NOT LIBPKGCONF_ATLEAST_0)
  message(FATAL_ERROR "pkgconf_check_version(ATLEAST 0) should be TRUE")
endif()

pkgconf_check_version(LIBPKGCONF_ATLEAST_999 libpkgconf ATLEAST 999.0)
if(LIBPKGCONF_ATLEAST_999)
  message(FATAL_ERROR "pkgconf_check_version(ATLEAST 999.0) should be FALSE")
endif()

pkgconf_variable_names(LIBPKGCONF_VARNAMES libpkgconf)
list(FIND LIBPKGCONF_VARNAMES "includedir" _libpkgconf_varnames_idx)
if(_libpkgconf_varnames_idx EQUAL -1)
  message(FATAL_ERROR "pkgconf_variable_names() did not include \"includedir\"")
endif()

pkgconf_print_provides(LIBPKGCONF_PROVIDES libpkgconf)
pkgconf_print_requires(LIBPKGCONF_REQUIRES libpkgconf)
pkgconf_print_requires(LIBPKGCONF_REQUIRES_PRIVATE libpkgconf PRIVATE)
pkgconf_link_abi(LIBPKGCONF_LINK_ABI libpkgconf)

pkgconf_list_all(PKGCONF_LIST_ALL)
set(_pkgconf_list_all_has_libpkgconf FALSE)
foreach(_pkgconf_list_all_line IN LISTS PKGCONF_LIST_ALL)
  if(_pkgconf_list_all_line MATCHES "^libpkgconf[ \t]")
    set(_pkgconf_list_all_has_libpkgconf TRUE)
  endif()
endforeach()
if(NOT _pkgconf_list_all_has_libpkgconf)
  message(FATAL_ERROR "pkgconf_list_all() did not include libpkgconf")
endif()
