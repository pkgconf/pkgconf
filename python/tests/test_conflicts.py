import pypkgconf

import pytest


with_lib1 = pytest.mark.usefixtures('lib1_pkg_config_path')


@with_lib1
def test_libs():
    result = pypkgconf.query_args('--libs conflicts')

    assert result == ['-L/test/lib', '-lconflicts']


@with_lib1
def test_ignore():
    result = pypkgconf.query_args('--ignore-conflicts --libs conflicts')

    assert result == ['-L/test/lib', '-lconflicts']
