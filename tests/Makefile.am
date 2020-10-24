
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
TestMUS \
TestMos6510

check_PROGRAMS = $(TESTS)

TestEnvelopeGenerator_SOURCES = \
Main.cpp \
TestEnvelopeGenerator.cpp \
$(top_srcdir)/src/builders/residfp-builder/residfp/EnvelopeGenerator.cpp \
$(top_srcdir)/src/builders/residfp-builder/residfp/Dac.cpp

TestWaveformGenerator_SOURCES = \
Main.cpp \
TestWaveformGenerator.cpp\
$(top_srcdir)/src/builders/residfp-builder/residfp/WaveformGenerator.cpp \
$(top_srcdir)/src/builders/residfp-builder/residfp/WaveformCalculator.cpp \
$(top_srcdir)/src/builders/residfp-builder/residfp/Dac.cpp

TestSpline_SOURCES = \
Main.cpp \
TestSpline.cpp \
$(top_srcdir)/src/builders/residfp-builder/residfp/Spline.cpp

TestDac_SOURCES = \
Main.cpp \
TestDac.cpp \
$(top_srcdir)/src/builders/residfp-builder/residfp/Dac.cpp

TestPSID_SOURCES = \
Main.cpp \
TestPSID.cpp
TestPSID_LDADD = $(top_builddir)/src/libsidplayfp.la

TestMUS_SOURCES = \
Main.cpp \
TestMUS.cpp
TestMUS_LDADD = $(top_builddir)/src/libsidplayfp.la

TestMos6510_SOURCES = \
Main.cpp \
TestMos6510.cpp \
$(top_srcdir)/src/c64/CPU/mos6510.cpp \
$(top_srcdir)/src/EventScheduler.cpp

endif
