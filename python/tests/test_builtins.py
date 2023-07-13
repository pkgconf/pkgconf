import pypkgconf

import pytest


with_lib1 = pytest.mark.usefixtures('lib1_pkg_config_path')


# FIXME: this test suite does not seem to be run with Kyua

@with_lib1
def test_modversion():
    result = pypkgconf.query_args('--modversion pkg-config')

    # FIXME: how could it be 1.0.1?
    assert result == ['1.9.5']


@with_lib1
def test_variable():
    result = pypkgconf.query_args('--variable=prefix foo')

    assert result == ['/test']


@with_lib1
def test_define_variable():
    result = pypkgconf.query_args('--define-variable=prefix=/test2 --variable=prefix foo')

    assert result == ['/test2']


# @pytest.mark.xfail
# @with_lib1
# def test_global_variable(testsdir):
#     # FIXME: broken?
#     result = pypkgconf.query_args('--exists -foo')

#     assert result == [ (testsdir / 'lib1').as_posix() ]


# @with_lib1
# def test_argv_parse_3():
#     result = pypkgconf.query_args('--libs argv-parse-3')

#     assert result == ['-llib-1', '-pthread /test/lib/lib2.so']


# @with_lib1
# def test_tilde_quoting():
#     result = pypkgconf.query_args('--libs tilde-quoting')
#     assert result == ['-L~', '-ltilde']

#     result = pypkgconf.query_args('--cflags tilde-quoting')
#     assert result == ['-I~']


# @with_lib1
# def test_paren_quoting():
#     result = pypkgconf.query_args('--libs paren-quoting')

#     # FIXME: escape $?
#     assert result == ['-L$(libdir)', '-ltilde']
