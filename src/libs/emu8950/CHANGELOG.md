# v1.2.2 (2026-10-08)
BugFix: thanks @madscient
- Fixed `OPL_calcStereo()` resampling L and R at different positions when the rate converter is active. (Issue[#5](https://github.com/digital-sound-antiques/emu8950/issues/5))
- Fixed ADPCM RAM/ROM buffers leaking when the chip type is switched away from Y8950. (Issue[#6](https://github.com/digital-sound-antiques/emu8950/issues/6))
- Fixed ADPCM keeping the last decoded value as a DC offset after playback has stopped. (Issue[#7](https://github.com/digital-sound-antiques/emu8950/issues/7))
- Fixed ADPCM playback not stopping when the START bit is cleared. (Issue[#8](https://github.com/digital-sound-antiques/emu8950/issues/8))
- Made internal functions static and removed the `FILE` type reference. (Issue[#9](https://github.com/digital-sound-antiques/emu8950/issues/9))

# v1.2.1 (2026-09-07)
- Reduced the table footprint from 138KB to 7KB.

# v1.2.0 (2026-07-22)
- Fixed reset function to fully clear runtime state.
- Added save/load state functionality.

# v1.1.3 (2024-06-15)
- Fixed the issue where key-on could fail when the attack envelope rate is around 14. (Issue[#3](https://github.com/digital-sound-antiques/emu8950/issues/3)).

# v1.1.2 (2022-09-14)
- Update minimum cmake version to 3.0

# v1.1.1 (2021-05-04)
- Fix the problem where BUF_RDY status bit stays 0 after status register is reset.

# v1.1.0 (2020-10-04)
- Support notesel, timer and CSM mode.

# v1.0.1 (2020-02-12)
- Remove deferred rhythm mode switching.
- Improve white noise emulation.

# v1.0.0 (2020-02-08)
- Rewrite FM engine based on emu2413 v1.3.0.
  - Improve emulation quality.
  - Support rhythm channels.
- Support ADPCM ROM.
- Semantic Versioning.

# v0.14 (2016-09-06)
- Support per-channel output.

# v0.13 (2003-09-19)
- Add OPL_setMask & OPL_toggleMask.

# v0.12 (2002-03-02)
- Remove OPL_init & OPL_close.

# v0.11 (2001-??-??)
- Add ADPCM emulation.

# v0.10 (2001-05-19)
- Test release.
