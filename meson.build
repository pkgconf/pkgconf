project('pkgconf', 'c',
  version : '1.9.3',
  license : 'ISC',
  meson_version : '>=0.47',
  default_options : ['c_std=c99'],
)

cc = meson.get_compiler('c')

add_project_arguments(
  '-D_BSD_SOURCE',
  '-D_DEFAULT_SOURCE',
  cc.get_supported_arguments(
    '-Wimplicit-function-declaration',
    '-Wmisleading-indentation',
  ),
  language : 'c',
)

cdata = configuration_data()

check_functions = [
  ['strlcat', 'string.h'],
  ['strlcpy', 'string.h'],
  ['strndup', 'string.h'],
  ['strdup', 'string.h'],
  ['strncasecmp', 'strings.h'],
  ['strcasecmp', 'strings.h'],
  ['reallocarray', 'stdlib.h'],
]

foreach f : check_functions
  if cc.has_function(f[0], prefix : '#define _BSD_SOURCE\n#include <@0@>'.format(f[1])) and cc.has_header_symbol(f[1], f[0], prefix : '#define _BSD_SOURCE')
    cdata.set('HAVE_@0@'.format(f[0].to_upper().underscorify()), 1)
  endif
endforeach

default_path = []
foreach f : ['libdir', 'datadir']
  default_path += [join_paths(get_option('prefix'), get_option(f), 'pkgconfig')]
endforeach

personality_path = []
foreach f : ['libdir', 'datadir']
  personality_path += [join_paths(get_option('prefix'), get_option(f), 'pkgconfig', 'personality.d')]
endforeach

cdata.set_quoted('SYSTEM_LIBDIR', join_paths(get_option('prefix'), get_option('libdir')))
cdata.set_quoted('SYSTEM_INCLUDEDIR', join_paths(get_option('prefix'), get_option('includedir')))
cdata.set_quoted('PKG_DEFAULT_PATH', ':'.join(default_path))
cdata.set_quoted('PERSONALITY_PATH', ':'.join(personality_path))
cdata.set_quoted('PACKAGE_NAME', meson.project_name())
cdata.set_quoted('PACKAGE_VERSION', meson.project_version())
cdata.set_quoted('PACKAGE_BUGREPORT', 'https://todo.sr.ht/~kaniini/pkgconf')
cdata.set('abs_top_srcdir', meson.current_source_dir())
cdata.set('abs_top_builddir', meson.current_build_dir())


subdir('libpkgconf')

libtype = get_option('default_library')
if libtype == 'static'
  build_static = '-DPKGCONFIG_IS_STATIC'
else
  build_static = '-DPKGCONFIG_IS_NOT_STATIC'
endif

libpkgconf = library('pkgconf',
  'libpkgconf/argvsplit.c',
  'libpkgconf/audit.c',
  'libpkgconf/bsdstubs.c',
  'libpkgconf/cache.c',
  'libpkgconf/client.c',
  'libpkgconf/dependency.c',
  'libpkgconf/fileio.c',
  'libpkgconf/fragment.c',
  'libpkgconf/parser.c',
  'libpkgconf/path.c',
  'libpkgconf/personality.c',
  'libpkgconf/pkg.c',
  'libpkgconf/queue.c',
  'libpkgconf/tuple.c',
  c_args: ['-DLIBPKGCONF_EXPORT', build_static],
  install : true,
  version : '4.0.0',
  soversion : '4',
)

# For other projects using libpkgconfig as a subproject
dep_libpkgconf = declare_dependency(
  link_with : libpkgconf,
  include_directories : include_directories('.'),
)

# If we have a new enough meson override the dependency so that only
# `dependency('libpkgconf')` is required from the consumer
if meson.version().version_compare('>= 0.54.0')
  meson.override_dependency('libpkgconf', dep_libpkgconf)
endif

pkg = import('pkgconfig')
pkg.generate(libpkgconf,
  name : 'libpkgconf',
  description : 'a library for accessing and manipulating development framework configuration',
  url: 'http://github.com/pkgconf/pkgconf',
  filebase : 'libpkgconf',
  subdirs: ['pkgconf'],
  extra_cflags : build_static
)


pkgconf_exe = executable('pkgconf',
  'cli/main.c',
  'cli/getopt_long.c',
  'cli/renderer-msvc.c',
  link_with : libpkgconf,
  c_args: build_static,
  install : true)

if get_option('tests')
  kyua_exe = find_program('kyua')
  atf_sh_exe = find_program('atf-sh')
  kyuafile = configure_file(input : 'Kyuafile.in', output : 'Kyuafile', configuration : cdata)
  test('kyua', kyua_exe, args : ['--config=none', 'test', '--kyuafile', kyuafile, '--build-root', meson.current_build_dir()])
  subdir('tests')
endif

install_man('man/pkgconf.1')
install_man('man/pkg.m4.7')
install_man('man/pc.5')
install_man('man/pkgconf-personality.5')
install_data('pkg.m4', install_dir: 'share/aclocal')
install_data('AUTHORS', install_dir: 'share/doc/pkgconf')
install_data('README.md', install_dir: 'share/doc/pkgconf')
