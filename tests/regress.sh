#!/usr/bin/env atf-sh

. $(atf_get_srcdir)/test_env.sh

tests_init \
	modversion_one_word_expression \
	modversion_two_word_expression \
	modversion_three_word_expression \
	modversion_one_word_expression_no_space \
	modversion_one_word_expression_no_space_zero \
	explicit_sysroot

#	sysroot_munge \

sysroot_munge_body()
{
	sed "s|/sysroot/|${selfdir}/|g" ${selfdir}/lib1/sysroot-dir.pc > ${selfdir}/lib1/sysroot-dir-selfdir.pc
	export PKG_CONFIG_PATH="${selfdir}/lib1" PKG_CONFIG_SYSROOT_DIR="${selfdir}"
	atf_check \
		-o inline:"-L${selfdir}/lib -lfoo\n" \
		pkgconf --libs sysroot-dir-selfdir
}

explicit_sysroot_body()
{
	export PKG_CONFIG_SYSROOT_DIR=${selfdir}
	atf_check -o inline:"${selfdir}/usr/share/test\n" \
		pkgconf --with-path="${selfdir}/lib1" --variable=pkgdatadir explicit-sysroot
}

modversion_one_word_expression_body()
{
	atf_check -o inline:"1.2.3\n" \
		pkgconf --with-path="${selfdir}/lib1" --modversion "foo > 1.0"
}

modversion_two_word_expression_body()
{
	atf_check -o inline:"1.2.3\n" \
		pkgconf --with-path="${selfdir}/lib1" --modversion foo "> 1.0"
}

modversion_three_word_expression_body()
{
	atf_check -o inline:"1.2.3\n" \
		pkgconf --with-path="${selfdir}/lib1" --modversion foo ">" 1.0
}

modversion_one_word_expression_no_space_body()
{
	atf_check -o inline:"1.2.3\n" \
		pkgconf --with-path="${selfdir}/lib1" --modversion "foo >1.0"
}

modversion_one_word_expression_no_space_zero_body()
{
	atf_check -o inline:"1.2.3\n" \
		pkgconf --with-path="${selfdir}/lib1" --modversion "foo >0.5"
}
