AM_CPPFLAGS = \
-I$(top_srcdir)/../libsidplay/include/sidplay \
-I$(top_srcdir)/../libsidplay/include

if HAVE_VISIBILITY
  AM_CXXFLAGS = -fvisibility=hidden -fvisibility-inlines-hidden
endif

# The library.
noinst_LTLIBRARIES = libMD5.la

libMD5_la_SOURCES = MD5.cpp MD5.h MD5_Defs.h
