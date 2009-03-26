AM_CPPFLAGS = \
-I$(top_srcdir)/include/headers \
-I$(top_srcdir)/include/sidplay \
-I$(top_srcdir)/include 

if HAVE_VISIBILITY
  AM_CXXFLAGS = -fvisibility=hidden -fvisibility-inlines-hidden
endif

noinst_LTLIBRARIES = libmos6526.la

libmos6526_la_SOURCES = mos6526.cpp mos6526.h
