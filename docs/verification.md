# Verification

The current inhabitant revision 15 integrates the hunter into the delivered game.
The [final headless suite](evidence/hunter/final-headless/summary.json) passes input,
snapshot and response checks with UndefinedBehaviorSanitizer, **21,160 original-state
comparisons across 15 scenarios**, **35 route checks** and **36 escape checks**.
The separate [native-rendered preservation report](evidence/hunter/render-preservation.json)
passes **6,640 original-state comparisons**, also in both presentation modes.

The [full game-loop hunter report](evidence/hunter/final-headless/hunter-full-game.json)
uses the executable's actual input loop for taking, returning, carrying away,
resetting, tending and fire conversation. It compares every item, ownership and
hunter state across **8,506 frames** in flat/depth runs. The intended historical
change is retained explicitly: Hold now picks up a real stone where the original
cairn was decorative. The original player implementation and inherited item
operations remain checked; this is not a claim that added content has no effect.

The [module checks](evidence/hunter/final-headless/hunter-module.txt) audit 28,748
conservation steps, 14,116 voice poses, 3,276 actual arm poses and 6,000 exact
inherited-state idle hashes. Fixed arm segments are 4.2 logical pixels each; the
largest audited target reach is 8.297761 of 8.4 pixels. The
[audio checks](evidence/hunter/final-headless/hunter-audio.txt) examine 26,293
pitch-scaled phrase samples, PCM bounds, lifecycle and original audio isolation.
They do not establish listening comfort or actual device latency.

[Final native captures](evidence/hunter/native-v15/manifest.json) pin every source
C/header hash, executable, input report, capture tool and image. They show actual
pickup/return, tending, fire and flat/depth poses, with an exported voice montage.
The [corrected rendering review](hunter-rendering-review.md) closes the sampled
arm-stretch finding while retaining cairn shape, occlusion and transition concerns.
Initial captures and the [initial independent review](hunter-independent-review-v15.md)
remain dated evidence of the rejected version, not acceptance of the final build.

The [WebGL smoke record](evidence/hunter/web-smoke.json) pins source/build hashes.
Both rooms rendered at 1280 by 720, F2 switched flat/depth and back, and no warning
or error entries were captured. X interaction was exercised at the camp; overlapping
bodies limit visual item counting, which the native game-loop traces cover separately.
The self-contained HTML is 21,249,065 bytes. All 13 gallery runtime/model images loaded.
This is not a full browser playthrough or perceptual sound review.

The [whole-brief review](artistic-review-v15.md) remains **rework**. Foreground rock,
face carving, cairn and character motion, remaining inhabitants/discovery and human
acceptance still need work. Technical correctness does not establish AAA craft or
immersion.

## Earlier environment evidence retained

The environment asset arrays are unchanged from craft revision 14. Its
[actual-array architecture audit](evidence/craft-v14/final-architecture.json), including
19 adversarial mutations, and [embedded-material checks](evidence/craft-v14/runtime-material-package.json)
retain that scope. Editable Blender terrain, face and scanned-material sources have
clean reopen/reimport records. [Material provenance and delivery](materials.md)
distinguish the credited CC0 scan from authored geometry and other surfaces.

The v14 [matched native captures](evidence/craft-v14/native/manifest.json) compare
local illumination with a control. At 1920 by 1080, after 120 warmup frames, 600
measured frame-loop calls averaged 3.1173 ms (Vault) / 2.2606 ms (Drowned) with local
lights, versus 3.1334 / 2.2724 ms without. The small difference is run variation,
not a performance gain. A 4 ms rest between frames is outside the timer. These
are local wall times, not GPU-only timings, v15 performance measurements or a
hardware guarantee. See [lighting](local-lighting.md). The v14
[browser smoke](evidence/craft-v14/web-smoke.json) and
[artistic review](artistic-review-v14.md) remain historical records.

Revision 13's [material package](evidence/materials/runtime-package.json),
[native texture comparison](evidence/materials/native/manifest.json) and
[browser smoke record](evidence/materials/web-smoke.json) also remain historical.

