import pypkgconf

import pytest

import shutil


with_lib1 = pytest.mark.usefixtures('lib1_pkg_config_path')
with_lib1lib2 = pytest.mark.usefixtures('lib1lib2_pkg_config_path')


@with_lib1
def test_noargs():
    with pytest.raises(pypkgconf.PkgConfError):
        pypkgconf.query_args('')


@with_lib1
def test_libs():
    result = pypkgconf.query_args('--libs foo')

    assert result == ['-L/test/lib', '-lfoo']


@with_lib1
def test_libs_cflags():
    result = pypkgconf.query_args('--cflags --libs foo')

    assert result == ['-fPIC', '-I/test/include/foo', '-L/test/lib', '-lfoo']


@with_lib1
def test_libs_cflags_version():
    result = pypkgconf.query_args('--cflags --libs "foo > 1.2"')

    assert result == ['-fPIC', '-I/test/include/foo', '-L/test/lib', '-lfoo']


@with_lib1
def test_libs_cflags_version_multiple():
    result = pypkgconf.query_args('--cflags --libs "foo > 1.2 bar >= 1.3"')

    assert result == ['-fPIC', '-I/test/include/foo', '-L/test/lib', '-lbar', '-lfoo']


@with_lib1
def test_libs_cflags_version_multiple_coma():
    result = pypkgconf.query_args('--cflags --libs "foo > 1.2,bar >= 1.3"')

    assert result == ['-fPIC', '-I/test/include/foo', '-L/test/lib', '-lbar', '-lfoo']


@with_lib1
def test_libs_cflags_version_alt():
    result = pypkgconf.query_args('--cflags --libs "foo" ">" "1.2"')

    assert result == ['-fPIC', '-I/test/include/foo', '-L/test/lib', '-lfoo']


@with_lib1
def test_libs_cflags_version_different():
    result = pypkgconf.query_args('--cflags --libs "foo" "!=" "1.3.0"')

    assert result == ['-fPIC', '-I/test/include/foo', '-L/test/lib', '-lfoo']


@with_lib1
def test_libs_cflags_version_different_bad(caplog):
    with pytest.raises(pypkgconf.PkgConfError):
        pypkgconf.query_args('--cflags --libs "foo" "!=" "1.2.3"')

    assert "Package dependency requirement 'foo != 1.2.3' could not be satisfied.\n" in caplog.text
    assert "Package 'foo' has version '1.2.3', required version is '!= 1.2.3'\n" in caplog.text


@with_lib1
def test_exists_nonexitent():
    with pytest.raises(pypkgconf.PkgConfError):
        pypkgconf.query_args('--exists nonexistant')


@with_lib1
def test_nonexitent():
    with pytest.raises(pypkgconf.PkgConfError):
        pypkgconf.query_args('nonexistant')


@with_lib1
def test_exists_version():
    assert pypkgconf.query_args('--exists "foo > 1.2"') == []


@with_lib1
def test_exists_version_bad():
    with pytest.raises(pypkgconf.PkgConfError):
        pypkgconf.query_args('--exists "foo > 1.2.3"')


@with_lib1
def test_exists_version_bad():
    assert pypkgconf.query_args('--exists "foo" ">" "1.2"') == []


@with_lib1
def test_uninstalled_bad():
    with pytest.raises(pypkgconf.PkgConfError):
        pypkgconf.query_args('--uninstalled "foo"')


@with_lib1
def test_uninstalled():
    assert pypkgconf.query_args('--uninstalled "omg"') == []


@with_lib1
def test_exists_version_bad2():
    with pytest.raises(pypkgconf.PkgConfError):
        pypkgconf.query_args('--exists "foo >= "')


@with_lib1
def test_exists_version_bad3():
    with pytest.raises(pypkgconf.PkgConfError):
        pypkgconf.query_args('--exists "tilde >= 1.0.0"')


@with_lib1
def test_exists():
    assert pypkgconf.query_args('--exists "tilde = 1.0.0~rc1"') == []


@with_lib1
def test_exists2():
    assert pypkgconf.query_args('--exists "tilde <= 1.0.0"') == []


@with_lib1
def test_exists3():
    assert pypkgconf.query_args('--exists "" "foo"') == []


