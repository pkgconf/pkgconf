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
    path.c
    pkg.c
    queue.c
    tuple.c
)
INSTALL(TARGETS libpkgconf
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
SET_TARGET_PROPERTIES(libpkgconf PROPERTIES VERSION "2.0.0" SOVERSION 2)
