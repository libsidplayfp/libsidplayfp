
ACLOCAL_AMFLAGS = -I m4

AUTOMAKE_OPTIONS = foreign subdir-objects


if MINGW32
  W32_LDFLAGS = -no-undefined
endif

if HAVE_VISIBILITY
  AM_CXXFLAGS = -fvisibility=hidden -fvisibility-inlines-hidden
endif

AM_CPPFLAGS = -I $(top_builddir)/sidplayfp/builders/residfp-builder \
-I $(top_builddir)/sidplayfp/builders/residfp-builder/residfp \
-I $(top_builddir)/sidplayfp/builders/resid-builder \
-I $(top_builddir)/sidplayfp/builders/resid-builder/resid \
-I $(top_builddir)/sidplayfp \
-I $(top_srcdir)/sidplayfp \
@debug_flags@

#=========================================================
EXTRA_DIST = \
docs/mainpage.dox \
sidplayfp/psiddrv.a65 \
sidplayfp/builders/residfp-builder/residfp/AUTHORS \
sidplayfp/builders/residfp-builder/residfp/COPYING \
sidplayfp/builders/residfp-builder/residfp/README \
sidplayfp/builders/resid-builder/resid/AUTHORS \
sidplayfp/builders/resid-builder/resid/ChangeLog \
sidplayfp/builders/resid-builder/resid/COPYING \
sidplayfp/builders/resid-builder/resid/configure.in \
sidplayfp/builders/resid-builder/resid/INSTALL \
sidplayfp/builders/resid-builder/resid/Makefile.am \
sidplayfp/builders/resid-builder/resid/NEWS \
sidplayfp/builders/resid-builder/resid/README \
sidplayfp/builders/resid-builder/resid/README.VICE \
sidplayfp/builders/resid-builder/resid/THANKS \
sidplayfp/builders/resid-builder/resid/TODO \
$(noinst_DATA) \
$(noinst_SCRIPTS)

#=========================================================
# Libraries
lib_LTLIBRARIES = \
sidplayfp/libsidplayfp.la \
sidplayfp/libstilview.la

noinst_LTLIBRARIES = \
sidplayfp/builders/residfp-builder/residfp/libresidfp.la \
sidplayfp/builders/resid-builder/resid/libresid.la \
sidplayfp/builders/residfp-builder/libsidplayfp-residfp.la \
sidplayfp/builders/resid-builder/libsidplayfp-resid.la

if HARDSID
  noinst_LTLIBRARIES += sidplayfp/builders/hardsid-builder/libsidplayfp-hardsid.la
endif

#=========================================================
# libsidplayfp

if LIBGCRYPT
   MD5SRC = \
sidplayfp/utils/md5Gcrypt.h
else
   MD5SRC = \
sidplayfp/utils/md5Internal.h \
sidplayfp/utils/MD5/MD5.cpp \
sidplayfp/utils/MD5/MD5.h \
sidplayfp/utils/MD5/MD5_Defs.h
endif

