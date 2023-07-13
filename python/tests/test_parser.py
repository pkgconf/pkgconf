import pypkgconf

import pytest

import logging


with_lib1 = pytest.mark.usefixtures('lib1_pkg_config_path')


@with_lib1
def test_comments():
    result = pypkgconf.query_args('--libs comments')

    assert result == ['-lfoo']


@with_lib1
def test_comments_in_fields():
    result = pypkgconf.query_args('--libs comments-in-fields')

    assert result == ['-lfoo']


@with_lib1
def test_dos():
    result = pypkgconf.query_args('--libs dos-lineendings')

    assert result == ['-L/test/lib/dos-lineendings' ,'-ldos-lineendings']


@with_lib1
def test_no_trailing_newline():
    result = pypkgconf.query_args('--cflags no-trailing-newline')

    assert result == ['-I/test/include/no-trailing-newline']


@with_lib1
def test_parse():
    result = pypkgconf.query_args('--libs argv-parse')

    assert result == ['-llib-3', '-llib-1', '-llib-2', '-lpthread']


@with_lib1
def test_bad_option():
    with pytest.raises(pypkgconf.PkgConfError):
        pypkgconf.query_args('--exists -foo')


@with_lib1
def test_argv_parse_3():
    result = pypkgconf.query_args('--libs argv-parse-3')

    assert result == ['-llib-1', '-pthread', '/test/lib/lib2.so']


@with_lib1
def test_quoting():
    result = pypkgconf.query_args('--libs tilde-quoting')
    assert result == ['-L~', '-ltilde']

    result = pypkgconf.query_args('--cflags tilde-quoting')
    assert result == ['-I~']


@with_lib1
def test_paren_quoting():
    result = pypkgconf.query_args('--libs paren-quoting')

    # FIXME: escape $?
    assert result == ['-L$(libdir)', '-ltilde']


@with_lib1
def test_multiline_field():
    result = pypkgconf.query_args('--list-all')

    assert 'multiline description' in '\n'.join(result)


@with_lib1
def test_multiline_bogus_header():
    assert pypkgconf.query_args('--exists multiline-bogus') == []


def test_escaped_backslash(testsdir):
    selfdir = (testsdir / 'lib1').as_posix()
    result = pypkgconf.query_args(f'--with-path={selfdir} --cflags escaped-backslash')

    assert result == ['-IC:\\\\A']


@with_lib1
def test_quoted():
    result = pypkgconf.query_args('--cflags quotes')

    #FIXME: is it ok?
    assert result == ['-DQUOTED=\\"bla\\"', '-DA=\\"escaped\\ string\\\'\\ literal\\"', '-DB=\\\\1$', '-DC=bla']


@with_lib1
def test_flag_order_1():
    result = pypkgconf.query_args('--libs flag-order-1')

    assert result == ['-L/test/lib', '-Bdynamic', '-lfoo', '-Bstatic', '-lbar']


@pytest.mark.xfail
@with_lib1
def test_flag_order_2():
    result = pypkgconf.query_args('--libs flag-order-1 foo')

    assert result == ['-L/test/lib', '-Bdynamic', '-lfoo', '-Bstatic', '-lbar', '-lfoo']


@with_lib1
def test_flag_order_3():
    result = pypkgconf.query_args('--libs flag-order-3')

    assert result == ['-L/test/lib', '-Wl,--start-group', '-lfoo', '-lbar', '-Wl,--end-group']


@pytest.mark.xfail
@with_lib1
def test_flag_order_4():
    result = pypkgconf.query_args('--libs flag-order-3 foo')

    assert result == ['-L/test/lib', '-Wl,--start-group', '-lfoo', '-lbar', '-Wl,--end-group', '-lfoo']


@with_lib1
def test_variable_whitespace():
    result = pypkgconf.query_args('--cflags variable-whitespace')

    assert result == ['-I/test/include']


@with_lib1
def test_fragment_quoting():
    result = pypkgconf.query_args('--cflags fragment-quoting')

    assert result == ['-fPIC', '-I/test/include/foo', '-DQUOTED=\\\"/test/share/doc\\\"']


@with_lib1
def test_fragment_quoting_2():
    result = pypkgconf.query_args('--cflags fragment-quoting-2')

    assert result == ['-fPIC', '-I/test/include/foo', '-DQUOTED=/test/share/doc']


