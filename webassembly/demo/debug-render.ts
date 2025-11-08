// @ts-nocheck
import { readFile } from "fs/promises";
import path from "path";
import { fileURLToPath } from "url";
import createLibsidplayfp from "../dist/libsidplayfp.js";

async function main(): Promise<void> {
    const sidPath = process.argv[2];
    if (!sidPath) {
        console.error("Usage: bun run debug-render.ts <sidPath>");
        process.exit(1);
    }

    const sidBytes = new Uint8Array(await readFile(sidPath));
    console.log("Read SID bytes", sidBytes.length);

    const __dirname = path.dirname(fileURLToPath(import.meta.url));
    const module = await createLibsidplayfp({
        locateFile: (asset: string) => path.join(__dirname, "../dist", asset),
    });
    console.log("Module created");

    const player = new module.SidPlayerContext();
    console.log("Player constructed");

    if (!player.configure(44100, true)) {
        throw new Error(player.getLastError());
    }
    console.log("Configured");

    if (!player.loadSidBuffer(sidBytes)) {
        throw new Error(player.getLastError());
    }
    console.log("Loaded tune");

    const chunk = player.render(20000);
    console.log("Render chunk", chunk?.length);
}

main().catch((err) => {
    console.error(err);
    process.exit(1);
});
