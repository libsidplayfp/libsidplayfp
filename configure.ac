m4_define([lib_major], [1])
m4_define([lib_minor], [0])
m4_define([lib_level], [0alpha1])
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
        ;;
esac
case "$host" in
    *mingw32* )
        LDFLAGS="${LDFLAGS} -no-undefined"
        MINGW32=yes
        ;;
esac
AM_CONDITIONAL([HARDSID], [test "x$hardsid" = "xyes"])
AM_CONDITIONAL([MINGW32], [test "x$MINGW32" = "xyes"])

dnl Initialize libtool.
LT_INIT([win32-dll])

dnl Checks for programs.
AC_PROG_CXX

dnl Use C++ for tests.
AC_LANG([C++])

dnl check for hidden visibility support
AX_CHECK_COMPILE_FLAG([-fvisibility=hidden -fvisibility-inlines-hidden], [has_visibility=yes], [has_visibility=no])
AM_CONDITIONAL([HAVE_VISIBILITY], [test "x$has_visibility" = xyes])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_BIGENDIAN

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
#
# http://blog.flameeyes.eu/2009/04/22/shared-object-version
#
# if you don’t change the interface at all just increase the
#     “interface revision” value;
# if you make backward-compatible changes (like adding interfaces),
#     increase the “current interface” value and the “older interface age”
#     value, reset “interface revision” to zero;
# if you make backward-incompatible changes, breaking ABI
#     (removing interfaces for instance), increase the “current interface”
#     value and # reset both “older interface age” and ”interface revision”
#     to zero.

LIBSIDPLAYCUR=3
LIBSIDPLAYREV=0
LIBSIDPLAYAGE=0
LIBSIDPLAYVERSION=$LIBSIDPLAYCUR:$LIBSIDPLAYREV:$LIBSIDPLAYAGE

LIBSTILVIEWCUR=0
LIBSTILVIEWREV=0
LIBSTILVIEWAGE=0
LIBSTILVIEWVERSION=$LIBSTILVIEWCUR:$LIBSTILVIEWREV:$LIBSTILVIEWAGE

AC_MSG_CHECKING([for debugging])
AC_ARG_ENABLE(debug, [AS_HELP_STRING([--enable-debug], [compile for debugging @<:@no/yes/full, default=no@:>@])],
 [], [enable_debug=no])

if test "x$enable_debug" = "xno"; then
  AC_MSG_RESULT([Build without debugging messages])
  debug_flags=-DNDEBUG
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


# hack to avoid warnings
saveCPPFLAGS=$CPPFLAGS
CPPFLAGS="$CPPFLAGS $CXXFLAGS"

AC_CHECK_HEADERS([mmintrin.h])

CPPFLAGS=$saveCPPFLAGS

AC_CACHE_CHECK([for working bool], ac_cv_cxx_bool,
[AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([],
    [[bool flag;]])],
  [ac_cv_cxx_bool=yes],
  [ac_cv_cxx_bool=no])]
)

if test $ac_cv_cxx_bool = no; then
  RESID_HAVE_BOOL=0
else
  RESID_HAVE_BOOL=1
fi


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
  [AC_COMPILE_IFELSE([AC_LANG_SOURCE([int main() { __builtin_expect(0, 0); }])],
    [resid_cv_builtin_expect=yes], [resid_cv_builtin_expect=no])])

AS_IF([test "$resid_cv_builtin_expect" = no],
  [AC_SUBST([HAVE_BUILTIN_EXPECT], [0])],
  [AC_SUBST([HAVE_BUILTIN_EXPECT], [1])])


AC_CHECK_PROGS([DOXYGEN], [doxygen])
if test -z "$DOXYGEN"; then
  AC_MSG_WARN([Doxygen not found - continuing without Doxygen support])
fi

AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$DOXYGEN"])

AM_COND_IF([HAVE_DOXYGEN], [AC_CONFIG_FILES([Doxyfile])])


AC_ARG_ENABLE([testsuite],
  [AS_HELP_STRING([--enable-testsuite=PATH_TO_TESTSUITE],
    [enable Lorenz' testsuite [default=no]]
  )],
  [],
  [enable_testsuite=no]
)

AS_IF([test "x$enable_testsuite" != xno],
  AC_DEFINE_UNQUOTED([PC64_TESTSUITE],
    ["$enable_testsuite"],
    [Path to Lorenz' testsuite.]
  )
)

AM_CONDITIONAL([TESTSUITE], [test "x$enable_testsuite" != xno])

AC_SUBST(RESID_HAVE_BOOL)
AC_SUBST(RESID_INLINING)
AC_SUBST(RESID_INLINE)
AC_SUBST(LIBSIDPLAYVERSION)
AC_SUBST(LIBSTILVIEWVERSION)


AC_CONFIG_FILES([
Makefile
libsidplayfp.pc
libstilview.pc
sidplayfp/sidversion.h
builders/residfp-builder/residfp/siddefs-fp.h
builders/resid-builder/resid/siddefs.h
])

AC_OUTPUT
