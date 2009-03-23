## Process this file with automake to create Makefile.in

resid_headers = siddefs.h sid.h voice.h wave.h envelope.h filter.h extfilt.h pot.h
resid_sources = sid.cc voice.cc wave.cc envelope.cc filter.cc extfilt.cc pot.cc version.cc

if INSTALL_RESID
lib_LTLIBRARIES = libresid.la
pkginclude_HEADERS = $(resid_headers)
libresid_la_SOURCES = $(resid_sources)
else
noinst_LTLIBRARIES = libresidc.la
noinst_HEADERS = $(resid_headers)
libresidc_la_SOURCES = $(resid_sources)

all-local:
	@if test ! -h libresid.la; then ln -f -s libresidc.la libresid.la; fi

endif

## Make sure these will be cleaned even when they're not built by default.
CLEANFILES = libresid.la libresidc.la
