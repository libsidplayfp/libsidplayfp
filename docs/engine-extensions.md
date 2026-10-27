# Checkpoints, seeking and PlaySID samples

The engine provides playback checkpoints, silent seeking, recognised tracker calls and PlaySID software samples.

## Building

Build and install libresidfp from the companion `performance/seek-support` branch. Set `PKG_CONFIG_PATH` to its installed pkg-config directory before configuring libsidplayfp. The `performance/seek-speedups` branch contains the checkpoint and seeking changes, `features/playsid` contains the software sample engine, and `integration/reviewed-engine` combines them with checkpoint support for the sample channels.

## PlaySID samples

Tunes with the PlaySID compatibility flag enable 2 Hülsbeck sample channels and Galway noise sequences. The channels read C64 RAM and mix their output through the first SID's volume register. The fourth mute control mutes the software samples. Checkpoints include their channel state and queued events.

The sample engine is adapted from Simon White's XSID implementation in sidplay-libs 2.1.1, distributed under GPL-2.0-or-later. Copyright and licence notices accompany the source in `src/xsid`. The original archive is available at https://deb.debian.org/debian/pool/main/s/sidplay-libs/sidplay-libs_2.1.1.orig.tar.gz.

## Checkpoints and seeking

`saveState()` saves the machine state to a byte vector. `restoreState()` restores it to the same player instance and tune load. The caller stores the vectors and chooses when to restore them.

Silent clocking advances digital SID state during seeking. PCM and tracker shortcuts check the player code and memory layout before running. A SID read or unsupported accelerated call requests a replay with the shortcut disabled. The caller renders a warm-up interval before resuming audible playback.

The accelerated player-call interpreter derives from Mihály 'Hermit' Horváth's libcRSID. See `player-call-attribution.md` for its source and permission notice.
