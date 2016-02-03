
ACLOCAL_AMFLAGS = -I m4

AUTOMAKE_OPTIONS = foreign subdir-objects

AM_CPPFLAGS = \
$(FTDI_CFLAGS) \
@debug_flags@

EXTRA_DIST = \
docs/mainpage.dox

lib_LTLIBRARIES = libexsid.la

libexsid_la_SOURCES = \
exSID.c

libexsid_la_HEADERS = \
exSID.h

noinst_HEADERS = \
	exSID_defs.h

libexsid_ladir = $(includedir)

libexsid_la_LDFLAGS = -version-info $(LIBEXSIDVERSION)

#=========================================================

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libexsid.pc

#=========================================================
# docs
if HAVE_DOXYGEN
doc:
	$(DOXYGEN) Doxyfile

clean-local:
	rm -fr $(builddir)/docs/html
endif
