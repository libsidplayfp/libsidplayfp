AM_CPPFLAGS = -I$(top_srcdir)/include/sidplay/utils

if HAVE_VISIBILITY
  AM_CXXFLAGS = -fvisibility=hidden -fvisibility-inlines-hidden
endif

# The library.
noinst_LTLIBRARIES = libini.la

libini_la_SOURCES = headings.h ini.cpp ini.h \
keys.h list.h

EXTRA_DIST=headings.i  keys.i list.i types.i
