# CMake configuration for pkgconf
#
# Caution: this assumes you don't set CMAKE_BUILD_TYPE
#
# FIXME: this isn't a native cmake approach, it's just a straight translation
# of configure.ac + Makefile.am, barely good enough to work on Linux, Mac, and Windows.

# Require recent cmake, but not so recent that Ubuntu 16.04 users have to upgrade.
CMAKE_MINIMUM_REQUIRED(VERSION 3.5.1 FATAL_ERROR)

PROJECT(pkgconf C)

SET(PACKAGE_BUGREPORT http://github.com/pkgconf/pkgconf/issues)
SET(PACKAGE_NAME pkgconf)
SET(PACKAGE_VERSION 1.3.7)
SET(LIBPKGCONF_VERSION "3.0.0")
SET(LIBPKGCONF_SOVERSION 3)

#-------- GNU directory variables ---------

SET(abs_top_srcdir ${pkgconf_SOURCE_DIR})
SET(prefix ${CMAKE_INSTALL_PREFIX})
SET(exec_prefix ${prefix})
SET(datarootdir ${prefix}/share)
SET(datadir ${datarootdir})
SET(libdir ${prefix}/lib)
SET(includedir ${prefix}/include)

#-------- User-settable options ---------

# FIXME: this is overridden in get_default_pkgconfig_path() on windows, but not in test_env.sh.in?!
SET(pkg_config_dir "${libdir}/pkgconfig:${datadir}/pkgconfig" CACHE STRING "specify the places where pc files will be found")
SET(PKGCONFIGDIR "${pkg_config_dir}")
SET(pkg_default_dir "${PKGCONFIGDIR}")   # c'mon, make up your mind

SET(personality_dir "${libdir}/pkgconfig/personality.d:${datadir}/pkgconfig/personality.d" CACHE STRING "specify the places where personality files will be found")

SET(system_libdir "${libdir}" CACHE STRING "specify the system library directory (default LIBDIR)")
SET(SYSTEM_LIBDIR "${system_libdir}")

SET(system_includedir "${includedir}" CACHE STRING "specify the system include directory (default INCLUDEDIR)")
SET(SYSTEM_INCLUDEDIR "${system_includedir}")

#-------- Probe system ---------

INCLUDE (CheckIncludeFiles)
CHECK_INCLUDE_FILES(sys/stat.h HAVE_SYS_STAT_H)
INCLUDE (CheckFunctionExists)
CHECK_FUNCTION_EXISTS(strlcpy HAVE_STRLCPY)
CHECK_FUNCTION_EXISTS(strlcat HAVE_STRLCAT)
CHECK_FUNCTION_EXISTS(strndup HAVE_STRNDUP)
CHECK_FUNCTION_EXISTS(cygwin_conv_path HAVE_CYGWIN_CONV_PATH)

#-------- Generate source files ---------

CONFIGURE_FILE(libpkgconf/config.h.cmake.in libpkgconf/config.h @ONLY)

#-------- Configure common compiler options --------

IF (MSVC)
  # Make warnings fatal... but ignore C4996: 'strdup' two different ways
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /WX /wd4996")
  # Ignore warning C4996: 'strncpy'
  ADD_DEFINITIONS("-D_CRT_SECURE_NO_WARNINGS=1")
ELSE()
  INCLUDE(CheckCCompilerFlag)
  CHECK_C_COMPILER_FLAG("-Wall" COMPILER_HAS_WALL)
  IF (COMPILER_HAS_WALL)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
  ENDIF()
  CHECK_C_COMPILER_FLAG("-Wextra" COMPILER_HAS_WEXTRA)
  IF (COMPILER_HAS_WEXTRA)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wextra")
  ENDIF()
  CHECK_C_COMPILER_FLAG("-Wformat=2" COMPILER_HAS_WFORMAT)
  IF (COMPILER_HAS_WFORMAT)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wformat=2")
  ENDIF()
  CHECK_C_COMPILER_FLAG("-std=gnu99" COMPILER_HAS_STD_GNU99)
  CHECK_C_COMPILER_FLAG("-std=c99" COMPILER_HAS_STD_C99)
  IF (COMPILER_HAS_STD_GNU99)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
  ELSEIF (COMPILER_HAS_STD_C99)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
  ENDIF()
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -g")
ENDIF()

INCLUDE_DIRECTORIES(${pkgconf_SOURCE_DIR} ${pkgconf_BINARY_DIR})
ADD_DEFINITIONS(-DPERSONALITY_PATH=\"${personality_dir}\")
ADD_DEFINITIONS(-DPKG_DEFAULT_PATH=\"${pkg_default_dir}\")
ADD_DEFINITIONS(-DSYSTEM_INCLUDEDIR=\"${system_includedir}\")
ADD_DEFINITIONS(-DSYSTEM_LIBDIR=\"${system_libdir}\")

#-------- Build and install library --------

# Place shared libraries in same place as binary, for ease of setting PATH in test_env.sh
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${pkgconf_BINARY_DIR})
ADD_SUBDIRECTORY(libpkgconf)

#-------- Build and install executable --------

INCLUDE_DIRECTORIES(${libpkgconf_BINARY_DIR})
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Icli")
ADD_EXECUTABLE(pkgconf cli/main.c cli/getopt_long.c cli/renderer-msvc.c)
TARGET_LINK_LIBRARIES(pkgconf libpkgconf)
INSTALL(TARGETS pkgconf DESTINATION bin)

#-------- Tests ---------

ENABLE_TESTING()

# Handy that these files need configuring; cygwin atf doesn't like windows line endings, and NEWLINE_STYLE helps.
FOREACH(file Kyuafile tests/Kyuafile tests/test_env.sh)
   CONFIGURE_FILE(${file}.in ${file} @ONLY NEWLINE_STYLE UNIX)
ENDFOREACH()

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
# Handy that these files need configuring; cygwin atf doesn't like windows line endings, and NEWLINE_STYLE helps.
FOREACH(file ${test_scripts})
   CONFIGURE_FILE(${file}.sh ${file} @ONLY NEWLINE_STYLE UNIX)
ENDFOREACH()

ADD_TEST(kyua kyua --config=none test)
