configure_file(input: 'Kyuafile.in', output: 'Kyuafile', configuration: cdata)
configure_file(input: 'test_env.sh.in', output: 'test_env.sh', configuration: cdata)


tests = [
  'conflicts',
  'parser',
  'personality',
  'regress',
  'requires',
  'spdxtool',
  'symlink',
  'version',
]


# yuck
foreach test : tests
  test_file = configure_file(input: test + '.sh', output: test, copy: true)
endforeach
