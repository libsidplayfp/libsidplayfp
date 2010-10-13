AC_INIT([sidplay-libs], [2.1.1])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE

AC_CANONICAL_HOST
case "$host" in
    *linux*|*-k*bsd*-gnu|*mingw*)
        hardsid=yes
        HARDSIDLIB=-lhardsid-builder
        ;;
esac
case "$host" in
    *mingw32* )
        LDFLAGS="${LDFLAGS} -no-undefined"
        MINGW32=yes
        ;;
esac
AC_SUBST([HARDSIDLIB])
AM_CONDITIONAL([HARDSID], [test "x$hardsid" = "xyes"])
AM_CONDITIONAL([MINGW32], [test "x$MINGW32" = "xyes"])

dnl Initialize libtool.
#LT_INIT([win32-dll])
AC_LIBTOOL_WIN32_DLL

dnl Checks for programs.
AC_PROG_CXX
AC_PROG_LIBTOOL

dnl Use C++ for tests.
AC_LANG([C++])

CHECK_VISIBILITY

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_BIGENDIAN

AC_CHECK_SIZEOF([char])

if test $ac_cv_sizeof_char != 1; then
  AC_MSG_ERROR([code cannot work correctly on this platform as no real 8 bit data type supported!])
fi

AC_CHECK_SIZEOF(int, 4)

if test $ac_cv_sizeof_int -lt 4; then
  AC_MSG_ERROR([only 32 bit or better CPUs are supported])
fi

dnl Checks for library functions.
AC_CHECK_FUNCS([strncasecmp strcasecmp])

dnl Checks for C++ library functions.
AC_CHECK_HEADERS([sstream])

dnl Check for nothrow allocator.
MY_CHECK_EXCEPTIONS

dnl Check ANSI C++ ios features
MY_CHECK_IOS_BIN
MY_CHECK_IOS_OPENMODE

dnl libtool-style version-info number
LIBSIDPLAYCUR=1
LIBSIDPLAYREV=1
LIBSIDPLAYAGE=0
LIBSIDPLAYVERSION=$LIBSIDPLAYCUR:$LIBSIDPLAYREV:$LIBSIDPLAYAGE
PKGSIDPLAYVERSION=2.$LIBSIDPLAYCUR.$LIBSIDPLAYREV

LIBSIDUTILSCUR=0
LIBSIDUTILSREV=4
LIBSIDUTILSAGE=0
LIBSIDUTILSVERSION=$LIBSIDUTILSCUR:$LIBSIDUTILSREV:$LIBSIDUTILSAGE
PKGSIDUTILSVERSION=1.$LIBSIDUTILSCUR.$LIBSIDUTILSREV

LIBRESIDCUR=0
LIBRESIDREV=1
LIBRESIDAGE=0
LIBRESIDVERSION=$LIBRESIDCUR:$LIBRESIDREV:$LIBRESIDAGE

LIBHARDSIDCUR=0
LIBHARDSIDREV=1
LIBHARDSIDAGE=0
LIBHARDSIDVERSION=$LIBHARDSIDCUR:$LIBHARDSIDREV:$LIBHARDSIDAGE

AC_ARG_ENABLE([inline],
  AS_HELP_STRING([--enable-inline],[enable inlining of functions [default=yes]]))

AS_IF([test x"$enable_inline" != "xno"],
  [RESID_INLINE=inline],
  [RESID_INLINE=""])


AC_ARG_ENABLE([sse],
  AS_HELP_STRING([--enable-sse],[enable the use of SSE [default=yes]]))

AS_IF([test x"$enable_sse" != "xno" -a x"$GXX" = "xyes"],
  [MSSE="-msse"
    RESID_USE_SSE=1],
  [RESID_USE_SSE=0
    MSSE=""])

AM_CONDITIONAL([USE_SSE], [test $RESID_USE_SSE = 1])

if test x"$GXX" = "xyes"; then
  NO_EXCEPTIONS="-fno-exceptions"
else
  NO_EXCEPTIONS=""
fi

RESID_FLAGS="$MSSE $NO_EXCEPTIONS"


# hack to avoid warnings
saveCPPFLAGS=$CPPFLAGS
CPPFLAGS="$CPPFLAGS $CXXFLAGS"

AC_CHECK_HEADERS([xmmintrin.h])

CPPFLAGS=$saveCPPFLAGS

AC_CACHE_CHECK([for working bool], ac_cv_cxx_bool,
[AC_TRY_COMPILE(,
[
bool flag;
],
ac_cv_cxx_bool=yes, ac_cv_cxx_bool=no)])

if test $ac_cv_cxx_bool = no; then
  RESID_HAVE_BOOL=0
else
  RESID_HAVE_BOOL=1
fi

dnl Checks for library functions.
if test x"$host_os" != "xminix"; then
  AC_CHECK_FUNCS([logf expf])
fi

AC_MSG_CHECKING([if the logf prototype is present])
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([[#include <math.h>
    #include <stdio.h>]], [[printf("%d",logf);]])],
  [AC_MSG_RESULT(yes)
  HAVE_LOGF_PROTOTYPE=1],
  [AC_MSG_RESULT(no)
  HAVE_LOGF_PROTOTYPE=0])

AC_MSG_CHECKING([if the expf prototype is present])
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([[#include <math.h>
    #include <stdio.h>]], [[printf("%d",expf);]])],
  [AC_MSG_RESULT(yes)
  HAVE_EXPF_PROTOTYPE=1],
  [AC_MSG_RESULT(no)
  HAVE_EXPF_PROTOTYPE=0])

AC_SUBST(HAVE_LOGF_PROTOTYPE)
AC_SUBST(HAVE_EXPF_PROTOTYPE)

AC_SUBST(RESID_HAVE_BOOL)
AC_SUBST(RESID_USE_SSE)
AC_SUBST(RESID_INLINE)
AC_SUBST(RESID_FLAGS)
AC_SUBST(LIBSIDPLAYVERSION)
AC_SUBST(PKGSIDPLAYVERSION)
AC_SUBST(LIBSIDUTILSVERSION)
AC_SUBST(PKGSIDUTILSVERSION)
AC_SUBST(LIBRESIDVERSION)
AC_SUBST(LIBHARDSIDVERSION)


dnl Email addresses
AH_BOTTOM([/* Email addresses */
#define S_A_WHITE_EMAIL "sidplay2@yahoo.com"
])

AC_CONFIG_FILES([
Makefile
libsidplay2.spec
libsidplay2.pc
libsidutils.spec
libsidutils.pc
builders/resid-builder/resid/siddefs-fp.h
])

AC_OUTPUT
