#!/usr/bin/env bash
set -euo pipefail

if [[ -n "${EMSDK:-}" && -f "${EMSDK}/emsdk_env.sh" ]]; then
  source "${EMSDK}/emsdk_env.sh" >/dev/null
elif [[ -f /emsdk/emsdk_env.sh ]]; then
  source /emsdk/emsdk_env.sh >/dev/null
elif [[ -f /opt/emsdk/emsdk_env.sh ]]; then
  source /opt/emsdk/emsdk_env.sh >/dev/null
else
  echo "emsdk environment script not found" >&2
  exit 1
fi

BUILD_ROOT=/tmp/libsidplayfp
OUTPUT_ROOT=/dist

rm -rf "${BUILD_ROOT}"
mkdir -p "${BUILD_ROOT}" "${OUTPUT_ROOT}"

GIT_URL="https://github.com/libsidplayfp/libsidplayfp"

git clone --depth 1 --recurse-submodules "${GIT_URL}" "${BUILD_ROOT}"
cd "${BUILD_ROOT}"

git submodule update --init --recursive

python3 /opt/libsidplayfp-wasm/webassembly/scripts/apply_thread_guards.py "${BUILD_ROOT}"

if grep -q 'AC_MSG_ERROR("pthreads not found")' configure.ac; then
    sed -i 's/AX_PTHREAD(\[\], \[AC_MSG_ERROR("pthreads not found")\])/AX_PTHREAD([], [])/' configure.ac
fi

autoreconf -vfi

emconfigure ./configure \
    --disable-shared \
    --enable-static \
    --without-gcrypt \
    --without-exsid \
    --without-usbsid \
    --disable-dependency-tracking \
    CFLAGS="-O3" \
    CXXFLAGS="-O3"

emmake make -j"$(nproc)"

cp /opt/libsidplayfp-wasm/bindings.cpp "${BUILD_ROOT}/"

em++ bindings.cpp src/.libs/libsidplayfp.a \
    -I./src \
    -I./src/sidplayfp \
    -I./src/sidtune \
    -I./src/builders/residfp-builder \
    --bind -O3 \
    -sMODULARIZE=1 \
    -sEXPORT_NAME=\"createLibsidplayfp\" \
    -sEXPORT_ES6=1 \
    -sALLOW_MEMORY_GROWTH=1 \
    -sDISABLE_EXCEPTION_CATCHING=0 \
    -sFORCE_FILESYSTEM=1 \
    -sASSERTIONS=1 \
    -sENVIRONMENT=web,node \
    -sDEFAULT_LIBRARY_FUNCS_TO_INCLUDE='[$ccall,$cwrap]' \
    -sEXPORTED_RUNTIME_METHODS='[FS,PATH,cwrap,ccall]' \
    -o "${OUTPUT_ROOT}/libsidplayfp.js"

cp COPYING "${OUTPUT_ROOT}/LICENSE"

cat <<'JSON' >"${OUTPUT_ROOT}/package.json"
{
  "name": "libsidplayfp-wasm",
  "version": "0.1.0",
  "description": "WebAssembly build of libsidplayfp with embind bindings for TypeScript projects.",
  "type": "module",
  "main": "./libsidplayfp.js",
  "module": "./libsidplayfp.js",
  "types": "./libsidplayfp.d.ts",
  "sideEffects": false
}
JSON

cat <<'DTS' >"${OUTPUT_ROOT}/libsidplayfp.d.ts"
export interface SidPlayerContextOptions {
  locateFile?(path: string, prefix?: string): string | URL;
  [key: string]: unknown;
}

export type SidTuneInfo = Record<string, unknown> | null;
export type EngineInfo = Record<string, unknown> | null;

export class SidPlayerContext {
  constructor();
  configure(sampleRate: number, stereo: boolean): boolean;
  loadSidBuffer(buffer: Uint8Array | ArrayBufferView): boolean;
  loadSidFile(path: string): boolean;
  selectSong(song: number): number;
  render(cycles: number): Int16Array | null;
  reset(): boolean;
  hasTune(): boolean;
  isStereo(): boolean;
  getChannels(): number;
  getSampleRate(): number;
  getTuneInfo(): SidTuneInfo;
  getEngineInfo(): EngineInfo;
  getLastError(): string;
  setSystemROMs(
    kernal?: Uint8Array | ArrayBufferView | null,
    basic?: Uint8Array | ArrayBufferView | null,
    chargen?: Uint8Array | ArrayBufferView | null
  ): boolean;
}

export interface LibsidplayfpWasmModule {
  FS: any;
  PATH: any;
  SidPlayerContext: typeof SidPlayerContext;
}

export default function createLibsidplayfp(moduleConfig?: SidPlayerContextOptions): Promise<LibsidplayfpWasmModule>;
DTS

cat <<'MD' >"${OUTPUT_ROOT}/README.md"
# libsidplayfp WebAssembly Build

This bundle is produced by the Docker build located in `webassembly/`. It exposes
`SidPlayerContext` through an embind wrapper so you can drive the C64 SID player
from JavaScript or TypeScript.

## Quick Start

```ts
import createLibsidplayfp from "./libsidplayfp.js";

const module = await createLibsidplayfp();
const player = new module.SidPlayerContext();

const response = await fetch("Team_Patrol.sid");
const buffer = new Uint8Array(await response.arrayBuffer());

if (!player.loadSidBuffer(buffer)) {
  throw new Error(player.getLastError());
}

const samples = player.render(20000); // Int16Array with PCM samples
```

The generated module supports both browsers and Node.js. When using filesystem
paths, mount files into Emscripten's virtual FS (`FS`).
MD

rm -rf "${BUILD_ROOT}"
