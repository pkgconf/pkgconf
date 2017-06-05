CMAKE_MINIMUM_REQUIRED(VERSION 3.5.1)
PROJECT(pkgconf C)

SET(PACKAGE_BUGREPORT http://github.com/pkgconf/pkgconf/issues)
SET(PACKAGE_NAME pkgconf)
SET(PACKAGE_VERSION 1.3.7)

# Expand config.h
INCLUDE (CheckIncludeFiles)
CHECK_INCLUDE_FILES(sys/stat.h HAVE_SYS_STAT_H)
INCLUDE (CheckFunctionExists)
CHECK_FUNCTION_EXISTS(strlcpy HAVE_STRLCPY)
CHECK_FUNCTION_EXISTS(strlcat HAVE_STRLCAT)
CHECK_FUNCTION_EXISTS(strndup HAVE_STRNDUP)
CHECK_FUNCTION_EXISTS(cygwin_conv_path HAVE_CYGWIN_CONV_PATH)
SET(abs_top_srcdir ${pkgconf_SOURCE_DIR})
CONFIGURE_FILE(libpkgconf/config.h.cmake.in libpkgconf/config.h @ONLY)
# Expand others
FOREACH(file Kyuafile libpkgconf.pc tests/Kyuafile tests/test_env.sh)
   CONFIGURE_FILE(${file}.in ${file} @ONLY)
ENDFOREACH()

IF (WIN32)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /WX")
ELSE()
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wformat=2 -std=gnu99")
ENDIF()

INCLUDE_DIRECTORIES(${pkgconf_SOURCE_DIR} ${pkgconf_BINARY_DIR})
ADD_DEFINITIONS(-DPKG_DEFAULT_PATH=\"$(pkg_default_dir)\")
ADD_DEFINITIONS(-DSYSTEM_INCLUDEDIR=\"$(system_includedir)\")
ADD_DEFINITIONS(-DSYSTEM_LIBDIR=\"$(system_libdir)\")

ADD_SUBDIRECTORY(libpkgconf)

INCLUDE_DIRECTORIES(${libpkgconf_BINARY_DIR})
ADD_EXECUTABLE(pkgconf main.c getopt_long.c)
TARGET_LINK_LIBRARIES(pkgconf libpkgconf)
INSTALL(TARGETS pkgconf DESTINATION bin)

ENABLE_TESTING()

SET(test_scripts
    tests/basic
    tests/builtins
    tests/conflicts
    tests/framework
    tests/parser
    tests/provides
    tests/regress
    tests/requires
    tests/sysroot
    tests/version
   )
# No accounting for taste
FOREACH(file ${test_scripts})
   CONFIGURE_FILE(${file}.sh ${file} @ONLY)
ENDFOREACH()

ADD_TEST(kyua
    kyua --config=none test --kyuafile=${pkgconf_BINARY_DIR}/Kyuafile --build-root=${pkgconf_BINARY_DIR}
    )
