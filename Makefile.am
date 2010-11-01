
ACLOCAL_AMFLAGS = -I m4

AUTOMAKE_OPTIONS = foreign subdir-objects

if HAVE_VISIBILITY
  AM_CXXFLAGS = -fvisibility=hidden -fvisibility-inlines-hidden
endif

AM_CPPFLAGS = -I $(top_builddir)/builders/residfp-builder \
-I $(top_builddir)/builders/residfp-builder/residfp \
-I $(top_builddir)/builders/resid-builder \
-I $(top_builddir)/builders/resid-builder/resid

#=========================================================
EXTRA_DIST = \
sidplayfp/psiddrv.bin \
sidplayfp/kernal.bin \
sidplayfp/poweron.bin \
sidplayfp/basic.bin \
sidplayfp/char.bin \
sidplayfp/c64/c64cia.h \
sidplayfp/c64/c64vic.h \
sidplayfp/c64/c64xsid.h \
sidplayfp/mos6510/cycle_based \
builders/residfp-builder/residfp/AUTHORS \
builders/residfp-builder/residfp/ChangeLog \
builders/residfp-builder/residfp/COPYING \
builders/residfp-builder/residfp/INSTALL \
builders/residfp-builder/residfp/README \
builders/residfp-builder/residfp/README.VICE \
builders/residfp-builder/residfp/NEWS \
builders/resid-builder/resid/AUTHORS \
builders/resid-builder/resid/ChangeLog \
builders/resid-builder/resid/COPYING \
builders/resid-builder/resid/configure.in \
builders/resid-builder/resid/INSTALL \
builders/resid-builder/resid/README \
builders/resid-builder/resid/README.VICE \
$(noinst_DATA)

#=========================================================
# Libraries
lib_LTLIBRARIES = \
sidplayfp/libsidplayfp.la \
builders/residfp-builder/libsidplayfp-residfp.la \
builders/resid-builder/libsidplayfp-resid.la

if HARDSID
  lib_LTLIBRARIES += builders/hardsid-builder/libsidplayfp-hardsid.la
endif

noinst_LTLIBRARIES = \
builders/residfp-builder/residfp/libresidfp.la \
builders/resid-builder/resid/libresid.la

#=========================================================
# libsidplayfp
sidplayfp_libsidplayfp_la_SOURCES = \
sidplayfp/config.cpp \
sidplayfp/event.cpp \
sidplayfp/player.cpp \
sidplayfp/player.h \
sidplayfp/psiddrv.cpp \
sidplayfp/mixer.cpp \
sidplayfp/reloc65.c \
sidplayfp/sidplay2.cpp \
sidplayfp/nullsid.h \
sidplayfp/psiddrv.a65 \
sidplayfp/sidenv.h \
sidplayfp/mos656x/mos656x.cpp \
sidplayfp/mos656x/mos656x.h \
sidplayfp/mos6510/mos6510.cpp \
sidplayfp/mos6510/mos6510.h \
sidplayfp/mos6510/conf6510.h \
sidplayfp/mos6510/opcodes.h \
sidplayfp/mos6526/mos6526.cpp \
sidplayfp/mos6526/mos6526.h \
sidplayfp/sid6526/sid6526.cpp \
sidplayfp/sid6526/sid6526.h \
sidplayfp/sidtune/IconInfo.cpp \
sidplayfp/sidtune/InfoFile.cpp \
sidplayfp/sidtune/MUS.cpp \
sidplayfp/sidtune/p00.cpp \
sidplayfp/sidtune/PP20.cpp \
sidplayfp/sidtune/PP20.h \
sidplayfp/sidtune/PP20_Defs.h \
sidplayfp/sidtune/prg.cpp \
sidplayfp/sidtune/PSID.cpp \
sidplayfp/sidtune/SidTune.cpp \
sidplayfp/sidtune/SidTuneCfg.h \
sidplayfp/sidtune/SidTuneTools.cpp \
sidplayfp/sidtune/SidTuneTools.h \
sidplayfp/xsid/xsid.cpp \
sidplayfp/xsid/xsid.h \
utils/iniParser.cpp \
utils/iniParser.h \
utils/SidDatabase.cpp \
utils/SidFilter.cpp \
utils/SidTuneMod.cpp \
utils/SidUsage.cpp \
utils/smm.cpp \
utils/smm0.h \
utils/MD5/MD5.cpp \
utils/MD5/MD5.h \
utils/MD5/MD5_Defs.h

sidplayfp_libsidplayfp_la_LDFLAGS = -version-info $(LIBSIDPLAYVERSION)

sidplayfp_libsidplayfp_ladir = $(includedir)/sidplayfp

sidplayfp_libsidplayfp_la_HEADERS = \
sidplayfp/sidconfig.h \
sidplayfp/Buffer.h \
sidplayfp/c64env.h \
sidplayfp/component.h \
sidplayfp/event.h \
sidplayfp/sid2types.h \
sidplayfp/sidbuilder.h \
sidplayfp/sidendian.h \
sidplayfp/sidplay2.h \
sidplayfp/SidTune.h \
sidplayfp/sidtypes.h \
sidplayfp/sidusage.h \
sidplayfp/SmartPtr.h \
utils/SidDatabase.h \
utils/SidFilter.h \
utils/SidTuneMod.h \
utils/SidUsage.h

