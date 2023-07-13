import pypkgconf

import pytest


with_lib1 = pytest.mark.usefixtures('lib1_pkg_config_path')


@with_lib1
def test_simple():
    result = pypkgconf.query_args('--print-provides provides')
    assert result == [
        'provides-test-foo = 1.0.0',
        'provides-test-bar > 1.1.0',
        'provides-test-baz >= 1.1.0',
        'provides-test-quux < 1.2.0',
        'provides-test-moo <= 1.2.0',
        'provides-test-meow != 1.3.0',
        'provides = 1.2.3',
    ]

    result = pypkgconf.query_args('--libs provides-request-simple')
    assert result == ['-lfoo']

    with pytest.raises(pypkgconf.PkgConfError):
        pypkgconf.query_args('--no-provides --libs provides-request-simple')


@with_lib1
def test_foo():
    assert pypkgconf.query_args('--libs provides-test-foo') == ['-lfoo']
    assert pypkgconf.query_args('--libs "provides-test-foo = 1.0.0"') == ['-lfoo']
    assert pypkgconf.query_args('--libs "provides-test-foo >= 1.0.0"') == ['-lfoo']
    assert pypkgconf.query_args('--libs "provides-test-foo <= 1.0.0"') == ['-lfoo']
    with pytest.raises(pypkgconf.PkgConfError):
        pypkgconf.query_args('--libs "provides-test-foo != 1.0.0"')
    with pytest.raises(pypkgconf.PkgConfError):
        pypkgconf.query_args('--libs "provides-test-foo > 1.0.0"')
    with pytest.raises(pypkgconf.PkgConfError):
        pypkgconf.query_args('--libs "provides-test-foo < 1.0.0"')


@with_lib1
def test_bar():
    assert pypkgconf.query_args('--libs provides-test-bar') == ['-lfoo']
    assert pypkgconf.query_args('--libs "provides-test-bar = 1.1.1"') == ['-lfoo']
    assert pypkgconf.query_args('--libs "provides-test-bar >= 1.1.1"') == ['-lfoo']
    assert pypkgconf.query_args('--libs "provides-test-bar <= 1.1.1"') == ['-lfoo']
    assert pypkgconf.query_args('--libs "provides-test-bar != 1.1.0"') == ['-lfoo']
    with pytest.raises(pypkgconf.PkgConfError):
        pypkgconf.query_args('--libs "provides-test-bar != 1.1.1"')
    assert pypkgconf.query_args('--libs "provides-test-bar > 1.1.1"') == ['-lfoo']
    with pytest.raises(pypkgconf.PkgConfError):
        pypkgconf.query_args('--libs "provides-test-bar <= 1.1.0"')
    assert pypkgconf.query_args('--libs "provides-test-bar <= 1.2.0"') == ['-lfoo']
    

@with_lib1
def test_bar():
    assert pypkgconf.query_args('--libs provides-test-baz') == ['-lfoo']
    assert pypkgconf.query_args('--libs "provides-test-baz = 1.1.0"') == ['-lfoo']
    assert pypkgconf.query_args('--libs "provides-test-baz >= 1.1.0"') == ['-lfoo']
    assert pypkgconf.query_args('--libs "provides-test-baz <= 1.1.0"') == ['-lfoo']
    with pytest.raises(pypkgconf.PkgConfError):
        pypkgconf.query_args('--libs "provides-test-baz != 1.1.0"')
    assert pypkgconf.query_args('--libs "provides-test-baz !- 1.0.0"') == ['-lfoo']
    assert pypkgconf.query_args('--libs "provides-test-baz > 1.1.1"') == ['-lfoo']
    assert pypkgconf.query_args('--libs "provides-test-baz > 1.1.0"') == ['-lfoo']
    with pytest.raises(pypkgconf.PkgConfError):
        pypkgconf.query_args('--libs "provides-test-baz < 1.1.0"')
    assert pypkgconf.query_args('--libs "provides-test-baz < 1.2.0"') == ['-lfoo']


