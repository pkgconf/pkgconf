# pkgconf

pkgconf provides compiler and linker configuration for development frameworks.

## general summary

pkgconf is a program which helps to configure compiler and linker flags for
development frameworks.

It is similar to pkg-config, but was written from scratch in the summer of 2011
to replace pkg-config, which for a while needed itself to build itself (they have
since included a 'stripped down copy of glib 2.0')  Since then we have worked on
improving pkg-config for embedded use.

## usage

Implementations of pkg-config, such as pkgconf, are typically used with the
PKG_CHECK_MODULES autoconf macro.  As far as we know, pkgconf is
compatible with all known variations of this macro. pkgconf detects at
runtime whether or not it was started as 'pkg-config', and if so, attempts
to set program options such that its behaviour is similar.

In terms of the autoconf macro, it is possible to specify the PKG_CONFIG
environment variable, so that you can test pkgconf without overwriting your
pkg-config binary.  Some other build systems may also respect the PKG_CONFIG
environment variable.

To set the enviornment variable on the bourne shell and clones (i.e. bash), you
can run:

    $ export PKG_CONFIG=/usr/bin/pkgconf

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

If you are installing pkgconf into a custom prefix, such as `/opt/pkgconf`, you will
likely want to define the default system includedir and libdir for your toolchain.
To do this, use the `--with-system-includedir` and `--with-system-libdir` configure
flags like so:

    $ ./configure \
         --prefix=/opt/pkgconf \
         --with-system-libdir=/lib:/usr/lib \
         --with-system-includedir=/usr/include
    $ make
    $ sudo make install

If you want pkgconf to be used when you invoke `pkg-config`, you should install a
symlink for this.  We do not do this for you, as we believe it is better for vendors
to make this determination themselves.

    $ ln -sf /usr/bin/pkgconf /usr/bin/pkg-config

## release tarballs

Release tarballs are available at <http://tortois.es/~nenolod/distfiles/>.

## reporting bugs

See <https://github.com/pkgconf/pkgconf/issues>.