@with_lib1
def test_fragment_quoting_3():
    result = pypkgconf.query_args('--cflags fragment-quoting-3')

    assert result == ['-fPIC', '-I/test/include/foo', '-DQUOTED=\\\"/test/share/doc\\\"']


@with_lib1
def test_fragment_quoting_5():
    result = pypkgconf.query_args('--cflags fragment-quoting-5')

    assert result == ['-fPIC', '-I/test/include/foo', '-DQUOTED=/test/share/doc']


@with_lib1
def test_fragment_quoting_7():
    result = pypkgconf.query_args('--cflags fragment-quoting-7')

    assert result == ['-Dhello=10', '-Dworld=+32', '-DDEFINED_FROM_PKG_CONFIG=hello\\ world']


def test_fragment_escaping_1(testsdir):
    selfdir = (testsdir / 'lib1').as_posix()
    result = pypkgconf.query_args(f'--with-path={selfdir} --cflags fragment-escaping-1')

    assert result == ['-IC:\\\\D\\ E']


def test_fragment_escaping_2(testsdir):
    selfdir = (testsdir / 'lib1').as_posix()
    result = pypkgconf.query_args(f'--with-path={selfdir} --cflags fragment-escaping-2')

    assert result == ['-IC:\\\\D\\ E']


def test_fragment_escaping_3(testsdir):
    selfdir = (testsdir / 'lib1').as_posix()
    result = pypkgconf.query_args(f'--with-path={selfdir} --cflags fragment-escaping-3')

    assert result == ['-IC:\\\\D\\ E']


# FIXME: how to test macro arithmetics in Python...
# def test_fragment_quoting_7a(testsdir):
#     selfdir = (testsdir / 'lib1').as_posix()
#     result = pypkgconf.query_args(f'--with-path={selfdir} --cflags fragment-quoting-7')

#     assert result == ['-Dhello=10', '-Dworld=+32', '-DDEFINED_FROM_PKG_CONFIG=hello\\ world']

    # 	cat > test.c <<- __TESTCASE_END__
    # 		int main(int argc, char *argv[]) { return DEFINED_FROM_PKG_CONFIG; }
    # 	__TESTCASE_END__
    # 	cc -o test-fragment-quoting-7 ${test_cflags} ./test.c
    # 	atf_check -e 42 ./test-fragment-quoting-7
    # 	rm -f test.c test-fragment-quoting-7


def test_fragment_comment(testsdir):
    selfdir = (testsdir / 'lib1').as_posix()
    result = pypkgconf.query_args(f'--with-path={selfdir} --cflags fragment-comment')

    assert result == ['kuku=\\#ttt']


@pytest.mark.skip(reason='--msvc-syntax not implemented')
@with_lib1
def test_msvc_fragment_quoting():
    result = pypkgconf.query_args('--libs --msvc-syntax fragment-escaping-1')

    assert result == ['/libpath:"C:\\D E"', 'E.lib']


@pytest.mark.skip(reason='--msvc-syntax not implemented')
@with_lib1
def test_msvc_fragment_render_cflags():
    result = pypkgconf.query_args('--cflags --static --msvc-syntax foo')

    assert result == ['/I/test/include/foo', '/DFOO_STATIC']


def test_tuple_dequote(testsdir):
    selfdir = (testsdir / 'lib1').as_posix()
    result = pypkgconf.query_args(f'--with-path={selfdir} --libs tuple-quoting')

    assert result == ['-L/test/lib', '-lfoo']


def test_version_with_whitespace(testsdir):
    selfdir = (testsdir / 'lib1').as_posix()
    result = pypkgconf.query_args(f'--with-path={selfdir} --modversion malformed-version')

    assert result == ['3.922']


def test_version_with_whitespace_2(testsdir):
    selfdir = (testsdir / 'lib1').as_posix()
    result = pypkgconf.query_args(f'--with-path={selfdir} --print-provides malformed-version')

    assert result == ['malformed-version = 3.922']


def test_version_with_whitespace_diagnostic(testsdir, caplog):
    selfdir = (testsdir / 'lib1').as_posix()

    with caplog.at_level(logging.WARNING):
        result = pypkgconf.query_args(f'--with-path={selfdir} --validate malformed-version')

    assert result == []
    assert any(record.levelname == 'WARNING' for record in caplog.records)
