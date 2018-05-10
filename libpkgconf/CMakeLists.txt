PROJECT(libpkgconf C)

# Enforce visibiliity restrictions when building shared libraries on Unix.
SET(CMAKE_CXX_VISIBILITY_PRESET hidden)
ADD_DEFINITIONS(-DLIBPKGCONF_EXPORT)

ADD_LIBRARY(libpkgconf SHARED
    argvsplit.c
    audit.c
    bsdstubs.c
    cache.c
    client.c
    dependency.c
    fileio.c
    fragment.c
    parser.c
    path.c
    personality.c
    pkg.c
    queue.c
    tuple.c
)
SET_TARGET_PROPERTIES(libpkgconf PROPERTIES VERSION ${LIBPKGCONF_VERSION} SOVERSION ${LIBPKGCONF_SOVERSION})
INSTALL(TARGETS libpkgconf
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
INSTALL(
  FILES bsdstubs.h iter.h libpkgconf.h libpkgconf-api.h stdinc.h
  DESTINATION include/libpkgconf
)

# Hypothesis: .pc files are a Unix thing, should always have unix line endings.
CONFIGURE_FILE(${pkgconf_SOURCE_DIR}/libpkgconf.pc.in libpkgconf.pc @ONLY NEWLINE_STYLE UNIX)
INSTALL(FILES ${libpkgconf_BINARY_DIR}/libpkgconf.pc DESTINATION lib/pkgconfig)
