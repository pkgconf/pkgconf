project('pkgconf', 'c',
  version : '1.8.0',
  license : 'ISC',
  meson_version : '>=0.47')


cc = meson.get_compiler('c')


cdata = configuration_data()

check_functions = [
  ['HAVE_STRLCAT', 'strlcat', 'string.h'],
  ['HAVE_STRLCPY', 'strlcpy', 'string.h'],
  ['HAVE_STRNDUP', 'strndup', 'string.h'],
]

foreach f : check_functions
  if cc.has_function(f.get(1), prefix : '#include <' + f.get(2) + '>') and cc.has_header_symbol(f.get(2), f.get(1))
    cdata.set(f.get(0), 1)
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
cdata.set('abs_top_srcdir', meson.source_root())
cdata.set('abs_top_builddir', meson.build_root())


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
  version : '3.0.0',
  soversion : '3',
)

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
  test('kyua', kyua_exe, args : ['--config=none', 'test', '--kyuafile=' + join_paths(meson.build_root(), 'Kyuafile'), '--build-root=' + meson.build_root()])


  configure_file(input : 'Kyuafile.in', output : 'Kyuafile', configuration : cdata)
  subdir('tests')
endif

install_man('man/pkgconf.1')
install_man('man/pkg.m4.7')
install_man('man/pc.5')
install_man('man/pkgconf-personality.5')
install_data('pkg.m4', install_dir: 'share/aclocal')
install_data('AUTHORS', install_dir: 'share/doc/pkgconf')
install_data('README.md', install_dir: 'share/doc/pkgconf')