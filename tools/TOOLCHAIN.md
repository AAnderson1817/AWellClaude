# Toolchain notes (verified end-to-end in this container, 2026-08-29)

Setup: `tools/setup.sh` (idempotent). Build: `tools/build.sh [web|linux|win|all]`.
Paths: raylib 5.5 at `/home/claude/raylib`, emsdk at `/home/claude/emsdk`
(emcc 6.0.8). Archives in `lib/` as `libraylib_{web,linux,win}.a`.

## Gotchas found the hard way

**1. raylib's Makefile overwrites `libraylib.a` per platform.**
Build each target, then copy the archive to a distinct name before building the next.
Already handled in `setup.sh`.

**2. `TakeScreenshot()` ignores the directory in its path.**
raylib does `TextFormat("%s/%s", CORE.Storage.basePath, GetFileName(fileName))`, so
`TakeScreenshot("shots/f01.png")` writes `./f01.png`. Use
`Image i = LoadImageFromScreen(); ExportImage(i, path); UnloadImage(i);` when you need
a real path. `tools/shots.sh` assumes the game does this.

**3. emcc 6.0.8 `-s SINGLE_FILE=1` embeds the wasm as a UTF-8 JS string of code points
0-255, NOT base64.** The decoder is `c=bin.charCodeAt(i); o[i]=~c>>8&c`. The host page
MUST declare `<meta charset="utf-8">`, or the browser decodes the script as
windows-1252, every byte >= 0x80 becomes two characters, and the wasm fails to compile
with a misleading error:
  `CompileError: WebAssembly.instantiate(): unknown type form: 2` or
  `section was shorter than expected size (39107 bytes expected, 369 decoded)`
This is web-only — the native build is unaffected, so only verification step B catches
it. `tools/web/wrap.sh` emits the charset meta. The Artifact host skeleton supplies one
too, so published artifacts are fine.

**4. Chromium is at `/opt/pw-browsers/chromium-1194/chrome-linux/chrome`**, not
`/opt/pw-browsers/chromium/...`. Playwright is only installed globally, and ESM ignores
`NODE_PATH`, so `node_modules/playwright` is symlinked into the repo.

**5. `file://` pages cannot `fetch()` sibling files** (CORS). Everything must be inlined
into one HTML file — which is what we want for the Artifact anyway.

**6. Harmless noise in the headless browser:** `NPOT textures extension not found`,
`GPU stall due to ReadPixels`, and `ERR_CONNECTION_RESET` for Google Fonts.

## Verification loop
```bash
tools/build.sh all
tools/shots.sh 5,40,120                       # A: native, then Read the PNGs
tools/web/wrap.sh build/game.js build/play.html "Title"
node tools/web/drive.mjs build/play.html shots/web "wait:3000;shot:boot;down:KeyD;wait:900;shot:run;up:KeyD"
```
`drive.mjs` plan steps: `wait:MS`, `shot:NAME`, `key:CODE:MS`, `hold:A+B:MS`,
`down:CODE`, `up:CODE`.