#=========================================================
# residfp

if USE_SSE
  convolve_sse_src = builders/residfp-builder/residfp/convolve-sse.cc
else
  convolve_sse_src = 
endif

builders_residfp_builder_residfp_libresidfp_la_SOURCES = \
builders/residfp-builder/residfp/sid.h \
builders/residfp-builder/residfp/voice.h \
builders/residfp-builder/residfp/wave.h \
builders/residfp-builder/residfp/envelope.h \
builders/residfp-builder/residfp/filter.h \
builders/residfp-builder/residfp/extfilt.h \
builders/residfp-builder/residfp/pot.h \
builders/residfp-builder/residfp/sid.cc \
builders/residfp-builder/residfp/voice.cc \
builders/residfp-builder/residfp/wave.cc \
builders/residfp-builder/residfp/envelope.cc \
builders/residfp-builder/residfp/filter.cc \
builders/residfp-builder/residfp/extfilt.cc \
builders/residfp-builder/residfp/pot.cc \
builders/residfp-builder/residfp/version.cc \
builders/residfp-builder/residfp/convolve.cc \
$(convolve_sse_src)

builders_residfp_builder_residfp_libresidfp_la_CXXFLAGS = @RESIDFP_FLAGS@

#=========================================================
# resid

BUILT_SOURCES = $(noinst_DATA:.dat=.cc)

builders_resid_builder_resid_libresid_la_SOURCES = \
builders/resid-builder/resid/sid.h \
builders/resid-builder/resid/voice.h \
builders/resid-builder/resid/wave.h \
builders/resid-builder/resid/envelope.h \
builders/resid-builder/resid/filter.h \
builders/resid-builder/resid/extfilt.h \
builders/resid-builder/resid/pot.h \
builders/resid-builder/resid/spline.h \
builders/resid-builder/resid/sid.cc \
builders/resid-builder/resid/voice.cc \
builders/resid-builder/resid/wave.cc \
builders/resid-builder/resid/envelope.cc \
builders/resid-builder/resid/filter.cc \
builders/resid-builder/resid/extfilt.cc \
builders/resid-builder/resid/pot.cc \
builders/resid-builder/resid/version.cc \
$(noinst_DATA:.dat=.cc)

noinst_DATA = \
builders/resid-builder/resid/wave6581_PST.dat \
builders/resid-builder/resid/wave6581_PS_.dat \
builders/resid-builder/resid/wave6581_P_T.dat \
builders/resid-builder/resid/wave6581__ST.dat \
builders/resid-builder/resid/wave8580_PST.dat \
builders/resid-builder/resid/wave8580_PS_.dat \
builders/resid-builder/resid/wave8580_P_T.dat \
builders/resid-builder/resid/wave8580__ST.dat

#=========================================================
# builders
builders_residfp_builder_libsidplayfp_residfp_ladir = $(includedir)/sidplayfp/builders
builders_residfp_builder_libsidplayfp_residfp_la_HEADERS = \
builders/residfp-builder/residfp.h

builders_residfp_builder_libsidplayfp_residfp_la_SOURCES = \
builders/residfp-builder/residfp.cpp \
builders/residfp-builder/residfp-builder.cpp \
builders/residfp-builder/residfp-emu.h

builders_residfp_builder_libsidplayfp_residfp_la_LIBADD = \
builders/residfp-builder/residfp/libresidfp.la

builders_residfp_builder_libsidplayfp_residfp_la_LDFLAGS = -version-info $(LIBRESIDVERSION)


builders_resid_builder_libsidplayfp_resid_ladir = $(includedir)/sidplayfp/builders
builders_resid_builder_libsidplayfp_resid_la_HEADERS = \
builders/resid-builder/resid.h

builders_resid_builder_libsidplayfp_resid_la_SOURCES = \
builders/resid-builder/resid.cpp \
builders/resid-builder/resid-builder.cpp \
builders/resid-builder/resid-emu.h

builders_resid_builder_libsidplayfp_resid_la_LIBADD = \
builders/resid-builder/resid/libresid.la

builders_resid_builder_libsidplayfp_resid_la_LDFLAGS = -version-info $(LIBRESIDVERSION)


builders_hardsid_builder_libsidplayfp_hardsid_ladir = $(includedir)/sidplayfp/builders
builders_hardsid_builder_libsidplayfp_hardsid_la_HEADERS = \
builders/hardsid-builder/hardsid.h

if MINGW32
  hardsid_src = builders/hardsid-builder/hardsid-win.cpp
else
  hardsid_src = builders/hardsid-builder/hardsid-unix.cpp
endif

builders_hardsid_builder_libsidplayfp_hardsid_la_SOURCES = \
$(hardsid_src) \
builders/hardsid-builder/hardsid-builder.cpp  \
builders/hardsid-builder/hardsid-emu.h

builders_hardsid_builder_libsidplayfp_hardsid_la_LDFLAGS = -version-info $(LIBHARDSIDVERSION)

#=========================================================
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libsidplayfp.pc

#=========================================================
# Remove SVN directories from any subdirs that get
# included completely (e.g. win, directx,...).

dist-hook:
	rm -rf `find $(distdir) -path \*/.svn`