sidplayfp_libsidplayfp_la_SOURCES = \
sidplayfp/EventScheduler.cpp \
sidplayfp/EventScheduler.h \
sidplayfp/player.cpp \
sidplayfp/player.h \
sidplayfp/psiddrv.cpp \
sidplayfp/psiddrv.h \
sidplayfp/psiddrv.bin \
sidplayfp/mixer.cpp \
sidplayfp/mixer.h \
sidplayfp/poweron.bin \
sidplayfp/reloc65.cpp \
sidplayfp/reloc65.h \
sidplayfp/sidcxx11.h \
sidplayfp/sidmd5.h \
sidplayfp/sidmemory.h \
sidplayfp/SidInfoImpl.h \
sidplayfp/romCheck.h \
sidplayfp/sidemu.cpp \
sidplayfp/sidemu.h \
sidplayfp/sidendian.h \
sidplayfp/stringutils.h \
sidplayfp/c64/Banks/Bank.h \
sidplayfp/c64/c64cpu.h \
sidplayfp/c64/c64cia.h \
sidplayfp/c64/c64env.h \
sidplayfp/c64/c64sid.h \
sidplayfp/c64/c64vic.h \
sidplayfp/c64/c64.cpp \
sidplayfp/c64/c64.h \
sidplayfp/c64/component.h \
sidplayfp/c64/mmu.cpp \
sidplayfp/c64/mmu.h \
sidplayfp/c64/Banks/ColorRAMBank.h \
sidplayfp/c64/Banks/DisconnectedBusBank.h \
sidplayfp/c64/Banks/ExtraSidBank.h \
sidplayfp/c64/Banks/IOBank.h \
sidplayfp/c64/Banks/NullSid.h \
sidplayfp/c64/Banks/SidBank.h \
sidplayfp/c64/Banks/SystemRAMBank.h \
sidplayfp/c64/Banks/SystemROMBanks.h \
sidplayfp/c64/Banks/ZeroRAMBank.h \
sidplayfp/c64/VIC_II/mos656x.cpp \
sidplayfp/c64/VIC_II/mos656x.h \
sidplayfp/c64/VIC_II/lightpen.h \
sidplayfp/c64/VIC_II/sprites.h \
sidplayfp/c64/CPU/flags.h \
sidplayfp/c64/CPU/mos6510.cpp \
sidplayfp/c64/CPU/mos6510.h \
sidplayfp/c64/CPU/mos6510debug.cpp \
sidplayfp/c64/CPU/mos6510debug.h \
sidplayfp/c64/CPU/opcodes.h \
sidplayfp/c64/CIA/mos6526.cpp \
sidplayfp/c64/CIA/mos6526.h \
sidplayfp/c64/CIA/timer.cpp \
sidplayfp/c64/CIA/timer.h \
sidplayfp/c64/CIA/tod.cpp \
sidplayfp/c64/CIA/tod.h \
sidplayfp/sidplayfp/sidplayfp.cpp \
sidplayfp/sidplayfp/sidbuilder.cpp \
sidplayfp/sidplayfp/SidConfig.cpp \
sidplayfp/sidplayfp/SidTune.cpp \
sidplayfp/sidtune/MUS.cpp \
sidplayfp/sidtune/MUS.h \
sidplayfp/sidtune/p00.cpp \
sidplayfp/sidtune/p00.h \
sidplayfp/sidtune/prg.cpp \
sidplayfp/sidtune/prg.h \
sidplayfp/sidtune/PSID.cpp \
sidplayfp/sidtune/PSID.h \
sidplayfp/sidtune/sidplayer.bin \
sidplayfp/sidtune/SidTuneBase.cpp \
sidplayfp/sidtune/SidTuneBase.h \
sidplayfp/sidtune/SidTuneCfg.h \
sidplayfp/sidtune/SidTuneInfoImpl.h \
sidplayfp/sidtune/SidTuneTools.cpp \
sidplayfp/sidtune/SidTuneTools.h \
sidplayfp/sidtune/SmartPtr.h \
sidplayfp/utils/iMd5.h \
sidplayfp/utils/iniParser.cpp \
sidplayfp/utils/iniParser.h \
sidplayfp/utils/md5Factory.h \
sidplayfp/utils/SidDatabase.cpp \
$(MD5SRC)

sidplayfp_libsidplayfp_la_LDFLAGS = -version-info $(LIBSIDPLAYVERSION) $(W32_LDFLAGS)

sidplayfp_libsidplayfp_ladir = $(includedir)/sidplayfp

sidplayfp_libsidplayfp_la_HEADERS = \
sidplayfp/sidplayfp/siddefs.h \
sidplayfp/sidplayfp/event.h \
sidplayfp/sidplayfp/SidConfig.h \
sidplayfp/sidplayfp/SidInfo.h \
sidplayfp/sidplayfp/SidTuneInfo.h \
sidplayfp/sidplayfp/sidbuilder.h \
sidplayfp/sidplayfp/sidplayfp.h \
sidplayfp/sidplayfp/SidTune.h \
sidplayfp/utils/SidDatabase.h

nodist_sidplayfp_libsidplayfp_la_HEADERS = \
sidplayfp/sidversion.h

sidplayfp_libsidplayfp_la_LIBADD = \
sidplayfp/builders/residfp-builder/libsidplayfp-residfp.la \
sidplayfp/builders/resid-builder/libsidplayfp-resid.la \
$(LIBGCRYPT_LIBS)

if HARDSID
  sidplayfp_libsidplayfp_la_LIBADD += sidplayfp/builders/hardsid-builder/libsidplayfp-hardsid.la
endif

sidplayfp_libsidplayfp_la_CPPFLAGS = $(LIBGCRYPT_CFLAGS) $(AM_CPPFLAGS)

#=========================================================
# residfp

