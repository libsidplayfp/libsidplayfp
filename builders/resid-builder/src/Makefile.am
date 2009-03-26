AM_CPPFLAGS = \
-I$(top_srcdir)/include/sidplay/builders \
-I$(top_srcdir)/../../resid \
-I$(top_srcdir)/../../libsidplay/include/sidplay \
-I$(top_srcdir)/../../libsidplay/include

if HAVE_VISIBILITY
  AM_CXXFLAGS = -fvisibility=hidden -fvisibility-inlines-hidden
endif

# The library.
lib_LTLIBRARIES = libresid-builder.la

libresid_builder_la_SOURCES = resid.cpp resid-builder.cpp resid-emu.h
libresid_builder_la_LIBADD  = $(top_srcdir)/../../resid/libresid.la

# We substitute the libtool-specific library version in configure.in.
libresid_builder_la_LDFLAGS = -version-info $(LIBVERSION)
