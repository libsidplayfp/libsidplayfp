AC_INIT([sidplay-libs],[2.1.1])
AC_CONFIG_AUX_DIR([unix])
AC_CONFIG_MACRO_DIR([unix])
AC_CONFIG_SRCDIR([Makefile.am])
AM_INIT_AUTOMAKE

#Variables
src=`cd $srcdir && pwd`
build=`pwd`

ac_configure_args="$ac_configure_args \
--with-sidplay2-includes=$build/libsidplay/include \
--with-sidplay2-library=$build/libsidplay/src \
--with-resid-includes=$src/resid \
--with-resid-library=$build/resid \
--disable-library-checks --disable-resid-install"

AC_CANONICAL_HOST
case "$host" in
    *linux*|*-k*bsd*-gnu|*mingw*)
	    hardsid=true
        ;;
    *)
        hardsid=false
        ;;
esac
AM_CONDITIONAL(HARDSID, [test x$hardsid = xtrue])

AC_CONFIG_SUBDIRS([libsidplay])
AC_CONFIG_SUBDIRS([resid])
AC_CONFIG_SUBDIRS([builders/resid-builder])
AC_CONFIG_SUBDIRS([builders/hardsid-builder])
AC_CONFIG_SUBDIRS([libsidutils])

AC_CONFIG_FILES([
Makefile
builders/Makefile
])

AC_OUTPUT
