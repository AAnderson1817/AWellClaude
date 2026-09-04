# Toolchain notes

Build and check paths re-verified in this container on 2026-09-04. The original
setup ran end-to-end on 2026-08-29; the rewritten `setup.sh` below has not yet been run
end-to-end (the toolchain it would produce already existed here).

Setup: `tools/setup.sh`. Build: `tools/build.sh [web|linux|win|all]`.
Both scripts locate the checkout from their own path and work from any directory.
Setup targets Debian/Ubuntu and needs root for missing apt packages. With system
packages already installed, set `AWELL_SKIP_SYSTEM_DEPS=1` to skip apt entirely.
Setup stops at the first failure and only creates `tools/.setup_complete` on success.
All of its output, including the failing command, goes to `tools/setup.log`; the
terminal shows one line saying it failed and where to look.

Dependency paths are shared through `tools/env.sh`; all overrides must be absolute:

| Variable | Default / meaning |
|---|---|
| `AWELL_TOOLCHAIN_DIR` | `<checkout>/.toolchain` |
| `RAYLIB` | `$AWELL_TOOLCHAIN_DIR/raylib/src` (raylib 5.5 source directory) |
| `EMSDK` | `$AWELL_TOOLCHAIN_DIR/emsdk` |
| `AWELL_LIB_DIR` | `<checkout>/lib`, with `libraylib_{web,linux,win}.a` |
| `EMSDK_VERSION` | `6.0.8`, used by setup |
| `CC` | `gcc`, used by the Linux build and headless checks; may carry a wrapper or flags (`ccache gcc`) |

To reuse the original toolchain, set `RAYLIB=/home/claude/raylib/src` and
`EMSDK=/home/claude/emsdk` for setup, build and check alike (or symlink them under
`.toolchain/`, which is ignored).

`tools/check.sh` runs input/frame-timing and unsigned-hash regressions with UBSan,
plus Python checks for route/probe failures. It needs only a C compiler supporting
UBSan, Python 3, and raylib's header; graphics/window calls are stubbed. These checks
do not validate rendering. `python3 tools/route.py` uses `build/game` and exits
nonzero for unreachable hops or a failed/empty game probe.

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
