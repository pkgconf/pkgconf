import argparse
from enum import IntFlag
from functools import reduce
import logging
import operator
import os
import re
import shlex
import sys
import typing as T

from ._libpkgconf import ffi, lib
from .constants import Flags, MAXIMUM_TRAVERSE_DEPTH, OPERATORS
from .logger import logger, set_file_handler, set_stream_handler
from .util import NodeIter

class PkgConfError(RuntimeError): ...


def _render_fragments(fragment_list, fmt='{}') -> T.List[str]:
    result = []

    # Native code does: 
    # render_buf = lib.pkgconf_fragment_render(fragment_list, True, ffi.NULL)
    # result.append(ffi.string(render_buf).decode())
    # but we want to split as list

    for frag in NodeIter(fragment_list, 'pkgconf_fragment_t *'):
        item = ffi.string(frag.data).decode()

        # escape special chars
        if sys.platform not in {'win32', 'cygwin', 'msys'}:
            item = item .replace('\\', '\\\\')
        item = re.sub('[^ $()+-:=@A-Z\\^_a-z~]', R'\\\g<0>', item)
        if not frag.merged:
            item = item.replace(' ', '\\ ')

        if frag.type != b'\0':
            item = f'-{frag.type.decode()}{item}'
        item = fmt.format(item)

        if frag.merged:
            result.extend(item.split(' '))
        else:
            result.append(item)

    return result

def relocate_path(path: str) -> str:
    buf = path.encode()
    if len(buf) > lib.PKGCONF_BUFSIZE-1:
        buf = buf[:lib.PKGCONF_BUFSIZE-1]
    cbuf = ffi.new('char[]', buf)

    lib.pkgconf_path_relocate(cbuf, ffi.sizeof(cbuf))
    return ffi.string(cbuf).decode()

def apply_license(client, world, maxdepth) -> T.List[str]:
    result = []
    result_handle = ffi.new_handle(result)
    eflag = lib.pkgconf_pkg_traverse(client, world, lib.print_license, result_handle, maxdepth, 0)
    if eflag != lib.PKGCONF_PKG_ERRF_OK:
        return []
    
    return result

def version() -> str:
    return lib.PACKAGE_VERSION


def apply_uninstalled(client, world, data, maxdepth) -> bool:
    ret_handle = ffi.new_handle(data)
    eflag = lib.pkgconf_pkg_traverse(client, world, lib.check_uninstalled, ret_handle, maxdepth, 0)

    return eflag == lib.PKGCONF_PKG_ERRF_OK


def apply_env_var(prefix, client, world, maxdepth, collect_fn, filter_fn, result, data_handle):
    try:
        unfiltered_list = ffi.new('pkgconf_list_t*')
        filtered_list = ffi.new('pkgconf_list_t*')

        eflag = collect_fn(client, world, unfiltered_list, maxdepth)
        if eflag != lib.PKGCONF_PKG_ERRF_OK:
            return False
        
        lib.pkgconf_fragment_filter(client, filtered_list, unfiltered_list, filter_fn, data_handle)

        if filtered_list.head == ffi.NULL:
            return True
        
        result.extend(_render_fragments(filtered_list, fmt=f"{prefix}='{{}}'"))

    finally:
        lib.pkgconf_fragment_free(unfiltered_list)
        lib.pkgconf_fragment_free(filtered_list)
        

def apply_env(client, world, env_prefix, maxdepth, result, data_handle) -> T.List[str]:
    if not all(c.isalnum() for c in env_prefix):
        return []
    
    result = []

    workbuf = f'{env_prefix}_CFLAGS'
    if not apply_env_var(workbuf, client, world, maxdepth, lib.pkgconf_pkg_cflags, lib.filter_cflags, result, data_handle):
        return []
    
    workbuf = f'{env_prefix}_LIBS'
    if not apply_env_var(workbuf, client, world, maxdepth, lib.pkgconf_pkg_libs, lib.filter_libs, result, data_handle):
        return []
    
    return result


