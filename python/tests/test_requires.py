import pypkgconf

import pytest

import sys


with_lib1 = pytest.mark.usefixtures('lib1_pkg_config_path')
skip_pure = pytest.mark.skipif(sys.platform in {'win32', 'cygwin', 'msys'},
                               reason='default personality is pure on Windows')


@pytest.mark.xfail
@with_lib1
def test_libs():
    result = pypkgconf.query_args('--libs bar')
    assert result == ['-L/test/lib', '-lbar', '-lfoo']


@with_lib1
def test_cflags():
    result = pypkgconf.query_args('--libs --cflags baz')
    assert result == ['-fPIC', '-I/test/include/foo', '-L/test/lib', '-lbaz']


@skip_pure
@with_lib1
def test_libs_static():
    result = pypkgconf.query_args('--static --libs baz')
    assert result == ['-L/test/lib', '-lbaz', '-L/test/lib', '-lzee', '-L/test/lib', '-lfoo']


@with_lib1
def test_libs_static_pure():
    result = pypkgconf.query_args('--static --pure --libs baz')
    assert result == ['-L/test/lib', '-lbaz', '-L/test/lib', '-lfoo']


@with_lib1
def test_argv_parse2():
    result = pypkgconf.query_args('--static --libs argv-parse-2')
    assert result == ['-llib-1', '-pthread', '/test/lib/lib2.so']


@skip_pure
@with_lib1
def test_static_cflags():
    result = pypkgconf.query_args('--static --cflags baz')
    assert result == ['-fPIC', '-I/test/include/foo', '-DFOO_STATIC']


@skip_pure
@with_lib1
def test_private_duplication():
    result = pypkgconf.query_args('--static --libs-only-l private-libs-duplication')
    assert result == ['-lprivate', '-lbaz', '-lzee', '-lbar', '-lfoo', '-lfoo']


@skip_pure
@with_lib1
def test_libs_static2():
    result = pypkgconf.query_args('--static --libs static-libs')
    assert result == ['-lbar', '-lbar-private', '-L/test/lib', '-lfoo']


@with_lib1
def test_missing():
    with pytest.raises(pypkgconf.PkgConfError):
        pypkgconf.query_args('--cflags missing-require')


@skip_pure
def test_requires_internal(testsdir):
    selfdir = testsdir.as_posix()

    result = pypkgconf.query_args(f'--with-path="{selfdir}/lib1" --static --libs requires-internal')
    assert result == ['-lbar', '-lbar-private', '-L/test/lib', '-lfoo']


def test_requires_internal_missing(testsdir):
    selfdir = testsdir.as_posix()

    with pytest.raises(pypkgconf.PkgConfError):
        pypkgconf.query_args(f'--with-path="{selfdir}/lib1" --static --libs requires-internal-missing')


def test_requires_internal_collision(testsdir):
    selfdir = testsdir.as_posix()

    result = pypkgconf.query_args(f'--with-path="{selfdir}/lib1" --cflags requires-internal-collision')
    assert result == ['-I/test/local/include/foo']


def test_orphaned_requires_private(testsdir):
    selfdir = testsdir.as_posix()

    with pytest.raises(pypkgconf.PkgConfError):
        pypkgconf.query_args(f'--with-path="{selfdir}/lib1" --cflags --libs orphaned-requires-private')
