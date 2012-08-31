# pkgconf

pkgconf provides compiler and linker configuration for development frameworks.

## general summary

pkgconf is a program which helps to configure compiler and linker flags for
development frameworks.  It is similar to pkg-config, but was written from
scratch in the summer of 2011 to replace pkg-config, which now needs itself to
build itself (or you can set a bunch of environment variables, both are
pretty ugly).

Implementations of pkg-config, such as pkgconf, are typically used with the
PKG_CHECK_MODULES autoconf macro.  As far as I (nenolod) know, pkgconf is
compatible with all known variations of this macro. pkgconf detects at
runtime whether or not it was started as 'pkg-config', and if so, attempts
to set program options such that its behaviour is similar.

In terms of the autoconf macro, it is possible to specify the PKG_CONFIG
environment variable, so that you can test pkgconf without overwriting your
pkg-config binary.  (hint: export PKG_CONFIG=/usr/bin/pkgconf)  However,
if you do this, it will be running in native mode, so you may have some very
strange results as the dependency graph is compiled differently in native
mode.

## technical design (why pkgconf is better for distros)

pkgconf builds an acyclic directed dependency graph.  This allows for the user
to more conservatively link their binaries -- which may be helpful in some 
environments, such as when prelink(1) is being used.  As a result of building
a directed dependency graph designed for the specific problem domain provided
by the user, more accurate dependencies can be determined.  pkg-config, on the
other hand builds a database of all known pkg-config files on the system before
attempting to resolve dependencies, which is a considerably slower and less
efficient design.

pkgconf also does not bundle any third-party libraries or depend on any third-party
libraries, making it a great tool for embedded systems and distributions with
security concerns.

## compiling

pkgconf is basically compiled the same way any other autotools-based project is
compiled:

    $ ./configure
    $ make
    $ sudo make install

## release tarballs

Release tarballs are available at <http://tortois.es/~nenolod/distfiles/>.

## reporting bugs

See <https://github.com/pkgconf/pkgconf/issues>.
