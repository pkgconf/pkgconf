import pypkgconf

import pytest

with_lib1 = pytest.mark.usefixtures('lib1_pkg_config_path')


@with_lib1
def test_do_not_eat_slash(monkeypatch):
    monkeypatch.setenv('PKG_CONFIG_SYSROOT_DIR', '/')

    result = pypkgconf.query_args('--cflags baz')
    assert result == ['-fPIC', '-I/test/include/foo']


@with_lib1
def test_cflags(monkeypatch, testsdir):
    selfdir = testsdir.as_posix()
    monkeypatch.setenv('PKG_CONFIG_SYSROOT_DIR', selfdir)

    result = pypkgconf.query_args('--cflags baz')
    assert result == ['-fPIC', f'-I{selfdir}/test/include/foo']


@with_lib1
def test_variable(monkeypatch, testsdir):
    selfdir = testsdir.as_posix()
    monkeypatch.setenv('PKG_CONFIG_SYSROOT_DIR', selfdir)

    result = pypkgconf.query_args('--variable=prefix foo')
    assert result == [f'{selfdir}/test']

    result = pypkgconf.query_args('--variable=includedir foo')
    assert result == [f'{selfdir}/test/include']


@with_lib1
def test_do_not_duplicate_sysroot_dir(monkeypatch, testsdir):
    monkeypatch.setenv('PKG_CONFIG_SYSROOT_DIR', '/sysroot')

    result = pypkgconf.query_args('--cflags sysroot-dir-2')
    assert result == ['-I/sysroot/usr/include']

    result = pypkgconf.query_args('--cflags sysroot-dir-3')
    assert result == ['-I/sysroot/usr/include']

    result = pypkgconf.query_args('--cflags sysroot-dir-5')
    assert result == ['-I/sysroot/usr/include']

    selfdir = testsdir.as_posix()
    monkeypatch.setenv('PKG_CONFIG_SYSROOT_DIR', selfdir)
    result = pypkgconf.query_args('--cflags sysroot-dir-4')
    assert result == [f'-I{selfdir}/usr/include']


@with_lib1
def test_uninstalled(monkeypatch):
    monkeypatch.setenv('PKG_CONFIG_SYSROOT_DIR', '/sysroot')

    result = pypkgconf.query_args('--libs omg')
    assert result == ['-L/test/lib', '-lomg']


@with_lib1
def test_uninstalled_pkgconf1(monkeypatch):
    monkeypatch.setenv('PKG_CONFIG_SYSROOT_DIR', '/sysroot')
    monkeypatch.setenv('PKG_CONFIG_PKGCONF1_SYSROOT_RULES', '1')

    result = pypkgconf.query_args('--libs omg')
    assert result == ['-L/sysroot/test/lib', '-lomg']


@with_lib1
def test_uninstalled_fdo(monkeypatch):
    monkeypatch.setenv('PKG_CONFIG_SYSROOT_DIR', '/sysroot')
    monkeypatch.setenv('PKG_CONFIG_FDO_SYSROOT_RULES', '1')

    result = pypkgconf.query_args('--libs omg')
    assert result == ['-L/test/lib', '-lomg']


@with_lib1
def test_uninstalled_fdo_pc_sysrootdir(monkeypatch):
    monkeypatch.setenv('PKG_CONFIG_SYSROOT_DIR', '/sysroot')
    monkeypatch.setenv('PKG_CONFIG_FDO_SYSROOT_RULES', '1')

    result = pypkgconf.query_args('--libs omg-sysroot')
    assert result == ['-L/sysroot/test/lib', '-lomg']