@pytest.mark.xfail
@with_lib1
def test_libs_intermediary():
    result = pypkgconf.query_args('--libs intermediary-1 intermediary-2')

    assert result == ['-lintermediary-1', '-lintermediary-2', '-lfoo', '-lbar', '-lbaz']


@with_lib1
def test_libs_circular2(caplog):
    assert pypkgconf.query_args('circular-2 --validate') == []
    assert 'circular-1: breaking circular reference (circular-1 -> circular-2 -> circular-1)\n' in caplog.text


@with_lib1
def test_libs_circular1(caplog):
    assert pypkgconf.query_args('circular-1 --validate') == []
    assert 'circular-3: breaking circular reference (circular-3 -> circular-1 -> circular-3)\n' in caplog.text


@pytest.mark.xfail
@with_lib1
def test_libs_circular_directpc(testsdir):
    result = pypkgconf.query_args(f'--libs {testsdir.as_posix()}/lib1/circular-3.pc')

    assert result == ['-lcircular-2', '-lcircular-3', '-lcircular-1']


# @pytest.mark.xfail
@with_lib1
def test_libs_static():
    result = pypkgconf.query_args('--libs static-archive-libs')

    assert result == ['/libfoo.a', '-pthread']


@with_lib1
def test_libs_static_ordering():
    result = pypkgconf.query_args('--libs foo bar')

    assert result == ['-L/test/lib', '-lbar', '-lfoo']


@pytest.mark.xfail
@with_lib1lib2
def test_pkg_config_path():
    result = pypkgconf.query_args('--libs foo')

    assert result == ['-L/test/lib', '-lfoo']

    result = pypkgconf.query_args('--libs bar')

    assert result == ['-L/test/lib', '-lbar', '-lfoo']


@pytest.mark.xfail
def test_with_path(testsdir):
    lib1_path = testsdir / 'lib1'
    lib2_path = testsdir / 'lib2'

    result = pypkgconf.query_args(f'--with-path={lib1_path.as_posix()} --with-path={lib2_path.as_posix()} --libs foo')

    assert result == ['-L/test/lib', '-lfoo']

    result = pypkgconf.query_args(f'--with-path={lib1_path.as_posix()} --with-path={lib2_path.as_posix()} --libs bar')

    assert result == ['-L/test/lib', '-lbar', '-lfoo']


@with_lib1
def test_nolibs():
    assert pypkgconf.query_args('--libs nolib') == []


@with_lib1
def test_nocflags():
    assert pypkgconf.query_args('--cflags nocflag') == []


def test_arbitary_path(testsdir, tmp_path, monkeypatch):
    shutil.copy(testsdir / 'lib1' / 'foo.pc', tmp_path)
    monkeypatch.chdir(tmp_path)

    result = pypkgconf.query_args('--libs foo.pc')

    assert result == ['-L/test/lib', '-lfoo']


def test_relocatable(testsdir):
    basedir = pypkgconf.query_args(f'--relocate {testsdir.as_posix()}')[0]

    result = pypkgconf.query_args(f'--define-prefix --variable=prefix {basedir}/lib-relocatable/lib/pkgconfig/foo.pc')

    assert result == [f'{basedir}/lib-relocatable']


def test_single_depth_selectors(monkeypatch, testsdir):
    monkeypatch.setenv('PKG_CONFIG_MAXIMUM_TRAVERSE_DEPTH', '1')

    result = pypkgconf.query_args(f'--with-path={testsdir.as_posix()}/lib3 --print-requires bar')

    assert result == ['foo']


def test_license_isc(testsdir):
    result = pypkgconf.query_args(f'--with-path={testsdir.as_posix()}/lib1 --license foo')

    assert result == ['foo: ISC']


@pytest.mark.xfail
def test_license_noassertion(testsdir):
    result = pypkgconf.query_args(f'--with-path={testsdir.as_posix()}/lib1 --license bar')

    assert result == ['bar: NOASSERTION', 'foo: ISC']


def test_modversion_noflatten(testsdir):
    result = pypkgconf.query_args(f'--with-path={testsdir.as_posix()}/lib1 --modversion bar')
    
    assert result == ['1.3']
