m4_define([lib_major], [0])
m4_define([lib_minor], [1])
m4_define([lib_level], [0])
m4_define([lib_version], [lib_major.lib_minor.lib_level])

AC_INIT([libsidplayfp], [lib_version])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([sidplayfp/sidplay2.cpp])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE

LIB_MAJOR=lib_major
LIB_MINOR=lib_minor
LIB_LEVEL=lib_level
AC_SUBST([LIB_MAJOR])
AC_SUBST([LIB_MINOR])
AC_SUBST([LIB_LEVEL])

AC_CANONICAL_HOST
case "$host" in
    *linux*|*-k*bsd*-gnu|*mingw*)
        hardsid=yes
        HARDSIDLIB=-lsidplayfp-hardsid
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
LT_INIT([win32-dll])

dnl Checks for programs.
AC_PROG_CXX

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

# if you don’t change the interface at all just increase the
#     “interface revision” value;
# if you make backward-compatible changes (like adding interfaces),
#     increase the “current interface” value and the “older interface age”
#     value, reset “interface revision” to zero;
# if you make backward-incompatible changes, breaking ABI
#     (removing interfaces for instance), increase the “current interface”
#     value and # reset both “older interface age” and ”interface revision”
#     to zero.

# http://blog.flameeyes.eu/2009/04/22/shared-object-version

LIBSIDPLAYCUR=0
LIBSIDPLAYREV=0
LIBSIDPLAYAGE=0
LIBSIDPLAYVERSION=$LIBSIDPLAYCUR:$LIBSIDPLAYREV:$LIBSIDPLAYAGE

LIBRESIDCUR=0
LIBRESIDREV=0
LIBRESIDAGE=0
LIBRESIDVERSION=$LIBRESIDCUR:$LIBRESIDREV:$LIBRESIDAGE

LIBHARDSIDCUR=0
LIBHARDSIDREV=0
LIBHARDSIDAGE=0
LIBHARDSIDVERSION=$LIBHARDSIDCUR:$LIBHARDSIDREV:$LIBHARDSIDAGE

AC_MSG_CHECKING([for debugging])
AC_ARG_ENABLE(debug, [AS_HELP_STRING([--enable-debug], [compile for debugging @<:@no/yes/full, default=no@:>@])],
 [], [enable_debug=no])

if test "x$enable_debug" = "xno"; then
  AC_MSG_RESULT([Build without debugging messages])
else
  if test "x$enable_debug" = "xyes"; then
    debug_flags=-DDEBUG=1
    AC_MSG_RESULT([Build with debugging messages])
  else
    debug_flags=-DDEBUG=10
    AC_MSG_RESULT([Build with all debugging messages])
  fi
fi

AC_SUBST([debug_flags])


AC_ARG_ENABLE([inline],
  AS_HELP_STRING([--enable-inline],[enable inlining of functions [default=yes]]))

AS_IF([test x"$enable_inline" != "xno"],
  [RESID_INLINE=inline; RESID_INLINING=1],
  [RESID_INLINE=""; RESID_INLINING=0])


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

RESIDFP_FLAGS="$MSSE $NO_EXCEPTIONS"


# hack to avoid warnings
saveCPPFLAGS=$CPPFLAGS
CPPFLAGS="$CPPFLAGS $CXXFLAGS"

AC_CHECK_HEADERS([xmmintrin.h mmintrin.h])

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


dnl Enable branch prediction hints.
AC_ARG_ENABLE([branch-hints],
  [AS_HELP_STRING([--enable-branch-hints],
    [enable static branch prediction hints [default=yes]])],
  [],
  [enable_branch_hints=yes])

AS_IF([test "$enable_branch_hints" != no],
  [AC_SUBST([RESID_BRANCH_HINTS], [1])],
  [AC_SUBST([RESID_BRANCH_HINTS], [0])])


AC_CACHE_CHECK([for __builtin_expect], [resid_cv_builtin_expect],
  [AC_COMPILE_IFELSE([int main() { __builtin_expect(0, 0); }],
    [resid_cv_builtin_expect=yes], [resid_cv_builtin_expect=no])])

AS_IF([test "$resid_cv_builtin_expect" = no],
  [AC_SUBST([HAVE_BUILTIN_EXPECT], [0])],
  [AC_SUBST([HAVE_BUILTIN_EXPECT], [1])])


AC_SUBST(RESID_HAVE_BOOL)
AC_SUBST(RESID_USE_SSE)
AC_SUBST(RESID_INLINING)
AC_SUBST(RESID_INLINE)
AC_SUBST(RESIDFP_FLAGS)
AC_SUBST(LIBSIDPLAYVERSION)
AC_SUBST(PKGSIDPLAYVERSION)
AC_SUBST(LIBRESIDVERSION)
AC_SUBST(LIBHARDSIDVERSION)


AC_CONFIG_FILES([
Makefile
libsidplayfp.pc
sidplayfp/sidversion.h
builders/residfp-builder/residfp/siddefs-fp.h
builders/resid-builder/resid/siddefs.h
])

AC_OUTPUT