The previous architecture revision has a separate [final geometry audit](evidence/architecture/final-v12.json)
and [independent whole-brief review](artistic-review-v12.md). The geometry audit
checks the actual delivered C arrays, 2D landing contacts, normals, indices,
recessed support placement, and renderer integration; seven deliberately broken
fixtures establish that the checker rejects relevant defects. Artistic verdict:
**rework**. Geometry validity does not establish AAA craft or immersion.

The [matched native captures](evidence/architecture/native/manifest.json) compare
the same source/assets with and without geometry contact shading. Each room has
120 warmup frames followed by 600 measured frame-loop calls at 1920×1080. On this
host, mean times were 2.84/2.24 ms with occlusion and 1.59/1.36 ms without. These
are local wall-time throughput observations with vsync disabled, not GPU-only
timings or a cross-hardware guarantee. The current headless checks are in
[architecture/checks](evidence/architecture/checks/summary.json); 6,640 actually
rendered comparison frames also pass in
[render-preservation-report.json](evidence/architecture/checks/render-preservation-report.json).

The web verification for this revision is recorded separately in
[architecture/web-smoke.json](evidence/architecture/web-smoke.json). Earlier records
below retain their original revision scope.

The final depth-v11 browser check is recorded in
[city-responses/web-smoke.json](evidence/city-responses/web-smoke.json), including
current source and build hashes. Both rooms rendered in the in-app Chromium browser
with no captured warning/error entries; F2 switched modes and keyboard movement
worked. The corrected upper window panes remain inside their apertures. The final
self-contained HTML is 8,603,362 bytes. These are bounded live observations, not a
full browser playthrough or perceptual sound acceptance.

The [v11 whole-brief review](artistic-review-v11.md) inspects the current 720p/1080p
room captures and nine controlled response images. Its artistic verdict is rework.

This branch extends the C/raylib source inherited from
`AAnderson1817/AWellClaude`, source branch `claude/animal-well-redux-slice-oimk6l`,
commit `99de7856bedfa059439da1b89435f54c98bddaf7`. It preserves the existing two rooms
and tests the new presentation and city response increment against that specific source. It does not compare
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
snapshot, city response, hunter state and hunter audio tests with
UndefinedBehaviorSanitizer. The Bash/Linux target has not been run on this host.

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
| `city_responses.c`, same sanitizer | Four windows and timed return, mural/fire response, real sinking-stone acknowledgment, fish gathering/scatter/connected water bounds and city snapshot ownership | Pass, including 6,000 additional shoal bounds steps |
| `hunter_responses.c`, same sanitizer | Actual items, interruption, tending/placement, six-stone exhaustion, room/reset lifecycle, supported walking, voice timing and fixed arm reach | 28,748 conservation steps; 14,116 voice and 3,276 arm poses pass |
| `hunter_audio.c`, same sanitizer | Actual pitched PCM timeline, envelope, device/mute lifecycle and inherited audio isolation | 26,293 timeline samples pass |
| `test_hunter_integration.py` | Six scenarios through the actual CLI game loop; every item, owner and hunter field compared across both modes | 8,506 compared frames pass; authored historical pickup difference retained |
| `test_tools.py` | Reachability failures, crashes, empty traces and explicit executable selection | 5 tests pass |
| `check-preservation.py` | Independently compiled original source vs both presentation flags; all inherited body/item/water/reset/sound/life trace fields compared exactly; city and hunter sound counts reported separately | 15 scenarios, 21,160 compared frames, exact match; added sound counts agree between modes |
| Static source contract | Player source unchanged; items preserve original lines around the explicit ownership and draw hooks; original audio preserved before marked city/hunter append blocks; original life/props/effects retained around named hooks; authored tile/prop rows unchanged | Pass |
| `route.py` | Search-based climbing, water/islands, shaft in both directions, failed exit, full reset and aborted reset | 35 checks pass |
| `escape.py` | A wander bot returns to the original start from each standable run in both rooms | 36 of 36 surfaces pass |

The current hunter integration run completed on 2026-09-20 and is recorded in
[summary.json](evidence/hunter/final-headless/summary.json),
[preservation-report.json](evidence/hunter/final-headless/preservation-report.json),
[city response tests](evidence/hunter/final-headless/city_responses.txt),
[route.txt](evidence/hunter/final-headless/route.txt) and
[escape.txt](evidence/hunter/final-headless/escape.txt).
The counts are actual completed checks, not acceptance targets.

