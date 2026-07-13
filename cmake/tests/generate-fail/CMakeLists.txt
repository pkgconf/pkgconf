# No VERSION argument, target VERSION property, or project version is provided.

cmake_minimum_required(VERSION 3.13)
project(pkgconf_cmake_generate_pc_fail_test LANGUAGES C)

find_package(pkgconf REQUIRED CONFIG)

add_library(no_version STATIC ../generate/sample.c)
pkgconf_generate_pc(no_version)
