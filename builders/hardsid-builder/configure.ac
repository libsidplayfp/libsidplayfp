m4_define([LIBCUR],0)
m4_define([LIBREV],1)
m4_define([LIBAGE],0)

dnl Process this file with autoconf to produce a configure script.
AC_INIT([hardsid-builder],[1.LIBCUR.LIBREV])
AC_CONFIG_AUX_DIR([unix])
AC_CONFIG_MACRO_DIR([unix])
AC_CONFIG_SRCDIR([src/hardsid-builder.cpp])
AC_CONFIG_HEADERS([include/config.h])
AM_INIT_AUTOMAKE

dnl libtool-style version-info number
LIBVERSION=LIBCUR:LIBREV:LIBAGE
AC_SUBST(LIBVERSION)

AC_CANONICAL_HOST
case "$host" in
    *mingw32* )
        LDFLAGS="${LDFLAGS} -no-undefined"
        MINGW32=yes
        ;;
    *)
        MINGW32=no
        ;;
esac
AM_CONDITIONAL([MINGW32], [ test "x$MINGW32" = "xyes" ])

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CXX

CHECK_VISIBILITY

dnl Initialize libtool.
LT_INIT([win32-dll])
#AC_LIBTOOL_WIN32_DLL
#AC_PROG_LIBTOOL

dnl Use C++ for tests.
AC_LANG_CPLUSPLUS

dnl Check for nothrow allocator.
MY_CHECK_EXCEPTIONS

AC_MSG_CHECKING([for sidbuilders install directory])

dnl setup builder dir
#test "$prefix" = NONE && prefix="$LIBSIDPLAY2_PREFIX"
#test "$libdir" = '${exec_prefix}/lib' && libdir="$LIBSIDPLAY2_BUILDERS"

AC_MSG_RESULT([${libdir}])

AC_CONFIG_FILES([
Makefile
include/sidplay/builders/Makefile
src/Makefile
])

AC_OUTPUT
