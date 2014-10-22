
AUTOMAKE_OPTIONS = foreign subdir-objects

if ENABLE_TEST

AM_CPPFLAGS = \
-I $(top_builddir)/src/builders/residfp-builder/residfp

TESTS = \
TestEnvelopeGenerator \
TestSpline

check_PROGRAMS = $(TESTS)

TestEnvelopeGenerator_SOURCES = \
Main.cpp \
TestEnvelopeGenerator.cpp
TestEnvelopeGenerator_LDADD = -lUnitTest++ \
$(top_builddir)/src/builders/residfp-builder/residfp/EnvelopeGenerator.o \
$(top_builddir)/src/builders/residfp-builder/residfp/Dac.o

TestSpline_SOURCES = \
Main.cpp \
TestSpline.cpp 
TestSpline_LDADD = -lUnitTest++\
$(top_builddir)/src/builders/residfp-builder/residfp/Spline.o

endif
