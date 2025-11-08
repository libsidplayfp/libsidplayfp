// @ts-nocheck
import { Buffer } from "buffer";
import { readFile, writeFile } from "fs/promises";
import path from "path";
import process from "process";
import { fileURLToPath } from "url";
import createLibsidplayfp from "../dist/libsidplayfp.js";

interface CliArgs {
    sidPath: string;
    outputPath: string;
    seconds: number;
    roms: RomPaths;
}

interface RomPaths {
    dir?: string;
    kernal?: string;
    basic?: string;
    chargen?: string;
}

function parseArgs(argv: string[]): CliArgs {
    const positional: string[] = [];
    const flags = new Map<string, string>();

    for (let i = 2; i < argv.length; i++) {
        const token = argv[i];
        if (!token.startsWith("--")) {
            positional.push(token);
            continue;
        }

        const eqIndex = token.indexOf("=");
        if (eqIndex !== -1) {
            const key = token.slice(2, eqIndex);
            const value = token.slice(eqIndex + 1);
            flags.set(key, value);
            continue;
        }

        const key = token.slice(2);
        const value = argv[i + 1];
        if (!value || value.startsWith("--")) {
            console.error(`Flag --${key} requires a value`);
            process.exit(1);
        }
        flags.set(key, value);
        i += 1;
    }

    const sidPath = positional[0];
    const outputPath = positional[1] ?? "Team_Patrol.wav";
    const secondsArg = positional[2] ?? "60";

    if (!sidPath) {
        console.error("Usage: bun run demo.ts <path/to/file.sid> [output.wav] [seconds] [--rom-dir <dir>] [--rom-kernal <file>] [--rom-basic <file>] [--rom-chargen <file>]");
        process.exit(1);
    }

    const parsedSeconds = Number(secondsArg);
    if (!Number.isFinite(parsedSeconds) || parsedSeconds <= 0) {
        console.error("Duration must be a positive number of seconds");
        process.exit(1);
    }

    const roms: RomPaths = {
        dir: flags.get("rom-dir"),
        kernal: flags.get("rom-kernal"),
        basic: flags.get("rom-basic"),
        chargen: flags.get("rom-chargen"),
    };

    return { sidPath, outputPath, seconds: parsedSeconds, roms };
}

function encodeWav(samples: Int16Array, sampleRate: number, channels: number): Buffer {
    const bitsPerSample = 16;
    const byteRate = sampleRate * channels * (bitsPerSample / 8);
    const blockAlign = channels * (bitsPerSample / 8);
    const dataSize = samples.length * 2;
    const buffer = Buffer.allocUnsafe(44 + dataSize);
    let offset = 0;

    buffer.write("RIFF", offset); offset += 4;
    buffer.writeUInt32LE(36 + dataSize, offset); offset += 4;
    buffer.write("WAVE", offset); offset += 4;
    buffer.write("fmt ", offset); offset += 4;
    buffer.writeUInt32LE(16, offset); offset += 4; // PCM format chunk length
    buffer.writeUInt16LE(1, offset); offset += 2; // PCM format
    buffer.writeUInt16LE(channels, offset); offset += 2;
    buffer.writeUInt32LE(sampleRate, offset); offset += 4;
    buffer.writeUInt32LE(byteRate, offset); offset += 4;
    buffer.writeUInt16LE(blockAlign, offset); offset += 2;
    buffer.writeUInt16LE(bitsPerSample, offset); offset += 2;
    buffer.write("data", offset); offset += 4;
    buffer.writeUInt32LE(dataSize, offset); offset += 4;

    const pcmBytes = Buffer.from(samples.buffer, samples.byteOffset, samples.byteLength);
    pcmBytes.copy(buffer, offset);

    return buffer;
}