sidplayfp_builders_residfp_builder_residfp_libresidfp_la_SOURCES = \
sidplayfp/builders/residfp-builder/residfp/array.h \
sidplayfp/builders/residfp-builder/residfp/Dac.cpp \
sidplayfp/builders/residfp-builder/residfp/Dac.h \
sidplayfp/builders/residfp-builder/residfp/Integrator.cpp \
sidplayfp/builders/residfp-builder/residfp/Integrator.h \
sidplayfp/builders/residfp-builder/residfp/EnvelopeGenerator.cpp \
sidplayfp/builders/residfp-builder/residfp/EnvelopeGenerator.h \
sidplayfp/builders/residfp-builder/residfp/ExternalFilter.cpp \
sidplayfp/builders/residfp-builder/residfp/ExternalFilter.h \
sidplayfp/builders/residfp-builder/residfp/FilterModelConfig.cpp \
sidplayfp/builders/residfp-builder/residfp/FilterModelConfig.h \
sidplayfp/builders/residfp-builder/residfp/Filter.cpp \
sidplayfp/builders/residfp-builder/residfp/Filter.h \
sidplayfp/builders/residfp-builder/residfp/Filter8580.cpp \
sidplayfp/builders/residfp-builder/residfp/Filter8580.h \
sidplayfp/builders/residfp-builder/residfp/Filter6581.cpp \
sidplayfp/builders/residfp-builder/residfp/Filter6581.h \
sidplayfp/builders/residfp-builder/residfp/OpAmp.cpp \
sidplayfp/builders/residfp-builder/residfp/OpAmp.h \
sidplayfp/builders/residfp-builder/residfp/Potentiometer.h \
sidplayfp/builders/residfp-builder/residfp/SID.cpp \
sidplayfp/builders/residfp-builder/residfp/SID.h \
sidplayfp/builders/residfp-builder/residfp/Spline.cpp \
sidplayfp/builders/residfp-builder/residfp/Spline.h \
sidplayfp/builders/residfp-builder/residfp/Voice.h \
sidplayfp/builders/residfp-builder/residfp/WaveformCalculator.cpp \
sidplayfp/builders/residfp-builder/residfp/WaveformCalculator.h \
sidplayfp/builders/residfp-builder/residfp/WaveformGenerator.cpp \
sidplayfp/builders/residfp-builder/residfp/WaveformGenerator.h \
sidplayfp/builders/residfp-builder/residfp/resample/Resampler.h \
sidplayfp/builders/residfp-builder/residfp/resample/ZeroOrderResampler.h \
sidplayfp/builders/residfp-builder/residfp/resample/SincResampler.cpp \
sidplayfp/builders/residfp-builder/residfp/resample/SincResampler.h \
sidplayfp/builders/residfp-builder/residfp/resample/TwoPassSincResampler.h \
sidplayfp/builders/residfp-builder/residfp/version.cc

#=========================================================
# resid

BUILT_SOURCES = $(noinst_DATA:.dat=.h)

sidplayfp_builders_resid_builder_resid_libresid_la_SOURCES = \
sidplayfp/builders/resid-builder/resid/dac.h \
sidplayfp/builders/resid-builder/resid/sid.h \
sidplayfp/builders/resid-builder/resid/voice.h \
sidplayfp/builders/resid-builder/resid/wave.h \
sidplayfp/builders/resid-builder/resid/envelope.h \
sidplayfp/builders/resid-builder/resid/filter.h \
sidplayfp/builders/resid-builder/resid/extfilt.h \
sidplayfp/builders/resid-builder/resid/pot.h \
sidplayfp/builders/resid-builder/resid/spline.h \
sidplayfp/builders/resid-builder/resid/dac.cc \
sidplayfp/builders/resid-builder/resid/sid.cc \
sidplayfp/builders/resid-builder/resid/voice.cc \
sidplayfp/builders/resid-builder/resid/wave.cc \
sidplayfp/builders/resid-builder/resid/envelope.cc \
sidplayfp/builders/resid-builder/resid/filter.cc \
sidplayfp/builders/resid-builder/resid/extfilt.cc \
sidplayfp/builders/resid-builder/resid/pot.cc \
sidplayfp/builders/resid-builder/resid/resid-config.h \
sidplayfp/builders/resid-builder/resid/version.cc \
$(noinst_DATA:.dat=.h)

noinst_DATA = \
sidplayfp/builders/resid-builder/resid/wave6581_PST.dat \
sidplayfp/builders/resid-builder/resid/wave6581_PS_.dat \
sidplayfp/builders/resid-builder/resid/wave6581_P_T.dat \
sidplayfp/builders/resid-builder/resid/wave6581__ST.dat \
sidplayfp/builders/resid-builder/resid/wave8580_PST.dat \
sidplayfp/builders/resid-builder/resid/wave8580_PS_.dat \
sidplayfp/builders/resid-builder/resid/wave8580_P_T.dat \
sidplayfp/builders/resid-builder/resid/wave8580__ST.dat

