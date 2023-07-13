import pypkgconf

import pytest

import logging
import os
import sys


with_lib1 = pytest.mark.usefixtures('lib1_pkg_config_path')
with_lib2 = pytest.mark.usefixtures('lib2_pkg_config_path')


@with_lib1
def test_case_sensitivity():
    assert pypkgconf.query_args('--variable=foo case-sensitivity') == ['3']
    assert pypkgconf.query_args('--variable=Foo case-sensitivity') == ['4']


@with_lib1
def test_depgraph_break_1():
    with pytest.raises(pypkgconf.PkgConfError):
        pypkgconf.query_args('--exists --print-errors "foo > 0.6.0 foo < 0.8.0"')


@with_lib1
def test_depgraph_break_2():
    with pytest.raises(pypkgconf.PkgConfError):
        pypkgconf.query_args('--exists --print-errors "nonexisting foo <= 3"')


@with_lib1
def test_depgraph_break_3():
    with pytest.raises(pypkgconf.PkgConfError):
        pypkgconf.query_args('--exists --print-errors "depgraph-break"')


@with_lib1
def test_define_variable():
    result = pypkgconf.query_args('--variable=typelibdir --define-variable="libdir=\\${libdir}" typelibdir')
    assert result == ['\\${libdir}/typelibdir']


@with_lib1
def test_define_variable_override():
    result = pypkgconf.query_args('--variable=prefix --define-variable="prefix=/test" typelibdir')
    assert result == ['/test']


@with_lib1
def test_variable():
    result = pypkgconf.query_args('--variable=includedir foo')
    assert result == ['/test/include']


@with_lib1
def test_keep_system_libs(monkeypatch):
    monkeypatch.setenv('LIBRARY_PATH', '/test/local/lib')

    assert pypkgconf.query_args('--libs-only-L cflags-libs-only') == []

    result = pypkgconf.query_args('--libs-only-L --keep-system-libs cflags-libs-only')
    assert result == ['-L/test/local/lib']


@with_lib1
def test_libs():
    result = pypkgconf.query_args('--libs cflags-libs-only')
    assert result == ['-L/test/local/lib', '-lfoo']


@with_lib1
def test_libs_only():
    result = pypkgconf.query_args('--libs-only-L --libs-only-l cflags-libs-only')
    assert result == ['-L/test/local/lib', '-lfoo']


@pytest.mark.xfail
@with_lib1
def test_libs_never_mergeback():
    result = pypkgconf.query_args('--libs prefix-foo1')
    assert result == ['-L/test/bar/lib', '-lfoo1']

    result = pypkgconf.query_args('--libs prefix-foo1 prefix-foo2')
    assert result == ['-L/test/bar/lib', '-lfoo1', '-lfoo2']


@with_lib1
def test_cflags_only():
    result = pypkgconf.query_args('--cflags-only-I --cflags-only-other cflags-libs-only')
    assert result == ['-I/test/local/include/foo']


@pytest.mark.xfail
@with_lib1
def test_cflags_never_mergeback():
    result = pypkgconf.query_args('--cflags prefix-foo1 prefix-foo2')
    assert result == ['-I/test/bar/include/foo', '-DBAR', '-fPIC', '-DFOO']


@with_lib1
def test_incomplete_libs():
    result = pypkgconf.query_args('--libs incomplete')
    assert result == []


@with_lib1
def test_incomplete_cflags():
    result = pypkgconf.query_args('--cflags incomplete')
    assert result == []


@with_lib1
def test_isystem_munge_order():
    result = pypkgconf.query_args('--cflags isystem')
    assert result == ['-isystem', '/opt/bad/include', '-isystem', '/opt/bad2/include']


@with_lib1
def test_isystem_munge_sysroot(monkeypatch, testsdir):
    monkeypatch.setenv('PKG_CONFIG_SYSROOT_DIR', testsdir.as_posix())

    result = pypkgconf.query_args('--cflags isystem')

    assert f'-isystem {testsdir.as_posix()}/opt/bad/include' in ' '.join(result)


@with_lib1
def test_idirafter_munge_order():
    result = pypkgconf.query_args('--cflags idirafter')
    assert result == ['-idirafter', '/opt/bad/include', '-idirafter', '/opt/bad2/include']


@with_lib1
def test_idirafter_munge_sysroot(monkeypatch, testsdir):
    monkeypatch.setenv('PKG_CONFIG_SYSROOT_DIR', testsdir.as_posix())

    result = pypkgconf.query_args('--cflags idirafter')
    assert f'-idirafter {testsdir.as_posix()}/opt/bad/include' in ' '.join(result)


