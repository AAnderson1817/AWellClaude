# Verification

This branch extends the C/raylib source inherited from
`AAnderson1817/AWellClaude`, source branch `claude/animal-well-redux-slice-oimk6l`,
commit `99de7856bedfa059439da1b89435f54c98bddaf7`. It preserves the existing two rooms
and tests the new presentation against that specific source. It does not compare
against an unrelated reconstructed game or call the current flat renderer the
historical baseline.

## Reproduce on Windows

```powershell
./tools/setup-win.ps1
./tools/build-win.ps1
./tools/check-win.ps1
```

The checker uses the portable Zig 0.14.1/raylib 5.5 dependencies provisioned by setup.
It discovers Python 3 from PATH or the bundled Codex runtime. An explicit executable
can be supplied:

```powershell
./tools/check-win.ps1 -Python 'C:/path/to/python.exe'
```

The game itself does not require Python. The checker stops on a failed build, crashed
test, changed trace, unreachable route or unsuccessful escape sweep. `AWELL_GAME` is
temporarily pointed at the headless binary and restored afterward. The original Bash
`tools/check.sh` keeps its input/hash and Python regressions and now also runs the
snapshot test with UndefinedBehaviorSanitizer.

To store a deliberate review record in the repository:

```powershell
./tools/check-win.ps1 -EvidenceDirectory docs/evidence/headless
```

Ordinary checks default to ignored `build/verification`. Reports contain per-case
frame counts and SHA-256 hashes of the original traces, plus readable tool logs.
These are evidence from a particular working tree; rerun them after relevant changes.

## What runs

| Check | Coverage | Latest headless result |
|---|---|---|
| `input_hash.c`, UBSan with recovery disabled | Real frame loop input edges, sub-tick frames, released taps, catch-up ticks and unsigned hashing | Pass |
| `presentation_snapshots.c`, same sanitizer | Birds, beast, plants, props and event particles: bounded copies, null inputs, repeated reads, copy isolation, live-state preservation | 3,600 frames pass |
| `test_tools.py` | Reachability failures, crashes, empty traces and explicit executable selection | 5 tests pass |
| `check-preservation.py` | Independently compiled original source vs both presentation flags; all printed body/item/water/reset/audio/life states | 15 scenarios, 21,160 compared frames, exact match |
| Static source contract | Movement, items and audio unchanged; original life/props/effects preserved after removing only the named accessors/moved enums; authored tile and prop rows unchanged | Pass |
| `route.py` | Search-based climbing, water/islands, shaft in both directions, failed exit, full reset and aborted reset | 35 checks pass |
| `escape.py` | A wander bot returns to the original start from each standable run in both rooms | 36 of 36 surfaces pass |

The current tracked native run is recorded in
[summary.json](evidence/current/summary.json),
[preservation-report.json](evidence/current/preservation-report.json),
[route.txt](evidence/current/route.txt) and [escape.txt](evidence/current/escape.txt).
The counts are actual completed checks, not acceptance targets.

The historical source is extracted from the pinned commit into ignored
`.local/baseline-99de785` and compiled with the same compiler as the candidate. This
requires that commit to exist in the local Git repository. The source check deliberately
fails if signed-off mechanics or geometry change; an intentional future mechanics
change should revise this contract explicitly, not hide the difference in a screenshot.

## Actual rendering comparison

Headless checks do not execute OpenGL drawing. For native rendering parity:

```powershell
./tools/check-win.ps1 -Render
```

This runs the full headless suite, builds the current native renderer, separately
builds the original native renderer from Git blobs, then compares original/flat/depth
traces with drawing enabled. Quiet cases are shortened to 180 frames for the GPU pass.
The renderer is permitted to change pixels; the compared simulation states and events
must remain identical. The `LightStep` call must remain common to both modes: an
inherited lighting response consumes the props random stream.

The latest complete native pass, on the depth-v10 build, matched **6,640 frames across
15 cases** and also reran the full headless suite. Its record is
[render-preservation-report.json](evidence/current/render-preservation-report.json).
An earlier depth-v5 report is retained separately at
[render-depth-v5.json](evidence/render-depth-v5.json). Reports describe their tested
builds; they are not artistic acceptance and do not automatically cover later edits.

For a targeted diagnostic without rebuilding:

```powershell
python tools/check-preservation.py --render --baseline build/game-baseline.exe --candidate build/game.exe --case shaft-down --case stone-dive-and-reset --report build/render-targeted.json
```

For original-source compilation plus a direct rendering comparison, add
`--build-baseline` to that command. The baseline executable is built from the pinned
commit, not the current working tree.

## Web build and wrapper checks

The existing Emscripten target was compiled on Windows with the installed SDK 6.0.8
and raylib 5.5 sources using `tools/build-web-win.ps1`. The script builds the seven
standard raylib web modules and preserves `tools/build.sh`'s `ASYNCIFY`, `SINGLE_FILE`,
memory-growth, modularized `RL`, browser-only environment and exported heap-view flags.
It writes a separate `lib/libraylib_web.a`, preserving the native libraries.

