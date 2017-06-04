PROJECT(libpkgconf C)

ADD_LIBRARY(libpkgconf
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