@with_lib1
def test_idirafter_ordering():
    result = pypkgconf.query_args('--cflags idirafter-ordering')
    assert result == ['-I/opt/bad/include1', '-idirafter', '-I/opt/bad/include2', '-I/opt/bad/include3']


@with_lib2
def test_pcpath(testsdir):
    selfdir = testsdir.as_posix()

    result = pypkgconf.query_args(f'--cflags {selfdir}/lib3/bar.pc')
    assert result == ['-fPIC', '-I/test/include/foo']


def test_sysroot_munge(tmp_path, testsdir, monkeypatch):
    contents = (testsdir / 'lib1' / 'sysroot-dir.pc').read_text(encoding='utf-8')
    contents = contents.replace('/sysroot/', testsdir.as_posix() + '/')

    (tmp_path / 'lib1').mkdir()
    (tmp_path / 'lib1' / 'sysroot-dir-selfdir.pc').write_text(contents, encoding='utf-8')

    monkeypatch.setenv('PKG_CONFIG_PATH', (tmp_path / 'lib1').as_posix())
    monkeypatch.setenv('PKG_CONFIG_SYSROOT_DIR', testsdir.as_posix())

    result = pypkgconf.query_args('--libs sysroot-dir-selfdir')
    assert result == [f'-L{testsdir.as_posix()}/lib', '-lfoo']


def test_virtual_variable():
    assert pypkgconf.query_args('--exists pkg-config') == []
    assert pypkgconf.query_args('--exists pkgconf') == []

    if sys.platform in {'win32', 'cygwin', 'msys'}:
        pcpath = '../lib/pkgconfig;../share/pkgconfig'
    else:
        pcpath = os.environ.get('PKG_DEFAULT_PATH')
    result = pypkgconf.query_args('--variable=pc_path pkg-config')
    assert result == [pcpath]

    result = pypkgconf.query_args('--variable=pc_path pkgconf')
    assert result == [pcpath]


@pytest.mark.xfail
def test_fragment_collision(testsdir):
    selfdir = testsdir.as_posix()

    result = pypkgconf.query_args(f'--with-path="{selfdir}/lib1" --cflags fragment-collision')
    assert result == ['-D_BAZ', '-D_BAR', '-D_FOO', '-D_THREAD_SAFE', '-pthread']


def test_malformed_1(testsdir):
    selfdir = testsdir.as_posix()

    with pytest.raises(pypkgconf.PkgConfError):
        pypkgconf.query_args(f'--validate --with-path="{selfdir}/lib1" malformed-1')


def test_malformed_quoting(testsdir):
    selfdir = testsdir.as_posix()

    assert pypkgconf.query_args(f'--validate --with-path="{selfdir}/lib1" malformed-quoting') == []


def test_explicit_sysroot(monkeypatch, testsdir):
    # FIXME: does not work with drive letter...
    selfdir = testsdir.as_posix().removeprefix(testsdir.drive)
    monkeypatch.setenv('PKG_CONFIG_SYSROOT_DIR', selfdir)

    result = pypkgconf.query_args(f'--debug --with-path="{selfdir}/lib1" --variable=pkgdatadir explicit-sysroot')
    assert result == [f'{selfdir}/usr/share/test']


def test_empty_tuple(testsdir):
    selfdir = testsdir.as_posix()

    assert pypkgconf.query_args(f'--with-path="{selfdir}/lib1" --cflags empty-tuple') == []


@pytest.mark.xfail
def test_solver_requires_private_debounce(testsdir):
    selfdir = testsdir.as_posix()

    result = pypkgconf.query_args(f'--with-path="{selfdir}/lib1" --cflags --libs metapackage')
    assert result == ['-I/metapackage-1', '-I/metapackage-2', '-lmetapackage-1', '-lmetapackage-2']
    

def test_solver_requires_private_debounce(testsdir, caplog):
    selfdir = testsdir.as_posix()

    with caplog.at_level(logging.WARNING):
        assert pypkgconf.query_args(f'--with-path="{selfdir}/lib1" --validate billion-laughs') == []

    s = 0
    for r in caplog.records:
        if r.levelno == logging.WARNING and r.msg == 'warning: truncating very long variable to 64KB':
            s += 1
    assert s == 5


def test_maximum_package_depth_off_by_one(testsdir):
    selfdir = testsdir.as_posix()

    result = pypkgconf.query_args(f'--with-path="{selfdir}/lib1" --modversion foo bar baz')
    assert result == ['1.2.3']
