AC_INIT([libexsid], [1.1], [], [], [http://sourceforge.net/projects/sidplay-residfp/])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([exSID.c])
AM_INIT_AUTOMAKE

dnl Initialize libtool.
LT_INIT([win32-dll])

PKG_PROG_PKG_CONFIG


# debug options
AC_MSG_CHECKING([for debugging])
AC_ARG_ENABLE(debug, [AS_HELP_STRING([--enable-debug], [compile for debugging @<:@no/yes, default=no@:>@])],
 [], [enable_debug=no])

AS_IF([test "x$enable_debug" = "xno"],
  [AC_MSG_RESULT([Build without debugging messages]); debug_flags=-DNDEBUG],
  [AC_MSG_RESULT([Build with debugging messages]); debug_flags=-DDEBUG]
)

AC_SUBST([debug_flags])


# check for libraries
AC_ARG_WITH([ftd2xx],
    AS_HELP_STRING([--with-ftd2xx], [Enable use of ftd2xx]))

AS_IF([test "x$with_ftd2xx" = "xyes"],
      [AC_CHECK_HEADERS([ftd2xx.h])
	AS_IF(
		[test "x${ac_cv_header_ftd2xx_h}" = xyes],
		AC_CHECK_LIB([ftd2xx], [FT_Open])
	)]
)

AS_IF([test "x$ac_cv_lib_ftd2xx_FT_Open" = "xyes"],
	[AC_DEFINE([HAVE_FTD2XX], 1, [Define to 1 if you have libftd2xx (-lftd2xx).])],
	[PKG_CHECK_MODULES([FTDI],
		[libftdi1 >= 1.0],
		[AC_DEFINE([HAVE_FTDI], 1, [Define to 1 if you have libftdi1 (-lftdi1).])],
		[AC_MSG_WARN([$FTDI_PKG_ERRORS])]
	)]
)

AC_CACHE_CHECK([for __builtin_expect], [resid_cv_builtin_expect],
  [AC_COMPILE_IFELSE([AC_LANG_SOURCE([int main() { __builtin_expect(0, 0); }])],
    [exsid_cv_builtin_expect=yes], [exsid_cv_builtin_expect=no])]
)

AS_IF([test "$exsid_cv_builtin_expect" = yes],
  [AC_DEFINE([C_HAS_BUILTIN_EXPECT], 1, [Define to 1 if you have builtin_expect.])]
)


# doxygen
AC_CHECK_PROGS([DOXYGEN], [doxygen])
AS_IF([test -z "$DOXYGEN"],
  [AC_MSG_WARN([Doxygen not found - continuing without Doxygen support])],
  [AC_CHECK_PROG([DOT], [dot], [YES], [NO])]
)

AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$DOXYGEN"])

AM_COND_IF([HAVE_DOXYGEN], [AC_CONFIG_FILES([Doxyfile])])


dnl libtool-style version-info number
#
# https://autotools.io/libtool/version.html
#
# Always increase the revision value.
#
# Increase the current value whenever an interface has been added, removed or changed.
#
# Increase the age value only if the changes made to the ABI are backward compatible.

LIBEXSIDCUR=0
LIBEXSIDREV=0
LIBEXSIDAGE=0
LIBEXSIDVERSION=$LIBEXSIDCUR:$LIBEXSIDREV:$LIBEXSIDAGE

AC_SUBST(LIBEXSIDVERSION)

AC_CONFIG_FILES([
Makefile
libexsid.pc
])

AC_OUTPUT
