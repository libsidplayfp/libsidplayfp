dnl Setup library CURRENT, REVISION and AGE
m4_define([LIBCUR],1)
m4_define([LIBREV],1)
m4_define([LIBAGE],0)

dnl Process this file with autoconf to produce a configure script.
AC_INIT([libsidplay],[2.LIBCUR.LIBREV])
AC_CONFIG_MACRO_DIR([unix])
AC_CONFIG_HEADER([include/config.h])
AC_CONFIG_SRCDIR([src/sidplay2.cpp])
AC_CONFIG_AUX_DIR([unix])
AM_INIT_AUTOMAKE([no-define])

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

dnl Initialize libtool.
LT_INIT([win32-dll])
#AC_LIBTOOL_WIN32_DLL
#AC_PROG_LIBTOOL

dnl Checks for header files.
AC_HEADER_STDC
dnl Check for AC99 headers
AC_CHECK_HEADERS(stdbool.h)
if test "$ac_cv_header_stdbool_h" = yes; then
    MY_SUBST_DEF(SID_HAVE_STDBOOL_H)
else
    MY_SUBST_UNDEF(SID_HAVE_STDBOOL_H)
fi

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_BIGENDIAN
if test $ac_cv_c_bigendian = yes; then
    MY_SUBST(SID_WORDS_ENDIANESS,SID_WORDS_BIGENDIAN)
else
    MY_SUBST(SID_WORDS_ENDIANESS,SID_WORDS_LITTLEENDIAN)
fi

AC_CHECK_SIZEOF([char], 1)
AC_CHECK_SIZEOF([short int], 2)
AC_CHECK_SIZEOF([int], 4)
AC_CHECK_SIZEOF([long int], 4)

MY_SUBST(SID_SIZEOF_CHAR,$ac_cv_sizeof_char)
MY_SUBST(SID_SIZEOF_SHORT_INT,$ac_cv_sizeof_short_int)
MY_SUBST(SID_SIZEOF_INT,$ac_cv_sizeof_int)
MY_SUBST(SID_SIZEOF_LONG_INT,$ac_cv_sizeof_long_int)

dnl Checks for library functions.
AC_CHECK_FUNCS([strncasecmp strcasecmp])

AC_SYS_LONG_FILE_NAMES

dnl Use C++ for other tests.
AC_LANG_CPLUSPLUS

dnl Checks for C++ library functions.
AC_CHECK_HEADERS([sstream])

dnl Check whether the compiler has bool.
MY_CHECK_BOOL
if test "$test_cv_have_bool" = yes; then
    MY_SUBST_DEF(SID_HAVE_BOOL)
else
    MY_SUBST_UNDEF(SID_HAVE_BOOL)
fi

dnl Check for exceptions.
MY_CHECK_EXCEPTIONS

dnl Check ANSI C++ ios features
MY_CHECK_IOS_BIN
MY_CHECK_IOS_OPENMODE

AC_ARG_WITH(sidbuilders,
    [AS_HELP_STRING([--with-sidbuilders=DIR],
        [where the sid builder libraries are to be installed])],
    [builders="$withval"],
    [builders="${libdir}/sidplay/builders"]
)
AC_SUBST([builders])

dnl Email addresses
AH_BOTTOM([/* Email addresses */
#define S_A_WHITE_EMAIL "sidplay2@yahoo.com"
])

AC_CONFIG_FILES([
Makefile
include/Makefile
include/sidplay/Makefile
include/headers/Makefile
src/Makefile
src/c64/Makefile
src/mos6510/Makefile
src/mos6526/Makefile
src/mos656x/Makefile
src/sid6526/Makefile
src/sidtune/Makefile
src/xsid/Makefile
unix/Makefile
unix/libsidplay2.spec
unix/libsidplay2.pc
include/sidplay/sidconfig.h
])

AC_OUTPUT