noinst_SCRIPTS = sidplayfp/builders/resid-builder/resid/samp2src.pl

#.dat.h:
#	$(PERL) $(srcdir)/samp2src.pl $* $< $(srcdir)/$@

#=========================================================
# builders
sidplayfp_builders_residfp_builder_libsidplayfp_residfp_ladir = $(includedir)/sidplayfp/builders
sidplayfp_builders_residfp_builder_libsidplayfp_residfp_la_HEADERS = \
sidplayfp/builders/residfp-builder/residfp.h

sidplayfp_builders_residfp_builder_libsidplayfp_residfp_la_SOURCES = \
sidplayfp/builders/residfp-builder/residfp-builder.cpp \
sidplayfp/builders/residfp-builder/residfp-emu.cpp \
sidplayfp/builders/residfp-builder/residfp-emu.h

sidplayfp_builders_residfp_builder_libsidplayfp_residfp_la_LIBADD = \
sidplayfp/builders/residfp-builder/residfp/libresidfp.la


sidplayfp_builders_resid_builder_libsidplayfp_resid_ladir = $(includedir)/sidplayfp/builders
sidplayfp_builders_resid_builder_libsidplayfp_resid_la_HEADERS = \
sidplayfp/builders/resid-builder/resid.h

sidplayfp_builders_resid_builder_libsidplayfp_resid_la_SOURCES = \
sidplayfp/builders/resid-builder/resid-builder.cpp \
sidplayfp/builders/resid-builder/resid-emu.cpp \
sidplayfp/builders/resid-builder/resid-emu.h

builders_resid_builder_libsidplayfp_resid_la_LIBADD = \
sidplayfp/builders/resid-builder/resid/libresid.la

if HARDSID
sidplayfp_builders_hardsid_builder_libsidplayfp_hardsid_ladir = $(includedir)/sidplayfp/builders
sidplayfp_builders_hardsid_builder_libsidplayfp_hardsid_la_HEADERS = \
sidplayfp/builders/hardsid-builder/hardsid.h

if MINGW32
  hardsid_src = sidplayfp/builders/hardsid-builder/hardsid-emu-win.cpp
else
  hardsid_src = sidplayfp/builders/hardsid-builder/hardsid-emu-unix.cpp
endif

sidplayfp_builders_hardsid_builder_libsidplayfp_hardsid_la_SOURCES = \
$(hardsid_src) \
sidplayfp/builders/hardsid-builder/hardsid-builder.cpp  \
sidplayfp/builders/hardsid-builder/hardsid-emu.h
endif

#=========================================================
# libstilview
sidplayfp_libstilview_la_SOURCES = \
sidplayfp/utils/STILview/stil.cpp

sidplayfp_libstilview_la_LDFLAGS = -version-info $(LIBSTILVIEWVERSION) $(W32_LDFLAGS)

sidplayfp_libstilview_ladir = $(includedir)/stilview

sidplayfp_libstilview_la_HEADERS = \
sidplayfp/utils/STILview/stil.h \
sidplayfp/utils/STILview/stildefs.h

#=========================================================
# docs
if HAVE_DOXYGEN
doc:
	$(DOXYGEN) Doxyfile

clean-local:
	rm -fr $(builddir)/docs/html
endif

#=========================================================
# test
if TESTSUITE
noinst_PROGRAMS = \
test/demo \
test/test \
sidplayfp/builders/residfp-builder/residfp/resample/test

test_demo_SOURCES = test/demo.cpp 

test_demo_LDADD = sidplayfp/libsidplayfp.la

test_test_SOURCES = test/test.cpp 

test_test_LDADD = sidplayfp/libsidplayfp.la

sidplayfp_builders_residfp_builder_residfp_resample_test_SOURCES = sidplayfp/builders/residfp-builder/residfp/resample/test.cpp

sidplayfp_builders_residfp_builder_residfp_resample_test_LDADD = sidplayfp/builders/residfp-builder/residfp/resample/SincResampler.lo
endif

#=========================================================

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libsidplayfp.pc libstilview.pc

#=========================================================
# Recreate psiddrv.bin, needs xa65

CLEANFILES = sidplayfp/psiddrv.o65 sidplayfp/psiddrv.bin

sidplayfp/psiddrv.o65:
	xa -R -G $(srcdir)/sidplayfp/psiddrv.a65 -o $@

sidplayfp/psiddrv.bin: sidplayfp/psiddrv.o65
	od -v -An -w8 -tx1 $< | sed -re 's/[[:alnum:]]+/0x&,/g' > sidplayfp/psiddrv.bin
