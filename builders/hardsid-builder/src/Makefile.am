AM_CPPFLAGS = \
-I$(top_srcdir)/include/sidplay/builders \
$(LIBSIDPLAY2_CXXFLAGS)

if HAVE_VISIBILITY
  AM_CXXFLAGS = -fvisibility=hidden -fvisibility-inlines-hidden
endif

# The library.
lib_LTLIBRARIES = libhardsid-builder.la

if MINGW32
  hardsid_src = hardsid-win.cpp
else
  hardsid_src = hardsid-unix.cpp
endif

libhardsid_builder_la_SOURCES = \
$(hardsid_src) \
hardsid-builder.cpp \
hardsid-emu.h
libhardsid_builder_la_LIBADD  =

# We substitute the libtool-specific library version in configure.in.
libhardsid_builder_la_LDFLAGS = -version-info $(LIBVERSION)
