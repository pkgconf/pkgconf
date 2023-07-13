import pypkgconf

import pytest

with_lib1 = pytest.mark.usefixtures('lib1_pkg_config_path')


@with_lib1
def test_atleast():
    assert pypkgconf.query_args('--atleast-version 1.0 foo') == []

    with pytest.raises(pypkgconf.PkgConfError):
        pypkgconf.query_args('--atleast-version 2.0 foo')


@with_lib1
def test_exact():
    with pytest.raises(pypkgconf.PkgConfError):
        pypkgconf.query_args('--exact-version 1.0 foo')
        
    assert pypkgconf.query_args('--exact-version 1.2.3 foo') == []


@with_lib1
def test_max():
    with pytest.raises(pypkgconf.PkgConfError):
        pypkgconf.query_args('--max-version 1.0 foo')

    assert pypkgconf.query_args('--max-version 2.0 foo') == []
