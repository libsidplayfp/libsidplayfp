AM_CPPFLAGS = \
-I$(top_srcdir)/include/headers \
-I$(top_srcdir)/include/sidplay \
-I$(top_srcdir)/include 

if HAVE_VISIBILITY
  AM_CXXFLAGS = -fvisibility=hidden -fvisibility-inlines-hidden
endif

noinst_LTLIBRARIES = libmos656x.la

libmos656x_la_SOURCES = mos656x.cpp mos656x.h