def print_provides(pkg) -> T.List[str]:
    result = []
    for dep in NodeIter(pkg.provides, 'pkgconf_dependency_t *'):
        d = ffi.string(dep.package).decode()
        if dep.version != ffi.NULL:
            comp = ffi.string(lib.pkgconf_pkg_get_comparator(dep)).decode()
            ver = ffi.string(dep.version).decode()
            d += f' {comp} {ver}'
        result.append(d)
    return result


def apply_provides(world) -> T.List[str]:
    result = []
    for dep in NodeIter(world.required, 'pkgconf_dependency_t *'):
        result.extend(print_provides(dep.match))
    return result


def apply_modversion(world) -> T.List[str]:
    result = []
    for dep in NodeIter(world.required, 'pkgconf_dependency_t *'):
        pkg = dep.match
        if pkg.version != ffi.NULL:
            result.append(ffi.string(pkg.version).decode())
    return result


def apply_path(world) -> T.List[str]:
    result = []
    for dep in NodeIter(world.required, 'pkgconf_dependency_t *'):
        pkg = dep.match

        if pkg.filename != ffi.NULL:
            result.append(ffi.string(pkg.filename).decode())
    return result


def print_variables(pkg) -> T.List[str]:
    result = []
    for tuple_ in NodeIter(pkg.vars, 'pkgconf_tuple_t *'):
        result.append(ffi.string(tuple_.key).decode())
    return result


def apply_variables(world) -> T.List[str]:
    result = []
    for dep in NodeIter(world.required, 'pkgconf_dependency_t *'):
        result.extend(print_variables(dep.match))
    return result


def apply_variable(client, world, variable) -> T.List[str]:
    result = []
    for dep in NodeIter(world.required, 'pkgconf_dependency_t *'):
        pkg = dep.match
        var = lib.pkgconf_tuple_find(client, ffi.addressof(pkg.vars), variable.encode())
        if var != ffi.NULL:
            result.append(ffi.string(var).decode())
    return result


def print_requires(pkg) -> T.List[str]:
    result = []
    for dep in NodeIter(pkg.required, 'pkgconf_dependency_t *'):
        d = ffi.string(dep.package).decode()
        if dep.version != ffi.NULL:
            comp = ffi.string(lib.pkgconf_pkg_get_comparator(dep)).decode()
            ver = ffi.string(dep.version).decode()
            d += f' {comp} {ver}'
        result.append(d)
    return result


def apply_requires(world) -> T.List[str]:
    result = []
    for dep in NodeIter(world.required, 'pkgconf_dependency_t *'):
        result.extend(print_requires(dep.match))
    return result


def print_requires_private(pkg) -> T.List[str]:
    result = []
    for dep in NodeIter(pkg.requires_private, 'pkgconf_dependency_t *'):
        d = ffi.string(dep.package).decode()
        if dep.version != ffi.NULL:
            comp = ffi.string(lib.pkgconf_pkg_get_comparator(dep)).decode()
            ver = ffi.string(dep.version).decode()
            d += f' {comp} {ver}'
        result.append(d)
    return result


def apply_requires_private(world) -> T.List[str]:
    result = []
    for dep in NodeIter(world.required, 'pkgconf_dependency_t *'):
        result.extend(print_requires_private(dep.match))
    return result


def apply_cflags(client, world, maxdepth, data) -> T.List[str]:
    try:
        unfiltered_list = ffi.new('pkgconf_list_t*')
        filtered_list = ffi.new('pkgconf_list_t*')

        eflag = lib.pkgconf_pkg_cflags(client, world, unfiltered_list, maxdepth)
        if eflag != lib.PKGCONF_PKG_ERRF_OK:
            return []
        
        lib.pkgconf_fragment_filter(client, filtered_list, unfiltered_list, lib.filter_cflags, data)

        if filtered_list.head == ffi.NULL:
            return []
        
        return _render_fragments(filtered_list)
    
    finally:
        lib.pkgconf_fragment_free(unfiltered_list)
        lib.pkgconf_fragment_free(filtered_list)


