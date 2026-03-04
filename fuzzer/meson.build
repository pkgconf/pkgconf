parser_fuzzer_exe = executable(
  'parser-fuzzer',
  'parser-fuzzer.c',
  dependencies: dep_libpkgconf,
  include_directories: include_directories('..'),
  install: false,
  c_args: ['-fsanitize=fuzzer'],
  link_args: ['-fsanitize=fuzzer'],
)

run_target(
  'fuzz-parser',
  command: [parser_fuzzer_exe, '@0@/tests/lib1'.format(meson.project_source_root())]
)