@with_lib1
def test_quux():
    assert pypkgconf.query_args('--libs provides-test-quux') == ['-lfoo']
    assert pypkgconf.query_args('--libs "provides-test-quux = 1.1.9"') == ['-lfoo']
    assert pypkgconf.query_args('--libs "provides-test-quux >= 1.1.0"') == ['-lfoo']
    assert pypkgconf.query_args('--libs "provides-test-quux >= 1.1.9"') == ['-lfoo']
    with pytest.raises(pypkgconf.PkgConfError):
        pypkgconf.query_args('--libs "provides-test-quux >= 1.2.0"')
    with pytest.raises(pypkgconf.PkgConfError):
        pypkgconf.query_args('--libs "provides-test-quux <= 1.2.0"')
    assert pypkgconf.query_args('--libs "provides-test-quux <= 1.1.9"') == ['-lfoo']
    assert pypkgconf.query_args('--libs "provides-test-quux != 1.2.0"') == ['-lfoo']
    with pytest.raises(pypkgconf.PkgConfError):
        pypkgconf.query_args('--libs "provides-test-quux != 1.1.0"')
    assert pypkgconf.query_args('--libs "provides-test-quux > 1.1.9"') == ['-lfoo']
    with pytest.raises(pypkgconf.PkgConfError):
        pypkgconf.query_args('--libs "provides-test-quux > 1.2.0"')
    assert pypkgconf.query_args('--libs "provides-test-quux < 1.1.0"') == ['-lfoo']
    # FIXME: Test duplicated...
    # with pytest.raises(pypkgconf.PkgConfError):
    #     pypkgconf.query_args('--libs "provides-test-quux > 1.2.0"')


@with_lib1
def test_moo():
    assert pypkgconf.query_args('--libs provides-test-moo') == ['-lfoo']
    assert pypkgconf.query_args('--libs "provides-test-moo = 1.2.0"') == ['-lfoo']
    assert pypkgconf.query_args('--libs "provides-test-moo >= 1.1.0"') == ['-lfoo']
    assert pypkgconf.query_args('--libs "provides-test-moo >= 1.2.0"') == ['-lfoo']
    with pytest.raises(pypkgconf.PkgConfError):
        pypkgconf.query_args('--libs "provides-test-moo >= 1.2.1"')
    assert pypkgconf.query_args('--libs "provides-test-moo <= 1.2.0"') == ['-lfoo']
    with pytest.raises(pypkgconf.PkgConfError):
        pypkgconf.query_args('--libs "provides-test-moo != 1.1.0"')
    with pytest.raises(pypkgconf.PkgConfError):
        pypkgconf.query_args('--libs "provides-test-moo != 1.0.0"')
    assert pypkgconf.query_args('--libs "provides-test-moo > 1.1.9"') == ['-lfoo']
    with pytest.raises(pypkgconf.PkgConfError):
        pypkgconf.query_args('--libs "provides-test-moo > 1.2.0"')
    assert pypkgconf.query_args('--libs "provides-test-moo < 1.1.0"') == ['-lfoo']
    assert pypkgconf.query_args('--libs "provides-test-moo < 1.2.0"') == ['-lfoo']



@with_lib1
def test_meow():
    assert pypkgconf.query_args('--libs provides-test-meow') == ['-lfoo']
    with pytest.raises(pypkgconf.PkgConfError):
        pypkgconf.query_args('--libs "provides-test-meow = 1.3.0"')
    assert pypkgconf.query_args('--libs "provides-test-meow != 1.3.0"') == ['-lfoo']
    with pytest.raises(pypkgconf.PkgConfError):
        pypkgconf.query_args('--libs "provides-test-meow > 1.2.0"')
    with pytest.raises(pypkgconf.PkgConfError):
        pypkgconf.query_args('--libs "provides-test-meow < 1.3.1"')
    assert pypkgconf.query_args('--libs "provides-test-meow < 1.3.0"') == ['-lfoo']
    assert pypkgconf.query_args('--libs "provides-test-meow > 1.3.0"') == ['-lfoo']
    with pytest.raises(pypkgconf.PkgConfError):
        pypkgconf.query_args('--libs "provides-test-meow >= 1.3.0"')
    assert pypkgconf.query_args('--libs "provides-test-meow >= 1.3.1"') == ['-lfoo']
    with pytest.raises(pypkgconf.PkgConfError):
        pypkgconf.query_args('--libs "provides-test-meow <= 1.3.0"')
    assert pypkgconf.query_args('--libs "provides-test-meow < 1.2.0"') == ['-lfoo']


def test_indirect_dependency_node(testsdir):
    selfdir = (testsdir / 'lib1').as_posix()

    result = pypkgconf.query_args(f'--with-path="{selfdir}" --modversion "provides-test-meow"')
    assert result == ['1.2.3']

    with pytest.raises(pypkgconf.PkgConfError):
        pypkgconf.query_args(f'--with-path="{selfdir}" --modversion "provides-test-meow = 1.3.0"')
