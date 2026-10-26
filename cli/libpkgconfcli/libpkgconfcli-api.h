#ifndef LIBPKGCONFCLI__LIBPKGCONFCLI_API_H
#define LIBPKGCONFCLI__LIBPKGCONFCLI_API_H

/* Makefile.am specifies visibility using the libtool option -export-symbols-regex '^pkgconf_'
 * Unfortunately, that is not available when building with meson, so use attributes instead.
 */
#if defined(PKGCONFIG_IS_STATIC)
# define PKGCONFCLI_API
#elif defined(_WIN32) || defined(_WIN64)
# if defined(LIBPKGCONFCLI_EXPORT) || defined(DLL_EXPORT)
#  define PKGCONFCLI_API __declspec(dllexport)
# else
#  define PKGCONFCLI_API __declspec(dllimport)
# endif
#elif defined(__OS2__)
# if defined(LIBPKGCONFCLI_EXPORT) || defined(DLL_EXPORT)
#  define PKGCONFCLI_API __declspec(dllexport)
# else
#  define PKGCONFCLI_API
# endif
#else
# define PKGCONFCLI_API __attribute__((visibility("default")))
#endif

#endif
