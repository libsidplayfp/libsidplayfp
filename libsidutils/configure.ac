m4_define([LIBCUR],0)
m4_define([LIBREV],4)
m4_define([LIBAGE],0)

dnl Process this file with autoconf to produce a configure script.
AC_INIT([libsidutils],[1.LIBCUR.LIBREV])
AC_CONFIG_AUX_DIR([unix])
AC_CONFIG_MACRO_DIR([unix])
AC_CONFIG_SRCDIR([src/SidTuneMod.cpp])
AC_CONFIG_HEADERS([include/config.h])
AM_INIT_AUTOMAKE

dnl Initialize libtool.
LT_INIT([win32-dll])
#AC_LIBTOOL_WIN32_DLL
#AC_PROG_LIBTOOL

dnl libtool-style version-info number
LIBVERSION=LIBCUR:LIBREV:LIBAGE
AC_SUBST(LIBVERSION)

AC_CANONICAL_HOST
case "$host" in
    *mingw32* )
        LDFLAGS="${LDFLAGS} -no-undefined"
        ;;
esac

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CXX

CHECK_VISIBILITY

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_BIGENDIAN

dnl Use C++ for tests.
AC_LANG_CPLUSPLUS

dnl Check for nothrow allocator.
MY_CHECK_EXCEPTIONS

dnl Find libraries
SID2_LIB_CHECKS
SID2_FIND_LIBSIDPLAY2

AC_CONFIG_FILES([
Makefile
include/Makefile
include/sidplay/Makefile
include/sidplay/utils/Makefile
src/Makefile
src/ini/Makefile
src/MD5/Makefile
unix/Makefile
unix/libsidutils.spec
unix/libsidutils.qpg
unix/libsidutils.pc
])

AC_OUTPUT
