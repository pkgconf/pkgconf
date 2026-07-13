# Exercise the pkgconf CMake macros against zlib.pc.

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

# Use only the imported target to verify its usage requirements.
add_executable(zlib-consumer consumer.c)
target_link_libraries(zlib-consumer PRIVATE pkgconf::ZLIB)