async function main(): Promise<void> {
    const { sidPath, outputPath, seconds, roms } = parseArgs(process.argv);
    const sidBytes = new Uint8Array(await readFile(sidPath));

    const __dirname = path.dirname(fileURLToPath(import.meta.url));
    const module = await createLibsidplayfp({
        locateFile: (asset: string) => path.join(__dirname, "../dist", asset),
    });

    const player = new module.SidPlayerContext();

    if (!player.configure(44100, true)) {
        throw new Error(`Failed to configure player: ${player.getLastError()}`);
    }

    await applySystemROMs(player, roms);

    if (!player.loadSidBuffer(sidBytes)) {
        throw new Error(`Failed to load SID: ${player.getLastError()}`);
    }

    const channels = player.getChannels();
    const sampleRate = player.getSampleRate();
    const targetFrames = Math.floor(sampleRate * seconds);
    const totalSamples = Math.max(targetFrames * channels, channels);

    const tuneInfo = player.getTuneInfo();
    if (tuneInfo) {
        console.log("Tune Info:", tuneInfo);
    }

    console.log(`Loaded SID from ${sidPath}`);
    console.log(`Rendering ${seconds} seconds at ${sampleRate} Hz (${channels} channels)`);

    const pcm = new Int16Array(totalSamples);
    let writeIndex = 0;
    const cyclesPerCall = 20000;
    const expectedIterations = Math.max(Math.ceil(targetFrames / cyclesPerCall), 1);
    const maxIterations = expectedIterations * 8;
    let silentIterations = 0;

    for (let i = 0; i < maxIterations && writeIndex < totalSamples; i++) {
        const chunk = player.render(cyclesPerCall);
        if (!chunk) {
            console.log("Render returned null (tune finished)");
            break;
        }

        if (chunk.length === 0) {
            silentIterations += 1;
            if (silentIterations > 8) {
                console.log("Renderer produced repeated silence; stopping early.");
                break;
            }
            continue;
        }

        silentIterations = 0;

        const copied = chunk.slice();
        const samplesToCopy = Math.min(copied.length, totalSamples - writeIndex);
        pcm.set(copied.subarray(0, samplesToCopy), writeIndex);
        writeIndex += samplesToCopy;
    }

    if (writeIndex === 0) {
        throw new Error("No audio data was produced by the renderer");
    }

    const renderedSamples = writeIndex;
    const pcmView = pcm.subarray(0, renderedSamples);

    const wav = encodeWav(pcmView, sampleRate, channels);
    await writeFile(outputPath, wav);

    const secondsWritten = (renderedSamples / channels) / sampleRate;
    console.log(`Wrote ${outputPath} (${secondsWritten.toFixed(2)} seconds)`);
    console.log("First 16 samples:", Array.from(pcmView.slice(0, 16)));
}

main().catch((err) => {
    console.error(err instanceof Error ? err.message : err);
    process.exit(1);
});

async function applySystemROMs(player: any, roms: RomPaths): Promise<void> {
    const romDir = roms.dir ? path.resolve(roms.dir) : undefined;

    const loadRom = async (filePath?: string | null): Promise<Uint8Array | null> => {
        if (!filePath) {
            return null;
        }

        try {
            const resolved = path.resolve(filePath);
            const bytes = await readFile(resolved);
            console.log(`Loaded ROM ${resolved} (${bytes.length} bytes)`);
            return new Uint8Array(bytes);
        } catch (error) {
            console.warn(`Warning: unable to load ROM from ${filePath}:`, error instanceof Error ? error.message : error);
            return null;
        }
    };

    const resolveWithFallback = async (explicitPath: string | undefined, fallbackName: string): Promise<Uint8Array | null> => {
        if (explicitPath) {
            return loadRom(explicitPath);
        }
        if (romDir) {
            return loadRom(path.join(romDir, fallbackName));
        }
        return null;
    };

    const [kernal, basic, chargen] = await Promise.all([
        resolveWithFallback(roms.kernal, "kernal.bin"),
        resolveWithFallback(roms.basic, "basic.bin"),
        resolveWithFallback(roms.chargen, "chargen.bin"),
    ]);

    if (!kernal && !basic && !chargen) {
        return;
    }

    if (!player.setSystemROMs(kernal ?? null, basic ?? null, chargen ?? null)) {
        throw new Error(`Failed to apply ROMs: ${player.getLastError()}`);
    }

    console.log("Custom system ROMs applied.");
}
