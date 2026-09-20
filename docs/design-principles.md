# Design principles and preservation evidence

This is a presentation branch of **AAnderson1817/AWellClaude**, based on commit
`99de7856bedfa059439da1b89435f54c98bddaf7`. Its inherited game has two rooms: the Vault
Mouth and the Drowned Quarter. The branch preserves that game while developing
its 3D art direction and implementing the authored city responses. It is not a
replacement JavaScript game or a completed AAA release.

The governing project documents are [DESIGN-LAW](../claude/DESIGN-LAW.md),
[PREMISE](../claude/PREMISE.md), and the actual source. The dated project-state file
contains some older descriptions alongside newer additions; the lamp, stones,
buoyancy, fire and reset are present in the current source. No design claim below
relies on an obsolete statement that these mechanics are absent.

## Primary-source research

Billy Basso describes a small movement vocabulary, lateral thinking, a compact world
made dense through secrets, and revisiting familiar places with new understanding.
He also explains the locked screen as an invitation to inspect a complete composition.
His discussion of multiple discovery layers distinguishes a satisfying basic journey
from optional and more obscure discoveries. These are methods, not a requirement to
copy his creatures, objects, level layouts or inventory.
Source: [Basso, The secrets of Animal Well, PlayStation Blog, 2022](https://blog.playstation.com/2022/02/10/the-secrets-of-animal-well-coming-to-ps5/).

Basso describes creatures responding to their surroundings according to their own
behavior, and emphasizes unexpected item uses. He also explains the need to revisit
content with different item sets, entrances and player expectations in mind. This
supports preserving an indifferent ecology and testing routes from both directions.
Source: [Basso, An update on Animal Well and the origin story of its creator, 2023](https://blog.playstation.com/2023/02/09/an-update-on-animal-well-and-the-origin-story-of-its-creator/).

His rendering account separates the simple controls from extensive visual work:
composited layers, surface normals, three-dimensional background effects and fluids.
He emphasizes input latency. Therefore depth is not itself a new game verb; additional
visual complexity must preserve the immediate connection between input and body.
Source: [Basso, How Animal Well taps into PS5 hardware, 2022](https://blog.playstation.com/2022/07/20/how-animal-well-taps-into-ps5-hardware-to-elevate-2d-pixel-art-platforming/).

The official Nintendo introduction highlights exploration, observation, a connected
world and environmental detail. That supports wordless discovery, but does not prove
that any particular new scene succeeds at teaching itself.
Source: [Nintendo, Animal Well tips from the developer, 2024](https://www.nintendo.com/en-ca/whatsnew/explore-the-haunting-world-of-animal-well-and-read-some-tips-from-the-developer/).

## What the branch must protect

| Project requirement | Concrete implementation contract | Evidence or remaining limit |
|---|---|---|
| L1: density before expansion | Keep the two authored rooms; 25 remains the eventual ceiling. | Original room maps are compared directly with the baseline. No invented third environment or replacement world. |
| L2: mechanics precede puzzles | Add no resonance locks, collectible quests or authored combinations merely to decorate the new theme. | Original movement/items remain unchanged; city responses extend the existing Hold toy according to the inherited response tables. New observed surprises require actual play records; none are fabricated here. |
| L3–4: nonviolent toys without redundant verbs | Preserve Hold, one hand, the floating lamp and sinking stones. Leave the second verb undecided. | Item source is unchanged; pickup, set-down, weighted movement and reset appear in the trace comparison. |
| L5: no teaching text | No tutorial overlays, dial labels, quest text or lore captions inside the game. | Developer documents and optional debug surface tags remain outside normal play. Human wordless-teaching acceptance is still open. |
| L6: plausible actions receive a response | Preserve ropes, roots, chain lamps, plants, birds, beast, pots, door glint and campfire; implement the authored mural, windows, face and fish responses. | Detached snapshots preserve original reactions. New response tests exercise light/dark recovery, sinking-stone acknowledgment, floating-lamp gathering and body scatter. Readability and spontaneous discovery still require players. |
| L7: default-open traversal | Keep platforms, shaft, water, bulbs and movement tuning. | 35 route checks pass, including both shaft directions and failed-exit recovery. No new item gates. |
| L8: invisible deeper layers | Add no completion meter, visible checklist or promised secret count. | The inherited slice has no finished ending or accepted Layer 2 hook. This branch does not claim those acceptance tests are complete. |
| L9: indifference and no combat | Preserve the animals' behavior and the player's inability to attack them. | Original life step code is unchanged. Tone requires observation of players. |
| L10: no screenshake or body squash | Use layered scenery, light, material and existing procedural movement for depth. | Disney-inspired layering means staged visual planes, not a new reassuring body-animation vocabulary. |
| L11: one room, one locked frame | Show the original 40 × 22 tile stage in one fixed side view. Depth stays behind or in front of the interactive plane. | No following or scrolling camera; a GPU playthrough must confirm seam and edge readability. |
| L12: a second understanding | Preserve lamp/stone physics and add the lamp's concealment of the mural, windows' withdrawal, and the face's acknowledgment of a sinking stone. | These are functional second readings of Hold, without an announced checklist. Whether a player discovers them or experiences a revelation is an open human test. |
| D5–7: start, persistent fire, frozen geometry | Keep the start at the sealed door, fire persistence until reset, and the approved traversal geometry. | Static source comparisons plus baseline traces and route/escape sweeps. |

The project has already been tuned for quietness: fewer birds, sparse drips, longer
animal pauses, quiet ambience. A larger renderer is not permission to multiply event
rates, add constant spark showers, or add a continuous musical lead. The existing
audio and effects behavior is preserved. Sparse city responses add a quiet murmur
and one low sinking-stone note, reported separately from the inherited sound events.
Their full-mix comfort and clarity have not received human listening acceptance.

## Metallurgy, astronomy and faith within the existing premise

These themes belong to the vault and the city beneath it. They do not supersede that
premise or add mandatory puzzles.

* **Metallurgy:** tool marks, seams, hammered bands, cold rivets, old casting channels
  and corrosion make materials legible. Moisture explains patina in the flooded room.
  Ornament must not resemble a new usable handle unless it can acknowledge interaction.
* **Astronomy:** repeated orbital proportions, an occluded sky aperture, a fixed
  bronze armillary or reflected star-like points imply observation. Apparent distant
  openings remain visibly outside the playable plane. A marked star pattern must not
  imply an unsolvable code or a new gate.
* **Faith:** worn approach steps, a carefully placed vessel, an unoccupied niche and
  repeated maintenance suggest ritual through use. No voiced explanation dictates
  what people believed. A place can support both measurement and devotion.

The existing light rule is semantic: hunters' flame is amber; the city's light is
green-white. A bronze city mechanism may catch warm reflected light, but it must not
become an unexplained amber emitter. A hunter object must not acquire green magical
emission for color balance. Metal, ritual and the heavens should raise questions through
material evidence while keeping that existing distinction readable.

## Simulation / presentation boundary

`player.c` and `items.c` are byte-equivalent to the baseline apart from line-ending
normalization. Original `audio.c` code remains equivalent after removing only the
marked appended city synthesis block, which consumes no inherited randomness. The
two new sounds are intentional extensions, not historical-baseline events. Authored
map and prop rows are identical. `life.c`
and `props.c` gain only bounded snapshot getters; their existing update and drawing
functions keep their behavior. Snapshot enums moved to `aw.h` retain their order.
`fx.c` also gains a bounded copy getter; the preservation check removes exactly that
accessor and verifies every original effects line remains unchanged.

`city.c` owns its fixed-step response state and independent random stream. It reads
the body, items and persistent fire without changing them. Both presentations use
the same city state. Room 1 is now consistently zoned as city in both renderers;
its fixture light is green-white while the hunter's moving lamp remains warm.
The face-coordinate ambiguity and resolution are recorded as D8 in the premise.

Snapshot coordinates are original room pixels, **y down**, excluding `ROOM_Y`:

| Snapshot | Anchor and information |
|---|---|
| `LifeBirdViews` | Perch/flight location, velocity, facing and flap phase. |
| `LifeBeastView` | Body left, ground below its feet, direction, pose, blink, head lift, leg phase and five tail segments. |
| `LifePlantViews` | Base, sway/lean, active pod/mouth/perk, and exact derived positions of all three pods. |
| `PropsViews` | Original kind, tile anchor, extent, state/timer, pot location, pendulum/banner response and room fire state. |
| `FxViews` | Active event particles copied from the existing fixed pool, including splash, landing dust, door grit, sparks and pot shards. Ambient motes are separate. |
| `CityWindowViews`, `CityMuralView` | Window light/delay/crossing state and mural visibility driven by the lamp and fire. |
| `CityFaceView`, `CityFishViews` | Seven-second eye pulse, two-second stone acknowledgment, and eight water-constrained fish positions/scatter/gathering state. |

Getters copy into caller-owned buffers. They do not return writable pointers into
simulation arrays, advance timers, consume random numbers or change gameplay. Empty
or invalid buffers return zero. The flat presentation remains available for direct
comparison and includes city responses. The independently compiled Git source is
the historical baseline. The 3D renderer owns its GPU resources and consumes snapshots.

## Automated evidence

The following checks were run on this branch with the bundled Windows Zig 0.14.1
compiler and raylib 5.5 headers. The baseline headless binary was built independently
from the Git blobs at `99de785`, using the same compiler and I/O stubs.

| Check | Result | What it establishes |
|---|---|---|
| `tools/check-preservation.py --build-baseline`, current city increment | 15 scenarios, **21,160 compared frames**, exact trace equality under both `--flat` and `--depth` flags | Fixed-step movement, items, water, room seams, reset, inherited sound events and life counters retain baseline behavior. City sound counts are recorded separately and agree between modes. |
| `tools/check-preservation.py --render`, current city increment | 15 scenarios, **6,640 compared frames**, exact original trace equality with actual rendering | Original native drawing, current flat drawing and current 3D drawing produce the same reported inherited states/events. Idle cases are shortened to 180 frames for the GPU pass. |
| Static baseline contracts in the same tool | Passed | Movement/items/fx and authored geometry/prop placements are preserved; original audio remains equivalent after excluding the explicitly marked city append. |
| `tools/route.py` using `AWELL_GAME=build/game-probe.exe` | **35 / 35** | 19 upper-room climb checks, 13 water/shaft checks, 3 reset checks. Search finds real executable input sequences, rather than inferring reachability from drawing. |
| `tools/escape.py` with the same executable | **36 / 36 surfaces return home** | A wander bot actually reaches home from each standable run. This is evidence of escape paths, not proof of every imaginable player state. |
| `tools/tests/input_hash.c`, UndefinedBehaviorSanitizer | Passed | Input edges survive frames with no simulation tick, taps/holds remain distinct, catch-up ticks do not duplicate a press, and the hash avoids signed-overflow UB. |
| `tools/tests/presentation_snapshots.c`, UndefinedBehaviorSanitizer | **3,600 frames passed** | Repeated view reads are stable, outputs are detached, bounds/null handling work, and live body/item/tile/fire state is unchanged by reads. |
| `tools/tests/city_responses.c`, UndefinedBehaviorSanitizer | Passed | Four window response contracts; mural recovery and actual fire coupling; real sinking-stone acknowledgment; floating-lamp gathering, disconnected-basin exclusion, body scatter, 6,000 shoal bounds steps and city snapshot ownership. |
| `tools/tests/test_tools.py` | **5 / 5** | Reachability tools report failures/crashes/empty traces correctly and honor an explicit probe executable. |

The machine-readable trace report is generated at `build/preservation-report.json`.
Use `AWELL_GAME` to select a headless executable on either platform; normal defaults
remain `build/game` on Unix and `build/game.exe` on Windows. `tools/check.sh` includes
the snapshot sanitizer checks. To validate actual native drawing, run:

```text
python tools/check-preservation.py --render --baseline build/game-baseline.exe --candidate build/game.exe --report build/render-preservation-report.json
```

That command compares the original native renderer with actual flat and depth drawing.
A full native pass for the city response increment completed on 2026-09-20 and is
recorded in [city response checks](evidence/city-responses/checks/summary.json), including
[rendered preservation](evidence/city-responses/checks/render-preservation-report.json).
The older `evidence/current` directory is retained as the depth-v10 milestone record;
its directory name does not make it current evidence for later edits. A headless pass alone
must never be reported as validation of GPU rendering. Both
renderers must execute the same `LightStep`, because an existing lighting effect also
consumes the props random stream. Skipping it would subtly alter subsequent responses.

An inherited limitation remains: `frameNo` advances per rendered frame, while the
fixed-step loop can execute zero to five simulation ticks in that frame. Some ambient
animation refers to `frameNo`. Therefore the evidence establishes deterministic
fixed-step parity and correct input latching; it does **not** establish full simulation
invariance at arbitrary rendering rates. Changing that would be a separate mechanics
and timing decision, outside this presentation branch.

The city increment also compiles with Emscripten 6.0.8. Earlier depth-v10 CUA browser
observations showed both rooms, F2 switching, movement and empty warning/error logs.
Current and historical browser coverage are distinguished in
[verification.md](verification.md); compilation is not browser execution, and neither
constitutes a full browser playthrough or perceptual audio validation.

## Human acceptance still required

An earlier independent visual comparison of the depth-v5 Vault Mouth screenshot, original
pixel screenshot and governing reference identified material gaps: ambient lighting
flattened the original warm/cool pools; raw-rock silhouettes still exposed a coarse
tile grid; dressed masonry repeated smooth rounded blocks; the central decorative
orrery could appear interactive; and the door's clean bronze material did not yet
match the weathered architecture. These are concrete art-direction findings, not
failures of the deterministic simulation checks. The subsequent
[v9/v10 review](artistic-review.md) records repairs and remaining material/production
gaps at that milestone. It predates the city response increment and is not a new
judgment of the face, mural, windows or fish. Their controlled captures and tests
establish implementation evidence; they do not constitute AAA visual acceptance.

Automated traces cannot establish immersion, artistic cohesion, comfortable sensory
load, wordless understanding or AAA production quality. Keep those claims open until
observed, and do not turn a good screenshot into a substitute for play.

Run a blind comparison with people who have not been given controls or a route. Ask
them to explore, then record their actions and questions without explaining objects.
Use both rooms and a fresh reset. At minimum, inspect these outcomes:

1. They find the body and distinguish landable surfaces from foreground and distant
   architecture. Log missed landings attributable to a misleading silhouette.
2. They discover Hold and an unannounced object response. Distinguish discovery from
   being told to test something. Their first question about a button is evidence
   against wordless teaching, not a reason to silently add text.
3. They descend, swim, use a stone, put it down, and return through the shaft. Confirm
   that the depth treatment does not conceal the surface, throat shelf or bulb crown.
4. They can remain still for a minute without feeling continuously prompted. Observe
   whether independent lights and movements compete for attention; compare with flat.
5. They describe the place, infer relationships among materials/light/ritual, and name
   an unanswered question in their own terms. Avoid feeding them the intended themes.

The inherited five acceptance tests include a full ending and exploration of 25 rooms.
Those cannot be signed off by a two-room presentation slice. They remain future whole-game
acceptance gates, while this branch can rigorously preserve the mechanics that exist.
