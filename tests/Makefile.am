
AUTOMAKE_OPTIONS = foreign subdir-objects

if ENABLE_TEST

AM_CPPFLAGS = \
-I$(top_srcdir)/src \
-I$(top_builddir)/src/builders/residfp-builder/residfp

AM_LDFLAGS = @unittest_libs@

TESTS = \
TestEnvelopeGenerator \
TestWaveformGenerator \
TestSpline \
TestDac \
TestPSID \
TestMUS

check_PROGRAMS = $(TESTS)

TestEnvelopeGenerator_SOURCES = \
Main.cpp \
TestEnvelopeGenerator.cpp
TestEnvelopeGenerator_LDADD = \
$(top_builddir)/src/builders/residfp-builder/residfp/EnvelopeGenerator.o \
$(top_builddir)/src/builders/residfp-builder/residfp/Dac.o

TestWaveformGenerator_SOURCES = \
Main.cpp \
TestWaveformGenerator.cpp
TestWaveformGenerator_LDADD = \
$(top_builddir)/src/builders/residfp-builder/residfp/WaveformGenerator.o \
$(top_builddir)/src/builders/residfp-builder/residfp/WaveformCalculator.o \
$(top_builddir)/src/builders/residfp-builder/residfp/Dac.o

TestSpline_SOURCES = \
Main.cpp \
TestSpline.cpp
TestSpline_LDADD = $(top_builddir)/src/builders/residfp-builder/residfp/Spline.o

TestDac_SOURCES = \
Main.cpp \
TestDac.cpp
TestDac_LDADD = $(top_builddir)/src/builders/residfp-builder/residfp/Dac.o

TestPSID_SOURCES = \
Main.cpp \
TestPSID.cpp
TestPSID_LDADD = $(top_builddir)/src/libsidplayfp.la

TestMUS_SOURCES = \
Main.cpp \
TestMUS.cpp
TestMUS_LDADD = $(top_builddir)/src/libsidplayfp.la

endif
