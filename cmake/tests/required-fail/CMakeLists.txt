# Configuration must fail when a required module does not exist.

cmake_minimum_required(VERSION 3.13)
project(pkgconf_cmake_macros_required_fail_test C)

find_package(pkgconf REQUIRED CONFIG)
pkgconf_check_modules(MISSING REQUIRED this-module-does-not-exist-xyz)
