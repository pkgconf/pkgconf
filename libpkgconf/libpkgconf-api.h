#ifndef PKGCONFG_API
#define PKGCONFG_API

/* Makefile.am specifies visibility using the libtool option -export-symbols-regex '^pkgconf_'
 * Unfortunately, that is not available when building with cmake, so use attributes instead,
 * in a way that doesn't depend on any cmake magic.
 */
#if defined(PKGCONFIG_IS_STATIC)
# define PKGCONF_API
#elif defined(_WIN32) || defined(_WIN64)
# if defined(LIBPKGCONF_EXPORT) || defined(DLL_EXPORT)
#  define PKGCONF_API __declspec(dllexport)
# else
#  define PKGCONF_API __declspec(dllimport)
# endif
#else
# define PKGCONF_API __attribute__((visibility("default")))
#endif

#endif
