from cffi import FFI

from pathlib import Path
import re
import sys


def sanitize_lipkgconf_h(lipkgconf_h: Path) -> str:
    cdef = lipkgconf_h.read_text()
    cdef = cdef.replace('#ifndef LIBPKGCONF__LIBPKGCONF_H\n', '')
    cdef = cdef.replace('#define LIBPKGCONF__LIBPKGCONF_H\n', '')
    cdef = re.sub('#include <.+>\n', '', cdef)
    cdef = re.sub('#if ((.|\n)*?)#endif.*\n', '', cdef, 0)
    cdef = re.sub('#ifn?def ((.|\n)*?)#endif.*\n', '', cdef, 0)
    cdef = re.sub('#define.*["()](.*\\\\\n)*(.*\n)', '', cdef)
    cdef = cdef.replace('#endif\n', '')
    cdef = cdef.replace('PKGCONF_API ', '')
    cdef = re.sub(r' PRINTFLIKE\(.*?\)', '', cdef)
    return cdef


if __name__ == '__main__':
    lipkgconf_h = Path(sys.argv[1])
    cdef = """
// from iter.h
typedef struct pkgconf_node_ pkgconf_node_t;

struct pkgconf_node_ {
	pkgconf_node_t *prev, *next;
	void *data;
};

typedef struct {
	pkgconf_node_t *head, *tail;
	size_t length;
} pkgconf_list_t;
"""
    cdef += sanitize_lipkgconf_h(lipkgconf_h)
    cdef += """
#define PKGCONF_BUFSIZE ...

/* Python callbacks */
extern "Python" bool error_handler(const char *msg, const pkgconf_client_t *client, void *data);
extern "Python" bool print_list_entry(const pkgconf_pkg_t *entry, void *data);
extern "Python" bool print_package_entry(const pkgconf_pkg_t *entry, void *data);
extern "Python" void print_license(pkgconf_client_t *client, pkgconf_pkg_t *pkg, void *data);
extern "Python" void check_uninstalled(pkgconf_client_t *client, pkgconf_pkg_t *pkg, void *data);
extern "Python" bool filter_cflags(const pkgconf_client_t *client, const pkgconf_fragment_t *frag, void *data);
extern "Python" bool filter_libs(const pkgconf_client_t *client, const pkgconf_fragment_t *frag, void *data);
"""

    tmp = Path(sys.argv[2]).with_suffix('.cdef')
    tmp.write_text(cdef)

    ffibuilder = FFI()
    ffibuilder.set_source('_libpkgconf', '#include <libpkgconf/libpkgconf.h>')
    ffibuilder.cdef(cdef)

    ffibuilder.emit_c_code(sys.argv[2])
