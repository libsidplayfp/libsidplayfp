# Bun Demo: Play SID and Export WAV

This demo shows how to drive the `libsidplayfp` WebAssembly build with Bun to
render a SID tune into a WAV file.

## Prerequisites

- Run `./webassembly/build.sh` so that `webassembly/dist/` contains the latest
  `libsidplayfp.js/.wasm` bundle.
- Bun v1.0+ available on your PATH (`bun --version`).

## Usage

```bash
bun run webassembly/demo/demo.ts \
  /home/chris/dev/c64/sid/hvsc/C64Music/MUSICIANS/B/Blues_Muz/Team_Patrol.sid \
  Team_Patrol.wav \
  90
```

Arguments:

1. Path to the SID file.
2. (Optional) Output WAV filename. Defaults to `Team_Patrol.wav` in the current
   directory.
3. (Optional) Duration in seconds to render. Defaults to 60 seconds.

The script logs basic tune information, renders the requested duration using the
WebAssembly player, and saves a 16-bit stereo PCM WAV file.
