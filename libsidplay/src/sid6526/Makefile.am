AM_CPPFLAGS = \
-I$(top_srcdir)/include/headers \
-I$(top_srcdir)/include/sidplay \
-I$(top_srcdir)/include 

if HAVE_VISIBILITY
  AM_CXXFLAGS = -fvisibility=hidden -fvisibility-inlines-hidden
endif

noinst_LTLIBRARIES = libsid6526.la

libsid6526_la_SOURCES = sid6526.cpp sid6526.h
