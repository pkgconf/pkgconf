# Tests the pkgconf cmake macros against a real third-party package, zlib,
# found through its installed zlib.pc. The basic test only queries pkgconf's
# own libpkgconf.pc, so this covers the more usual case of consuming a .pc
# file that ships with some other library.

cmake_minimum_required(VERSION 3.13)
project(pkgconf_cmake_macros_zlib_test C)

find_package(pkgconf REQUIRED CONFIG)

pkgconf_check_modules(ZLIB REQUIRED IMPORTED_TARGET zlib)
if(NOT ZLIB_FOUND)
  message(FATAL_ERROR "pkgconf_check_modules() failed to find zlib")
endif()
if(NOT ZLIB_VERSION)
  message(FATAL_ERROR "ZLIB_VERSION was not set for a found module")
endif()
message(STATUS "ZLIB_VERSION = ${ZLIB_VERSION}")

# Rely entirely on the imported target so the include dirs, link dirs and
# -lz coming out of the macros all have to be correct for this to build/run.
add_executable(zlib-consumer consumer.c)
target_link_libraries(zlib-consumer PRIVATE PkgConf::ZLIB)
