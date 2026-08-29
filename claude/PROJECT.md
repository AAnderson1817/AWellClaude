# Animal Well Redux — project state

A fresh session should be able to resume from this file plus `claude/DESIGN-LAW.md`
and `tools/TOOLCHAIN.md`.

## What this is
A playable vertical slice of an ORIGINAL 2D game built in C/raylib that reproduces the
*method* of Animal Well, not its content. 25 rooms, two verbs, no text, locked camera,
320x180. The law is in `claude/DESIGN-LAW.md` and is not negotiable. The research it
derives from is in `claude/animal-well-research-brief.md`.

## Phase state
- Phase 0 — Concept. IN PROGRESS. Premise slate generated, stress-tested and judged by
  workflow `phase0-premises`. GATE: the user picks a premise. Nothing premise-specific
  gets built before that.
- Phase 1 — Skeleton and feel. Foundation built ahead of the gate because none of it
  depends on the premise (see below). Still needs: the user's judgement on movement.
- Phases 2-5 — not started.

## Built so far (premise-independent)
- `src/aw.h` — all shared types and constants. GW/GH 320x180, TS 8, room 40x22 tiles at
  ROOM_Y=2 inside the 180px frame, WORLD 5x5 = 25 (L1, never rises).
- `src/arena.c` — three nested arenas carved from one 8 MB static block:
  global (process) -> session (per save, 4 MB) -> room (wiped every transition, 1 MB).
  Nothing allocates in the game loop. No ECS, no virtual dispatch.
- `src/world.c` — flat `u8 tiles[22][40]` per room. The whole collision/lighting model
  lives in `tileFlags`: SOLID, BLOCKS_L, OBSCURES, DARK, ONEWAY, WATER, CONTIG.
- `src/player.c` — movement. Constants are in px per 1/60s frame at the top of the file.
- `src/render.c` — 320x180 render target, integer upscale, CRT post pass, palette,
  room drawing with rim light on exposed tile tops.
- `src/main.c` — platform loop, arenas, the Phase 1 movement gym room, `--play`
  scripted input, `--shots`, `--trace`.

## Movement numbers (tuned, awaiting the user's verdict)
run 1.45 px/f (~87 px/s) - accel 0.24 ground / 0.16 air - turnaround accel x1.9 -
friction 0.34 / 0.045 - gravity 0.225 with x0.62 apex hangtime inside +/-0.70 vy -
terminal 4.30 - jump -3.20 - jump cutoff -1.05 - coyote 6 frames - input buffer 7 frames.
Measured: ~21 px rise (2.6 tiles), ~44 px air distance from a full run. Clears the
3-tile floor gap; the 4-tile gap needs a running start.
Coyote is consumed by a jump but never retro-cleared elsewhere — chained temporary
platforms depend on that.

## Rendering decisions
- The post pass is a real CRT, not an overlay: Y is snapped to source pixel centres, X
  is left bilinear, and scanline depth is modulated per-pixel by luminosity so bright
  pixels bloom across the gap. Vignette is mild and slightly wider than tall.
- A static hash dither (+/- 0.8/255) is applied last. Without it the vignette bands
  visibly on flat dark fields at 8-bit. Dithered posterisation is also period-correct.
- No screenshake, no squash-and-stretch (L10). The player figure is rigid; only
  second-order lag offsets (`leanX/leanY`) move, which is what makes procedural
  animation misbehave in the way we want.

## Verification loop — run all three before calling anything done
```bash
tools/build.sh all
tools/shots.sh 20,120,196                 # A: native, then READ the PNGs
tools/web/wrap.sh build/game.js build/play.html "Title"
node tools/web/drive.mjs build/play.html shots/web \
  "wait:2500;shot:boot;down:KeyD;wait:600;shot:run;down:KeyZ;wait:150;up:KeyZ;wait:350;shot:jump"
# and natively, deterministic and browser-free:
./build/game --play "R:26,RJ:12,R:26,RJ:12,R:36" --shots 120 --out shots --trace
```
Gotchas that cost real time are in `tools/TOOLCHAIN.md`. The important one: emcc's
SINGLE_FILE output is a UTF-8 string, so the host page MUST declare charset=utf-8 or the
wasm is silently corrupted. Native builds never show this — only step B catches it.

## Open questions for later phases
- The Layer 2 hook mechanism is premise-dependent and unresolved until Phase 0 closes.
- Lighting (rim light / thresholded posterised) is deferred: it is the visual identity
  breakthrough in the research, but its art direction depends on the chosen setting.
