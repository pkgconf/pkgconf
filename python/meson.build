pymod = import('python')
py = pymod.find_installation(modules: ['cffi'], pure: false)

libpkgconf_c = '_libpkgconf.c'

c_bindings = custom_target(
    'libpkgconf_c',
    command: [py, meson.current_source_dir() / 'build.py', '@INPUT@', '@OUTPUT@'],
    input: [ meson.global_source_root() / 'libpkgconf' / 'libpkgconf.h'],
    output: [libpkgconf_c],
    depend_files: ['build.py'],
)

subdir('pypkgconf')


pytest = find_program('pytest', dirs: py.get_path('scripts'), required: false, disabler: true)
if pytest.found()
    pytest_env = environment(
        {
            'PYTHONPATH': meson.current_build_dir(),
            'PKG_DEFAULT_PATH': cdata.get_unquoted('PKG_DEFAULT_PATH')
        }
    )

    test(
        'pypkgconf',
        pytest,
        args: [
            # '--runxfail',
            '-vv',
            # '-o log_cli=true', '--log-cli-level=DEBUG',
            meson.current_source_dir() / 'tests'],
        env: pytest_env,
        depends: test_python_files,
    )
else
    message('Skipping Python tests as pytest was not found.')
endif
