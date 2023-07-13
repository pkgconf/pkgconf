from ._libpkgconf import ffi, lib

from .constants import Flags
from .logger import logger


@ffi.def_extern()
def error_handler(msg: str, client, data) -> bool:
    """ This is a Python callback to handle errors """
    log_level = ffi.from_handle(data)

    logger.log(log_level, ffi.string(msg).decode(errors='replace').rstrip())
    return True


@ffi.def_extern()
def print_list_entry(entry, data) -> bool:
    results = ffi.from_handle(data)

    if entry.flags & lib.PKGCONF_PKG_PROPF_UNINSTALLED:
        return False
    
    entry_id = ffi.string(entry.id).decode()
    realname = ffi.string(entry.realname).decode()
    description = ffi.string(entry.description).decode()
    results.append(f'{entry_id:<30s} {realname} - {description}')

    return False


@ffi.def_extern()
def print_package_entry(entry, data) -> bool:
    results = ffi.from_handle(data)

    if entry.flags & lib.PKGCONF_PKG_PROPF_UNINSTALLED:
        return False
    
    results.append(str(entry.id))

    return False


@ffi.def_extern()
def print_license(client, pkg, data) -> None:
    result = ffi.from_handle(data)

    if pkg.flags & lib.PKGCONF_PKG_PROPF_VIRTUAL:
        return
    
    pkg_id = ffi.string(pkg.id).decode()
    # NOASSERTION is the default when the license is unknown, per SPDX spec § 3.15
    pkg_license = ffi.string(pkg.license).decode() if pkg.license != ffi.NULL else "NOASSERTION"
    result.append(f'{pkg_id}: {pkg_license}')


@ffi.def_extern()
def check_uninstalled(client, pkg, data) -> None:
    ret = ffi.from_handle(data)

    if pkg.flags & lib.PKGCONF_PKG_PROPF_UNINSTALLED:
        ret[0] = True


@ffi.def_extern()
def filter_cflags(client, frag, data) -> bool:
    want_flags, want_fragment_filter = ffi.from_handle(data)

    if Flags.KEEP_SYSTEM_CFLAGS not in want_flags and lib.pkgconf_fragment_has_system_dir(client, frag):
        return False
    
    frag_type = frag.type.decode()
    if (want_fragment_filter is not None
            and (frag_type not in want_fragment_filter or not frag_type)):
        return False
    
    got_flags = Flags.DEFAULT
    if frag_type == 'I':
        got_flags = Flags.CFLAGS_ONLY_I
    else:
        got_flags = Flags.CFLAGS_ONLY_OTHER

    return got_flags in want_flags


@ffi.def_extern()
def filter_libs(client, frag, data) -> bool:
    want_flags, want_fragment_filter = ffi.from_handle(data)

    if Flags.KEEP_SYSTEM_LIBS not in want_flags and lib.pkgconf_fragment_has_system_dir(client, frag):
        return False
    
    frag_type = frag.type.decode()
    if (want_fragment_filter is not None
            and (frag_type not in want_fragment_filter or not frag_type)):
        return False
    
    got_flags = Flags.DEFAULT
    if frag_type == 'L':
        got_flags = Flags.LIBS_ONLY_LDPATH
    elif frag_type == 'l':
        got_flags = Flags.LIBS_ONLY_LIBNAME
    else:
        got_flags = Flags.LIBS_ONLY_OTHER

    return got_flags in want_flags
