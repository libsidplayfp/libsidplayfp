dnl -------------------------------------------------------------------------
dnl Check whether C++ environment provides the "nothrow allocator".
dnl Will substitute @HAVE_EXCEPTIONS@ if test code compiles.
dnl -------------------------------------------------------------------------

AC_DEFUN([MY_CHECK_EXCEPTIONS],
[
    AC_MSG_CHECKING([whether exceptions are available])
    AC_CACHE_VAL(test_cv_have_exceptions,
    [
        AC_TRY_COMPILE(
            [#include <new.h>],
            [char* buf = new(nothrow) char[1024];],
            [test_cv_have_exceptions=yes],
            [test_cv_have_exceptions=no]
        )
    ])
    AC_MSG_RESULT($test_cv_have_exceptions)
    if test "$test_cv_have_exceptions" = yes; then
        AC_DEFINE(HAVE_EXCEPTIONS,,
            [Define if your C++ compiler implements exception-handling.]
        )
    fi
])
