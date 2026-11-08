# PlaySID software samples

PlaySID-compatible tunes use 2 Hülsbeck sample channels and Galway noise sequences. The sample engine reads C64 RAM and mixes through the first SID volume register. The fourth mute control mutes software samples.

The sample engine derives from Simon White's GPL-2.0-or-later XSID implementation in sidplay-libs 2.1.1. The original notices and history accompany the source.

The combined seeking integration adds the XSID events and channel state to playback checkpoints on the integration/reviewed-engine branch.