def apply_libs(client, world, maxdepth, data) -> T.List[str]:
    try:
        unfiltered_list = ffi.new('pkgconf_list_t*')
        filtered_list = ffi.new('pkgconf_list_t*')

        eflag = lib.pkgconf_pkg_libs(client, world, unfiltered_list, maxdepth)
        if eflag != lib.PKGCONF_PKG_ERRF_OK:
            return []
        
        lib.pkgconf_fragment_filter(client, filtered_list, unfiltered_list, lib.filter_libs, data)

        if filtered_list.head == ffi.NULL:
            return []

        return _render_fragments(filtered_list)
    
    finally:
        lib.pkgconf_fragment_free(unfiltered_list)
        lib.pkgconf_fragment_free(filtered_list)


def parse_args(command: str) -> argparse.Namespace:
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False, exit_on_error=False)
    parser.add_argument('--version', dest='want_flags', const=Flags.VERSION | Flags.PRINT_ERRORS, action='append_const')
    # parser.add_argument('--about', dest='want_flags', const=Flags.ABOUT | Flags.PRINT_ERRORS, action='append_const')
    parser.add_argument('--atleast-version', dest='required_module_version')
    parser.add_argument('--atleast-pkgconfig-version', dest='required_pkgconfig_version')
    parser.add_argument('--libs', dest='want_flags', const=Flags.LIBS | Flags.PRINT_ERRORS, action='append_const')
    parser.add_argument('--cflags', dest='want_flags', const=Flags.CFLAGS | Flags.PRINT_ERRORS, action='append_const')
    parser.add_argument('--modversion', dest='want_flags', const=Flags.MODVERSION | Flags.PRINT_ERRORS, action='append_const')
    parser.add_argument('--variable', dest='want_variable')
    parser.add_argument('--exists', dest='want_flags', const=Flags.EXISTS, action='append_const')
    parser.add_argument('--print-errors', dest='want_flags', const=Flags.PRINT_ERRORS, action='append_const')
    parser.add_argument('--short-errors', dest='want_flags', const=Flags.SHORT_ERRORS, action='append_const')
    parser.add_argument('--maximum-traverse-depth', type=int, default=MAXIMUM_TRAVERSE_DEPTH)
    parser.add_argument('--static', dest='want_flags', const=Flags.STATIC, action='append_const')
    parser.add_argument('--shared', dest='want_flags', const=Flags.SHARED, action='append_const')
    parser.add_argument('--pure', dest='want_flags', const=Flags.PURE, action='append_const')
    parser.add_argument('--print-requires', dest='want_flags', const=Flags.REQUIRES, action='append_const')
    parser.add_argument('--print-variables', dest='want_flags', const=Flags.VARIABLES | Flags.PRINT_ERRORS, action='append_const')
    # parser.add_argument('--digraph', dest='want_flags', const=Flags.DIGRAPH, action='append_const')
    # parser.add_argument('--help', dest='want_flags', const=Flags.HELP, action='append_const')
    parser.add_argument('--env-only', dest='want_flags', const=Flags.ENV_ONLY, action='append_const')
    parser.add_argument('--print-requires-private', dest='want_flags', const=Flags.REQUIRES_PRIVATE, action='append_const')
    parser.add_argument('--cflags-only-I', dest='want_flags', const=Flags.CFLAGS_ONLY_I | Flags.PRINT_ERRORS, action='append_const')
    parser.add_argument('--cflags-only-other', dest='want_flags', const=Flags.CFLAGS_ONLY_OTHER | Flags.PRINT_ERRORS, action='append_const')
    parser.add_argument('--libs-only-L', dest='want_flags', const=Flags.LIBS_ONLY_LDPATH | Flags.PRINT_ERRORS, action='append_const')
    parser.add_argument('--libs-only-l', dest='want_flags', const=Flags.LIBS_ONLY_LIBNAME | Flags.PRINT_ERRORS, action='append_const')
    parser.add_argument('--libs-only-other', dest='want_flags', const=Flags.LIBS_ONLY_OTHER | Flags.PRINT_ERRORS, action='append_const')
    parser.add_argument('--uninstalled', dest='want_flags', const=Flags.UNINSTALLED, action='append_const')
    parser.add_argument('--no-uninstalled', dest='want_flags', const=Flags.NO_UNINSTALLED, action='append_const')
    parser.add_argument('--keep-system-cflags', dest='want_flags', const=Flags.KEEP_SYSTEM_CFLAGS, action='append_const')
    parser.add_argument('--keep-system-libs', dest='want_flags', const=Flags.KEEP_SYSTEM_LIBS, action='append_const')
    parser.add_argument('--define-variable', action='append')
    parser.add_argument('--exact-version', dest='required_exact_module_version')
    parser.add_argument('--max-version', dest='required_max_module_version')
    parser.add_argument('--ignore-conflicts', dest='want_flags', const=Flags.IGNORE_CONFLICTS, action='append_const')
    parser.add_argument('--errors-to-stdout', dest='want_flags', const=Flags.ERRORS_ON_STDOUT, action='append_const')
    parser.add_argument('--silence-errors', dest='want_flags', const=Flags.SILENCE_ERRORS, action='append_const')
    parser.add_argument('--list-all', dest='want_flags', const=Flags.LIST | Flags.PRINT_ERRORS, action='append_const')
    parser.add_argument('--list-package-names', dest='want_flags', const=Flags.LIST_PACKAGE_NAMES | Flags.PRINT_ERRORS, action='append_const')
    # parser.add_argument('--simulate', dest='want_flags', const=Flags.SIMULATE, action='append_const')
    parser.add_argument('--no-cache', dest='want_flags', const=Flags.NO_CACHE, action='append_const')
    parser.add_argument('--print-provides', dest='want_flags', const=Flags.PROVIDES, action='append_const')
    parser.add_argument('--no-provides', dest='want_flags', const=Flags.NO_PROVIDES, action='append_const')
    parser.add_argument('--debug', dest='want_flags', const=Flags.DEBUG | Flags.PRINT_ERRORS, action='append_const')
    parser.add_argument('--validate', dest='want_flags', const=Flags.VALIDATE | Flags.PRINT_ERRORS | Flags.ERRORS_ON_STDOUT, action='append_const')
    parser.add_argument('--log-file', dest='logfile_arg')
    parser.add_argument('--path', dest='want_flags', const=Flags.PATH, action='append_const')
    parser.add_argument('--with-path', action='append')
    parser.add_argument('--prefix-variable')
    parser.add_argument('--define-prefix', dest='want_flags', const=Flags.DEFINE_PREFIX, action='append_const')
    parser.add_argument('--relocate')
    parser.add_argument('--dont-define-prefix', dest='want_flags', const=Flags.DONT_DEFINE_PREFIX, action='append_const')
    parser.add_argument('--dont-relocate-paths', dest='want_flags', const=Flags.DONT_RELOCATE_PATHS, action='append_const')
    parser.add_argument('--env', dest='want_env_prefix')
    # parser.add_argument('--msvc-syntax', dest='want_flags', const=Flags.MSVC_SYNTAX, action='append_const')
    parser.add_argument('--fragment-filter', dest='want_fragment_filter')
    parser.add_argument('--internal-cflags', dest='want_flags', const=Flags.INTERNAL_CFLAGS, action='append_const')
    # parser.add_argument('--dump-personality', dest='want_flags', const=Flags.DUMP_PERSONALITY, action='append_const')
    # parser.add_argument('--personality')
    parser.add_argument('--license', dest='want_flags', const=Flags.DUMP_LICENSE, action='append_const')
    parser.add_argument('args', nargs='*')

    options = parser.parse_args(shlex.split(command))
    options.want_flags = reduce(operator.or_, options.want_flags or [], Flags.DEFAULT)
    if options.define_variable:
        dv = {}
        for opt in options.define_variable:
            key, value = opt.split('=', maxsplit=1)
            dv[key] = value
        options.define_variable = dv

    return options