The first build produced `build/game.js` at **5,830,543 bytes** and a self-contained
`build/play.html` at **5,831,677 bytes**. There were no game compiler/linker errors;
six warnings came from upstream miniaudio/stb code. These sizes are a dated build
record, not a size guarantee for subsequent renderer/asset changes.

The final color-only polish was rebuilt with the same pipeline: `game.js` is
**5,830,557 bytes** and `play.html` is **5,831,706 bytes**. Current artifact SHA-256
hashes and build time are retained in [web-build/report.json](evidence/web-build/report.json).

`tools/web/wrap.py` works on Windows and Unix; the existing Bash wrapper delegates to
it using a checkout-relative path. It fixes the old hardcoded `/home/user` directory
and invalid `100%%` CSS. Five automated checks verify raw module byte/newline and
UTF-8 handling, embedded script-end escaping, safe title handling, observable
module initialization failures, and safely serialized developer arguments:

```text
python tools/tests/test_web_wrapper.py
```

All **5 wrapper tests passed**. The web build and wrapper tests do not execute a WebGL
context, so the browser was checked separately using CUA's in-app browser.

The live browser check confirmed the Vault Mouth boots into the 3D presentation,
F2 visibly switches to the original pixel renderer and back, and eight Right presses
move the player. The Drowned Quarter review page also boots with its waterline,
reflection and submerged geometry visible. Browser error/warning logs were empty in
both rooms; no shader errors were observed. These observations are recorded in
[web-smoke.json](evidence/web-smoke.json). They refer to live CUA screenshots and log
inspection in the task history, not saved browser screenshot files.

This is a browser smoke check, not full web/native state-trace equivalence or a complete
browser traversal. Perceptual audio/listening, long play sessions and broader browser/
device coverage remain open. The native deterministic suite does not establish them.

An optional developer-only `--arguments` JSON array creates review pages with the
game's existing command-line switches; normal play defaults to an empty array. For
example, this opens the flooded room without adding any in-game menu or new mechanics:

```powershell
python tools/web/wrap.py build/game.js build/review-water.html 'Drowned Quarter review' --arguments '["--room","1","--at","22,3","--mute"]'
```

## Visual and interactive review

Screenshots under `assets/review` document the original game and successive 3D
iterations. Compare both environments at the default 1280 × 720 output and at common
window sizes. Inspect the actual body, landable surfaces, waterline, bulb crowns,
shaft throat and return transition. Confirm the player can distinguish them from
background mechanisms, arches and foreground framing.

Test the same visible responses in both modes: the door's moving glint, rope/chain
sway, a compressed bedroll, skull tip, knocked/falling/shattered pots, persistent fire,
bird takeoff, beast sitting/watching, plant speech and water splash. A matching trace
can still accompany a missing visual response. That is why snapshot tests and rendered
trace checks are supplemented by visual review.

Current response captures are organized under `assets/review/responses` (bulb, bush,
dead lamp, fire and reset). They accompany the native trace checks and should be
reviewed alongside the original behavior, rather than treated as proof from filenames.

F2 switches flat/depth during ordinary interactive play; F4 toggles optional ambient
drift. These are review/presentation controls, not new game verbs. Keyboard instructions
remain in the README, outside the normal wordless interface.

## Limits of the evidence

* **Artistic quality remains open.** Automated checks do not certify AAA production
  quality, material cohesion, visual splendor or fidelity to the governing reference.
  The depth-v5 review identified coarse silhouettes, flat light, repeated masonry,
  material mismatch and a potentially misleading decorative mechanism. Later changes
  need fresh review.
* **Immersion and sensory comfort remain open.** The suite runs with audio muted and
  checks sound events; it does not substitute for listening or blind human playtests.
* **The complete design law is not certified.** The existing slice has two rooms, no
  finished ending and no accepted deeper-layer completion structure. Tests requiring
  a complete 25-room game cannot pass here.
* **Escape coverage is constructive, not exhaustive.** Finding a route proves that
  route exists. A finite wander sweep does not prove every inventory arrangement;
  the existing full-reset escape remains available and separately tested.
* **Timing preserves the inherited implementation.** `frameNo` advances per rendered
  frame, while zero to five physics ticks may execute. Some ambience reads that frame
  count. The tests establish fixed-step parity and input-latch correctness, not
  invariance of every effect at arbitrary rendering rates.
* **Native Windows and browser checks have different coverage.** The original Linux
  and Emscripten targets remain in `tools/build.sh`. Web compilation and live WebGL
  rendering in both rooms, basic movement and renderer switching passed. Complete
  browser traversal, audio listening and browser/device coverage are still open.
  The Linux target has not been rebuilt here.

The source-backed design interpretation and proposed blind playtests are described in
[design-principles.md](design-principles.md).
