AM_CPPFLAGS = \
-I$(top_srcdir)/include/headers \
-I$(top_srcdir)/include/sidplay \
-I$(top_srcdir)/include

if HAVE_VISIBILITY
  AM_CXXFLAGS = -fvisibility=hidden -fvisibility-inlines-hidden
endif

EXTRA_DIST = cycle_based

noinst_LTLIBRARIES = libmos6510.la

libmos6510_la_SOURCES = mos6510.cpp mos6510.h conf6510.h opcodes.h