The older `evidence/current` directory is the historical depth-v10 milestone record.
The city increment adds behavior absent at that milestone. The inherited
`dbgLastSfx` trace remains an original-gameplay-event field; a separate `CITY SFX`
line reports murmurs and hums; `HUNTER AUDIO` reports hunter phrases separately.
No original trace field is filtered to obtain parity.
The new audio synthesis is appended without consuming the inherited random stream.
The complete soundtrack is therefore intentionally extended, not byte-identical.

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

The latest complete native pass, on the hunter integration increment, matched **6,640
frames across 15 cases** after the final headless suite. Its record is
[render-preservation.json](evidence/hunter/render-preservation.json).
The earlier [depth-v10 report](evidence/current/render-preservation-report.json) and
depth-v5 report at
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

The historical city response build produced `build/game.js` at **8,602,213 bytes** and
self-contained `build/play.html` at **8,603,362 bytes** with Emscripten 6.0.8 and
raylib 5.5. These are inspected artifact sizes for this increment, not performance
measurements or guarantees for later revisions.

For build history, the first depth-v10 build produced `build/game.js` at **5,830,543 bytes** and a self-contained
`build/play.html` at **5,831,677 bytes**. There were no game compiler/linker errors;
six warnings came from upstream miniaudio/stb code. These sizes are a dated build
record, not a size guarantee for subsequent renderer/asset changes.

The depth-v10 color-only polish then produced `game.js` at **5,830,557 bytes** and
`play.html` at **5,831,706 bytes**. Its historical artifact hashes and build time are
retained in [web-build/report.json](evidence/web-build/report.json); that report does
not describe the larger city response build.

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

The historical depth-v10 live browser check confirmed the Vault Mouth boots into the 3D presentation,
F2 visibly switches to the original pixel renderer and back, and eight Right presses
move the player. The Drowned Quarter review page also boots with its waterline,
reflection and submerged geometry visible. Browser error/warning logs were empty in
both rooms; no shader errors were observed. These observations are recorded in
[web-smoke.json](evidence/web-smoke.json). They refer to live CUA screenshots and log
inspection in the task history, not saved browser screenshot files. They do not
automatically certify the newer city response renderer.

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

Earlier depth-v10 response captures are organized under `assets/review/responses` (bulb, bush,
dead lamp, fire and reset). They accompany the native trace checks and should be
reviewed alongside the original behavior, rather than treated as proof from filenames.

Current controlled city fixtures are under
[city-responses/native](evidence/city-responses/native). The actual frame loop and
3D renderer produced before/answer/after face captures, visible/concealed mural
and lit/doused window pairs, and capital/lantern shoal comparisons. State logs
record the face answer at 1.0 on frame 48 and back at zero by frame 180; mural
visibility is 1.0 without the lamp and zero with it. On frame 900 the shoal's mean
y is 84.448 without the nearby floating lamp and 67.041 beneath it. Initial
placement is a controlled fixture, including an extra sinking stone confined to
the review executable. These are not records of spontaneous player discovery.

Reproduce the native response fixture with:

```powershell
./tools/capture-city-win.ps1
```

See [implementation and limits](city-response-implementation.md) for the response
contracts and corresponding tests. The integrated hunter has separate
[implementation and evidence](hunter-implementation.md). The native, mouth bubbles
and remaining response-table content remain unfinished.

F2 switches flat/depth during ordinary interactive play; F4 toggles optional ambient
drift. These are review/presentation controls, not new game verbs. Keyboard instructions
remain in the README, outside the normal wordless interface.

## Limits of the evidence

* **Artistic quality remains open.** Automated checks do not certify AAA production
  quality, material cohesion, visual splendor or fidelity to the governing reference.
  The depth-v5 and v9/v10 reviews record dated material, layering and affordance
  findings. They predate the new city responses. New response captures do not by
  themselves supersede those quality judgments or establish the whole AAA brief.
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
  rendering in both rooms, basic movement and renderer switching passed at the
  historical depth-v10 milestone and again for depth-v11 as linked above. Complete
  browser traversal, audio listening and browser/device coverage are still open.
  The Linux target has not been rebuilt here.

The source-backed design interpretation and proposed blind playtests are described in
[design-principles.md](design-principles.md).
