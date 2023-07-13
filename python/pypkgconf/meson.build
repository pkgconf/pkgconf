# This subdir is only to ensure the extension module is generated
# inside a directory whose name matches the final installation dir
# to allow in-tree testing

package_name = 'pypkgconf'
python_files = ['__init__.py', 'callbacks.py', 'clilike.py', 'constants.py', 'logger.py', 'util.py']

test_python_files = []
foreach f: python_files
    test_python_files += fs.copyfile(f)
endforeach

test_python_files += py.extension_module(
    '_libpkgconf',
    [c_bindings],
    dependencies: [dep_libpkgconf],
    c_args: [
        '-D_CFFI_NO_LIMITED_API',  # workaround until meson support it
    ],
    install: true,
    subdir: package_name,
)

py.install_sources(
    python_files,
    subdir: package_name
)

if get_option('default_library') != 'static' and host_machine.system() == 'windows'

    error('dynamic linking is not supported yet on Windows. Please use -Ddefault_library=static .')

    # On Windows, the pkgconf dll must be side-by-side with the bindings
    # to be found when testing

    # However, this doesn't work yet, because it needs: https://github.com/mesonbuild/meson/pull/12044
    # test_python_files += fs.copyfile(
    #     libpkgconf,
    #     install: true,
    #     install_tag: 'python-runtime',
    #     install_dir: py.get_install_dir() / package_name,
    # )
endif
