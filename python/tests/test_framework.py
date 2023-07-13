import pypkgconf

import pytest


with_lib1 = pytest.mark.usefixtures('lib1_pkg_config_path')


@pytest.mark.xfail
@with_lib1
def test_libs():
    result = pypkgconf.query_args('--libs framework-1')
    assert result == ['-F/test/lib', '-framework framework-1']

    result = pypkgconf.query_args('--libs framework-2')
    assert result == ['-F/test/lib', '-framework framework-2', '-framework framework-1']

    result = pypkgconf.query_args('--libs framework-1 framework-2')
    assert result == ['-F/test/lib', '-framework framework-2', '-framework framework-1']