def query(args: T.Optional[T.List[str]] = None,
          want_flags: Flags = Flags.DEFAULT,
          required_module_version: T.Optional[str] = None,
          required_pkgconfig_version: T.Optional[str] = None,
          want_variable: T.Optional[str] = None,
          define_variable: T.Optional[T.Dict[str, str]] = None,
          maximum_traverse_depth: int = MAXIMUM_TRAVERSE_DEPTH,
          required_exact_module_version: T.Optional[str] = None,
          required_max_module_version: T.Optional[str] = None,
          logfile_arg: T.Optional[str] = None,
          with_path: T.Optional[str] = None,
          prefix_variable: T.Optional[str] = None,
          relocate: T.Optional[str] = None,
          want_env_prefix: T.Optional[str] = None,
          want_fragment_filter: T.Optional[str] = None,
          ) -> T.List[str]:
    
    if relocate:
        return [relocate_path(relocate)]

    maximum_package_count = 0
    pkgq = ffi.new('pkgconf_list_t*')
    dir_list = ffi.new('pkgconf_list_t*')
    want_client_flags = lib.PKGCONF_PKG_PKGF_NONE

    world = ffi.new('pkgconf_pkg_t *')
    id = ffi.new('char[]', 'virtual:world'.encode())
    world.id = id
    realname = ffi.new('char[]', 'virtual world package'.encode())
    world.realname = realname
    world.flags = lib.PKGCONF_PKG_PROPF_STATIC | lib.PKGCONF_PKG_PROPF_VIRTUAL

    pkg_client = ffi.new('pkgconf_client_t*')

    # if os.environ.get('PKG_CONFIG_EARLY_TRACE'):

    if define_variable:
        for key, value in define_variable.items():
            lib.pkgconf_tuple_define_global(pkg_client, f'{key}={value}'.encode())
    for path in with_path or []:
        lib.pkgconf_path_add(path.encode(), dir_list, True)
    if prefix_variable:
        lib.pkgconf_client_set_prefix_varname(pkg_client, prefix_variable.encode())
        
    personality = lib.pkgconf_cross_personality_default()

    lib.pkgconf_path_copy_list(ffi.addressof(personality.dir_list), dir_list)
    lib.pkgconf_path_free(dir_list)

    error_level_handle = ffi.new_handle(logging.ERROR)
    lib.pkgconf_client_init(pkg_client, lib.error_handler, error_level_handle, personality)

    def cleanup() -> None:
        lib.pkgconf_solution_free(pkg_client, world)
        lib.pkgconf_queue_free(pkgq)
        lib.pkgconf_cross_personality_deinit(personality)
        lib.pkgconf_client_deinit(pkg_client)

        logger.handlers.clear()

    def raise_and_clean(*args) -> None:
        try:
            raise PkgConfError(*args)
        finally:
            cleanup()


    if os.environ.get('PKG_CONFIG_MAXIMUM_TRAVERSE_DEPTH') is not None:
        maximum_traverse_depth = int(os.environ.get('PKG_CONFIG_MAXIMUM_TRAVERSE_DEPTH'))
    
    if Flags.PRINT_ERRORS not in want_flags:
        want_flags |= Flags.SILENCE_ERRORS
    
    if Flags.SILENCE_ERRORS in want_flags and not os.environ.get('PKG_CONFIG_DEBUG_SPEW'):
        want_flags |= Flags.SILENCE_ERRORS
    else:
        want_flags &= ~Flags.SILENCE_ERRORS
    
    if os.environ.get('PKG_CONFIG_DONT_RELOCATE_PATHS'):
        want_flags |= Flags.DONT_RELOCATE_PATHS

    if Flags.VALIDATE in want_flags or Flags.DEBUG in want_flags:
        warning_level_handle = ffi.new_handle(logging.WARNING)
        lib.pkgconf_client_set_warn_handler(pkg_client, lib.error_handler, warning_level_handle)

    #ifndef PKGCONF_LITE
    if Flags.DEBUG in want_flags:
        set_stream_handler(sys.stderr)
        debug_level_handle = ffi.new_handle(logging.DEBUG)
        lib.pkgconf_client_set_trace_handler(pkg_client, lib.error_handler, debug_level_handle)
    #endif

    # if Flags.ABOUT in want_flags

    if Flags.VERSION in want_flags:
        try:
            return [version()]
        finally:
            cleanup()

    # if Flags.HELP in want_flags

    if os.environ.get('PKG_CONFIG_FDO_SYSROOT_RULES'):
        want_client_flags |= lib.PKGCONF_PKG_PKGF_FDO_SYSROOT_RULES

    if os.environ.get('PKG_CONFIG_PKGCONF1_SYSROOT_RULES'):
        want_client_flags |= lib.PKGCONF_PKG_PKGF_PKGCONF1_SYSROOT_RULES

    if Flags.SHORT_ERRORS in want_flags:
        want_client_flags |= lib.PKGCONF_PKG_PKGF_SIMPLIFY_ERRORS
    
    if Flags.DONT_RELOCATE_PATHS in want_flags:
        want_client_flags |= lib.PKGCONF_PKG_PKGF_DONT_RELOCATE_PATHS
    
    if Flags.SILENCE_ERRORS in want_flags:
        set_stream_handler(None)
    elif Flags.ERRORS_ON_STDOUT in want_flags:
        set_stream_handler(sys.stdout)
    else:
        set_stream_handler(sys.stderr)

    if Flags.IGNORE_CONFLICTS in want_flags or os.environ.get('PKG_CONFIG_IGNORE_CONFLICTS'):
        want_client_flags |= lib.PKGCONF_PKG_PKGF_SKIP_CONFLICTS
    
    if Flags.STATIC in want_flags or personality.want_default_static:
        want_client_flags |= lib.PKGCONF_PKG_PKGF_SEARCH_PRIVATE | lib.PKGCONF_PKG_PKGF_MERGE_PRIVATE_FRAGMENTS
    
    if Flags.SHARED in want_flags:
        want_client_flags &= ~(lib.PKGCONF_PKG_PKGF_SEARCH_PRIVATE | lib.PKGCONF_PKG_PKGF_MERGE_PRIVATE_FRAGMENTS)

    if Flags.PURE in want_flags or os.environ.get('PKG_CONFIG_PURE_DEPGRAPH') is not None or personality.want_default_pure:
        want_client_flags &= ~lib.PKGCONF_PKG_PKGF_MERGE_PRIVATE_FRAGMENTS
    
    if Flags.ENV_ONLY in want_flags:
        want_client_flags |= lib.PKGCONF_PKG_PKGF_ENV_ONLY
    
    if Flags.NO_CACHE in want_flags:
        want_client_flags |= lib.PKGCONF_PKG_PKGF_NO_CACHE
    
    # FIXME: only win32?
    if sys.platform in {'win32', 'cygwin', 'msys'} or Flags.DEFINE_PREFIX in want_flags:
        want_client_flags |= lib.PKGCONF_PKG_PKGF_REDEFINE_PREFIX
    
    if Flags.NO_UNINSTALLED in want_flags or os.environ.get('PKG_CONFIG_DISABLE_UNINSTALLED') is not None:
        want_client_flags |= lib.PKGCONF_PKG_PKGF_NO_UNINSTALLED
    
    if Flags.NO_PROVIDES in want_flags:
        want_client_flags |= lib.PKGCONF_PKG_PKGF_SKIP_PROVIDES
    
    if Flags.DONT_DEFINE_PREFIX in want_flags or os.environ.get('PKG_CONFIG_DONT_DEFINE_PREFIX') is not None:
        want_client_flags &= ~lib.PKGCONF_PKG_PKGF_REDEFINE_PREFIX

    if Flags.INTERNAL_CFLAGS in want_flags:
        want_client_flags |= lib.PKGCONF_PKG_PKGF_DONT_FILTER_INTERNAL_CFLAGS
    
    if (Flags.REQUIRES in want_flags
            or Flags.REQUIRES_PRIVATE in want_flags
            or Flags.PROVIDES in want_flags
            or Flags.VARIABLES in want_flags
            or Flags.MODVERSION in want_flags
            or Flags.PATH in want_flags
            or want_variable):
        maximum_package_count = 1
        maximum_traverse_depth = 1
    
    if os.environ.get('PKG_CONFIG_ALLOW_SYSTEM_CFLAGS') is not None:
        want_flags |= Flags.KEEP_SYSTEM_CFLAGS
    
    if os.environ.get('PKG_CONFIG_ALLOW_SYSTEM_LIBS') is not None:
        want_flags |= Flags.KEEP_SYSTEM_LIBS
    
    builddir = os.environ.get('PKG_CONFIG_TOP_BUILD_DIR')
    if builddir is not None:
        lib.pkgconf_client_set_buildroot_dir(pkg_client, builddir.encode())
    
    if Flags.REQUIRES_PRIVATE in want_flags or want_flags & Flags.CFLAGS:
        want_client_flags |= lib.PKGCONF_PKG_PKGF_SEARCH_PRIVATE
    
    sysroot_dir = os.environ.get('PKG_CONFIG_SYSROOT_DIR')
    if sysroot_dir is not None:
        lib.pkgconf_client_set_sysroot_dir(pkg_client, sysroot_dir.encode())

        destdir = os.environ.get('DESTDIR')
        if destdir == sysroot_dir:
            want_client_flags |= lib.PKGCONF_PKG_PKGF_FDO_SYSROOT_RULES
    
    lib.pkgconf_client_set_flags(pkg_client, want_client_flags)
    lib.pkgconf_client_dir_list_build(pkg_client, personality)

    if required_pkgconfig_version is not None:
        if lib.pkgconf_compare_version(lib.PACKAGE_VERSION, required_pkgconfig_version.encode()) >= 0:
            cleanup()
            return []
        raise_and_clean()

    result = []
    result_handle = ffi.new_handle(result)

    if Flags.LIST in want_flags:
        lib.pkgconf_scan_all(pkg_client, result_handle, lib.print_list_entry)
        cleanup()
        return result
    
    if Flags.LIST_PACKAGE_NAMES in want_flags:
        lib.pkgconf_scan_all(pkg_client, result_handle, lib.print_package_entry)
        cleanup()
        return result
    
    if logfile_arg is not None:
        set_file_handler(logfile_arg)

    # Group the 3 version compare, as they are very similar
    module_version_key = None
    module_version_cmp = None

    if required_module_version is not None:
        module_version_key = required_module_version
        module_version_cmp = operator.ge
    elif required_exact_module_version is not None:
        module_version_key = required_exact_module_version
        module_version_cmp = operator.eq
    elif required_max_module_version is not None:
        module_version_key = required_max_module_version
        module_version_cmp = operator.le

    if module_version_key: 
        try:
            deplist = ffi.new('pkgconf_list_t*')

            for arg in args:
                lib.pkgconf_dependency_parse_str(pkg_client, deplist, arg.encode(), 0)

            pkg = ffi.NULL
            for pkgiter in NodeIter(deplist, 'pkgconf_dependency_t *'):
                pkg = lib.pkgconf_pkg_find(pkg_client, pkgiter.package)
                if pkg == ffi.NULL:
                    if Flags.PRINT_ERRORS in want_flags:
                        lib.pkgconf_error(pkg_client, b"Package '%s' was not found\n", pkgiter.package)

                    raise PkgConfError()
                
                if module_version_cmp(lib.pkgconf_compare_version(pkg.version, module_version_key.encode()), 0):
                    return []
                
            raise PkgConfError()

        finally:
            if pkg != ffi.NULL:
                lib.pkgconf_pkg_unref(pkg_client, pkg)
            lib.pkgconf_dependency_free(deplist)
            cleanup()

    while args:
        package = args.pop(0)
        
        if maximum_package_count > 0 and pkgq.length >= maximum_package_count:
            break
        
        # FIXME: probably unneeded
        package.lstrip()
        if not package:
            continue

        if args and args[0] in OPERATORS:
            package = f'{package} {args[0]}, {args[1]}'
            args = args[2:]
        lib.pkgconf_queue_push(pkgq, package.encode())

    if pkgq.head == ffi.NULL:
        raise_and_clean("Please specify at least one package name.")
    
    if not lib.pkgconf_queue_solve(pkg_client, pkgq, world, maximum_traverse_depth):
        raise_and_clean()
    
    if Flags.VALIDATE in want_flags:
        cleanup()
        return []
        
    if Flags.DUMP_LICENSE in want_flags:
        license = apply_license(pkg_client, world, 2)
        cleanup()
        return license

    if Flags.UNINSTALLED in want_flags:
        ret = [False]
        apply_uninstalled(pkg_client, world, ret, 2)
        if ret[0] is False:
            raise_and_clean()

        cleanup()
        return []
    
    if want_env_prefix is not None:
        data = (want_flags, want_fragment_filter)
        data_handle = ffi.new_handle(data)
        result.extend(apply_env(pkg_client, world, want_env_prefix, 2, data_handle))
        want_flags = 0
    
    if Flags.PROVIDES in want_flags:
        want_flags &= ~(Flags.CFLAGS | Flags.LIBS)
        result.extend(apply_provides(world))

    if Flags.MODVERSION in want_flags:
        want_flags &= ~(Flags.CFLAGS | Flags.LIBS)
        result.extend(apply_modversion(world))

    if Flags.PATH in want_flags:
        want_flags &= ~(Flags.CFLAGS | Flags.LIBS)
        lib.pkgconf_client_set_flags(pkg_client, want_client_flags | lib.PKGCONF_PKG_PKGF_SKIP_ROOT_VIRTUAL)
        result.extend(apply_path(world))
    
    if Flags.VARIABLES in want_flags:
        want_flags &= ~(Flags.CFLAGS | Flags.LIBS)
        result.extend(apply_variables(world))

    if want_variable:
        want_flags &= ~(Flags.CFLAGS | Flags.LIBS)
        lib.pkgconf_client_set_flags(pkg_client, want_client_flags | lib.PKGCONF_PKG_PKGF_SKIP_ROOT_VIRTUAL)
        result.extend(apply_variable(pkg_client, world, want_variable))
    
    if Flags.REQUIRES in want_flags:
        want_flags &= ~(Flags.CFLAGS | Flags.LIBS)
        result.extend(apply_requires(world))
    
    if Flags.REQUIRES_PRIVATE in want_flags:
        want_flags &= ~(Flags.CFLAGS | Flags.LIBS)
        result.extend(apply_requires_private(world))
    
    if Flags.CFLAGS & want_flags:
        data = (want_flags, want_fragment_filter)
        data_handle = ffi.new_handle(data)
        result.extend(apply_cflags(pkg_client, world, 2, data_handle))
    
    if Flags.LIBS & want_flags:
        if Flags.STATIC not in want_flags:
            lib.pkgconf_client_set_flags(pkg_client, pkg_client.flags & ~lib.PKGCONF_PKG_PKGF_SEARCH_PRIVATE)

            # redo the solution for the library set: free the solution itself, and any cached graph nodes
            lib.pkgconf_solution_free(pkg_client, world)
            lib.pkgconf_cache_free(pkg_client)

            if not lib.pkgconf_queue_solve(pkg_client, pkgq, world, maximum_traverse_depth):
                raise_and_clean()
        
        data = (want_flags, want_fragment_filter)
        data_handle = ffi.new_handle(data)
        result.extend(apply_libs(pkg_client, world, 2, data_handle))
    
    cleanup()
    return result
        

def query_args(command: str) -> T.List[str]:
    try:
        options = parse_args(command)
        return query(**vars(options))
    
    except SystemExit:
        raise PkgConfError()
