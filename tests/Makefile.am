
AUTOMAKE_OPTIONS = foreign subdir-objects

EXTRA_DIST = utpp

if ENABLE_TEST

AM_CPPFLAGS = \
-I$(top_builddir)/src \
-I$(top_srcdir)/src

TESTS = \
TestPSID \
TestPSIDv5 \
TestMUS \
TestMos6510 \
TestMD5 \
TestXSID

check_PROGRAMS = $(TESTS)

TestXSID_SOURCES = TestXSID.cpp ../src/xsid/xsid.cpp ../src/EventScheduler.cpp

TestPSID_SOURCES = \
Main.cpp \
TestPSID.cpp
TestPSID_LDADD = $(top_builddir)/src/libsidplayfp.la

TestPSIDv5_SOURCES = \
Main.cpp \
TestPSIDv5.cpp
TestPSIDv5_LDADD = $(top_builddir)/src/libsidplayfp.la

TestMUS_SOURCES = \
Main.cpp \
TestMUS.cpp
TestMUS_LDADD = $(top_builddir)/src/libsidplayfp.la

TestMos6510_SOURCES = \
Main.cpp \
TestMos6510.cpp

TestMD5_SOURCES = \
Main.cpp \
TestMD5.cpp

endif
