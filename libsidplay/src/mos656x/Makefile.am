AM_CPPFLAGS = \
-I$(top_builddir)/include/sidplay \
-I$(top_srcdir)/include \
-I$(top_srcdir)/include/sidplay

if HAVE_VISIBILITY
  AM_CXXFLAGS = -fvisibility=hidden -fvisibility-inlines-hidden
endif

noinst_LTLIBRARIES = libmos656x.la

libmos656x_la_SOURCES = mos656x.cpp mos656x.h
