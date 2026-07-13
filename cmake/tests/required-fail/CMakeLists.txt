# pkgconf_check_modules(... REQUIRED ...) should fail configure (nonzero exit)
# for a module that does not exist. CI runs this and expects it to fail.

cmake_minimum_required(VERSION 3.13)
project(pkgconf_cmake_macros_required_fail_test C)

find_package(pkgconf REQUIRED CONFIG)
pkgconf_check_modules(MISSING REQUIRED this-module-does-not-exist-xyz)
