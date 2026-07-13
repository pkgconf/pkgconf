#include <libpkgconf/libpkgconf.h>
#include <stdio.h>

int
main(void)
{
    int cmp = pkgconf_compare_version("1.0", "1.0");
	  printf("pkgconf_compare_version(\"1.0\", \"1.0\") = %d\n", cmp);
	  return cmp == 0 ? 0 : 1;
}
