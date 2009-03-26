AM_CPPFLAGS = \
-I$(top_srcdir)/include/headers \
-I$(top_srcdir)/include/sidplay \
-I$(top_srcdir)/include 

if HAVE_VISIBILITY
  AM_CXXFLAGS = -fvisibility=hidden -fvisibility-inlines-hidden
endif

noinst_LTLIBRARIES = libsidtune.la

libsidtune_la_SOURCES = IconInfo.cpp \
InfoFile.cpp \
MUS.cpp \
p00.cpp \
PP20.cpp \
PP20.h \
PP20_Defs.h \
prg.cpp \
PSID.cpp \
SidTune.cpp \
SidTuneCfg.h \
SidTuneTools.cpp \
SidTuneTools.h
