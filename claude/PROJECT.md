# Animal Well Redux — project state

A fresh session should be able to resume from this file plus `claude/DESIGN-LAW.md`
and `tools/TOOLCHAIN.md`.

## What this is
A playable vertical slice of an ORIGINAL 2D game built in C/raylib that reproduces the
*method* of Animal Well, not its content. 25 rooms, two verbs, no text, locked camera,
320x180. The law is in `claude/DESIGN-LAW.md` and is not negotiable. The research it
derives from is in `claude/animal-well-research-brief.md`.

## Phase state
- Phase 0 — Concept. DONE. **Brine and Ballast** chosen. The frozen spec and the four
  decisions taken at the gate are in `claude/PREMISE.md`; all twelve candidates and the
  finalists' stress reports are in `claude/phase0/`.
- Phase 1 — Skeleton and feel. Done, and folded into Phase 2: the load dial redefines what
  "movement feel" means here, so movement is judged with the kettles on rather than twice.
- Phase 2 — Verb A, the kettles. DONE. Six surprises logged in `SURPRISES.md`.
  GATE: the user plays it and judges whether the dial is worth turning.
- Phase 3 — Verb B, the gaff. NEXT. Needs 5+ surprises from the *pair*, found by playing
  a sandbox, before any room is designed.
- Phases 4-5 — not started.

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

## Verb A — the kettles (built)
`u8 load` 0..4 indexes `JUMP_V / GRAV_L / TERM_L / JCUT_L / SINK_L`. Measured jump ladder:
4.49 / 3.45 / 2.68 / 1.92 / 1.34 tiles. Horizontal handling is identical at every load and
must stay that way (PREMISE.md D4).

Buoyancy blends `GRAV_L` and `SINK_L` by **submerged fraction**, never a binary in-water
test — the binary version oscillated across the surface and pinned a floating body against
a flush rim with no way out. `SINK_L` is signed: loads 0-1 float, 2+ sink. `SWIM_THRUST` is
a constant, so which loads can rise is decided entirely by `SINK_L`.

Crust fatigue lives in `scratch.stress` in the **room arena**, so it recrystallises on
transition with no save state. It accumulates only while a body of load >= 2 rests on the
tile (`loadMap >= CRUST_BREAK_LOAD`), decays otherwise, and takes an extra dose on landing
scaled by impact x weight.

## Open questions for later phases
- Lighting (rim light / thresholded posterised) is deferred: it is the visual identity
  breakthrough in the research, but it is a big change and Phase 5 is where it belongs.
- Filter-feeders and drifting rafts do not exist yet. PREMISE.md D2 cut the gaff's two-body
  mass ratio from Phase 2/3 scope; it returns only if a moving anchor earns it in Phase 4.
- The bell room's constraint from D1: **no convex submerged corner below the brine line**,
  or the gaff's ratchet dissolves the Layer 2 gate.

## Debug flags
`--play "R:60,RJ:8,A:40"` scripted input (L R U D J A B, `-` idle) · `--shots 20,120`
· `--out DIR` · `--trace` per-frame state · `--at TX,TY` place the player · `--load N`
