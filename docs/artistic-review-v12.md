# Whole-brief review — depth v12

**Verdict: rework. Overall goal and artistic release state: work_in_progress.**
The architecture and foliage pass improves the existing game, preserves a readable
2D play plane, and gives its near scenery more physical construction. It does not
meet the requested AAA finish. Missing inhabitants and a completed discovery arc
are known scope gaps. Immersion, sound comfort and cold-player understanding remain
unverified; they are not inferred from pictures or passing tests.

Review date: 2026-09-20. Scope is the complete user brief: work in the existing
Animal Well repository on a branch; create a AAA 3D rendition retaining the 2D
view and animation-style layered depth; create at least 30 cohesive reference
images per environment; rigorously confirm design principles; create immersion;
interweave metallurgy, astronomy and faith to provoke investigation. The narrower
architecture increment does not remove earlier in-scope weaknesses.

**Review independence:** this reviewer did not author the terrain, supports,
foliage, face, mural geometry or renderer. The reviewer did author the city response
simulation and its tests in an earlier increment, and the independent delivered-array
alignment checker in this increment. The visual assessment is independent of the
asset and renderer makers; claims about city behavior also rely on separately
captured native evidence and the earlier whole-brief review. This is an agent
review, not a human acceptance session.

## Evidence and benchmark

- Actual native 1920×1080 [Vault](evidence/architecture/native/with-occlusion/room-0.png)
  and [Drowned](evidence/architecture/native/with-occlusion/room-1.png) captures,
  with corresponding same-source images without contact occlusion. The
  [capture manifest](evidence/architecture/native/manifest.json) identifies the
  renderer and terrain, support and foliage headers represented by those images.
  All four current source hashes matched that manifest at review. This is the
  receiver-fog candidate captured at `2026-09-20T20:40:49Z`, after the broad-wall
  and overly dark narrow-spine revisions.
- Both governing room references, both complete 30-image contact sheets, and
  detailed column, raw-rock seam, drowned doorway and cornice references. The
  library remains the 60 unique images verified in the v11 review; this pass did
  not substitute an easier visual target.
- Final bush and frond three-quarter source renders and their clean GLB-reimport
  renders in `assets/blender/review/foliage/`. The visible forms match across export:
  leaf attachment, branch layout, taper and tall/low frond hierarchy survive.
  Technical verification records zero degenerates, invalid normals or nonfinite
  geometry. This verifies delivery, not whole-game artistic quality.
- The current `claude/DESIGN-LAW.md`, `PREMISE.md`, `ROOMS.md`, `SURPRISES.md`,
  response implementation report and [v11 whole-brief review](artistic-review-v11.md).
  Earlier native response captures remain evidence for unchanged response logic,
  not a fresh test of discovery or listening.
- [Delivered architecture checks](evidence/architecture/final-v12.json) against
  the actual map and calibrated perspective camera. Technical and artistic
  acceptance are kept separate in the [contract](architecture-contract.md).

The references demonstrate coherent carved and built forms, material-specific
edges, worn interfaces, depth-bearing mass and selective detail. Their invented
platform positions and explorer costume are not required replacements for the
existing collision map or player identity. Exact reference reconstruction is not
the acceptance criterion; craftsmanship at the actual play size is.

## Requirement disposition

| Requirement | Result | Evidence and limit |
| --- | --- | --- |
| Extend the existing repo on a branch | Satisfied for this increment | C/raylib modules, embedded arrays and the existing gameplay remain the implementation. This review does not assert publication of a commit. |
| 3D rendition retaining the 2D view | Present | Modeled near scenery, actors and props use the locked calibrated camera. The scene retains the original landing layout and separate distant city layers. |
| At least 30 references per environment | Satisfied for the two existing environments | Two complete cohesive collections cover governing views and their sub-elements. Reference quantity does not establish runtime fidelity. |
| AAA visual and sensory splendor without overload | **Fail visually; sensory comfort unverified** | Construction has improved, but the foreground finish, hero carving and mural integration still fall below the governing benchmark. Still images cannot establish comfortable sound or prolonged motion. |
| Every element thematically cohesive | Partial; full requirement not met | Warm hunter metal/flame and cool city stone/light remain distinct. Astronomical forms and ritual imagery recur, but uneven material craftsmanship and unfinished inhabitants prevent complete cohesion. |
| Vigorously confirmed Animal Well design principles | Partial; full requirement not met | Collision/preservation and response checks are concrete technical evidence. The current surprise log does not establish the full present discovery sequence, and ending/deeper-layer requirements remain unfinished. Cold-player tests remain open. |
| True immersion | **Unverified** | Atmosphere and observable responses support the intent, but no recorded human session demonstrates sustained immersion. |
| Metallurgy, astronomy and faith provoke questions | Partial; player interpretation unverified | Metal joints, repairs, orbital instruments, the procession and responsive carved face suggest shared culture. Whether players form and pursue the intended questions has not been tested. |

## What improved, and what still blocks release

### Constructed support and background separation

The earlier v12 broad receiving walls were rejected: they passed collision checks
while covering large rectangular areas of the distant city. The revision uses
narrow receiving spines, leaving the arcades and fog visible between supports.
Delivered geometry now covers 3,732 open-map pixel samples in the Vault instead
of 13,330 in that rejected candidate; Drowned falls from 3,912 to 872. These are
diagnostics of projected geometry, not an artistic score or an alpha-visible
screen percentage.

Timber now has a front thickness, fasteners and seated brackets. Cornices have
layered profiles and discrete corbels. The large drowned fronts contain actual
recessed doorways and window slots, resolving the earlier absence of architectural
use. The center column has a broader capital and articulated base. Their landing
edges remain continuous and unobscured in the inspected views.

The final receiver-only fog reduces the narrow spines' dark foreground contrast;
they now sit behind the landing faces and preserve the open view to the city.
Their repeated vertical rhythm remains visible, but the rejected broad-wall
depth collapse is resolved in these captures. This approves that specific repair,
not the whole architecture's finish.

The matched occlusion comparison shows local darkening at the column's capital
and base, masonry recesses, foliage attachments and shelf joints. The frame keeps
the pale player, green fish and face eyes legible. I see no new conspicuous grain,
block-shaped halo or screen-wide stripe introduced by the pass in either still.
Diagonal surface marks on the face are present in both versions. These are useful
contact cues; they do not supply missing geometric craft or prove temporal
stability. The same local 1080p benchmark records mean frame-loop times of
2.8439/2.2398 ms with occlusion and 1.5939/1.3599 ms without it for Vault/Drowned,
over 600 measured frames after 120 warmup frames. These are wall times around
native frames on this host, not isolated GPU measurements or general performance
acceptance.

**Remaining blocker: foreground construction and material finish.** In the Vault,
the stepped ceiling and left rock mass still expose a regular tile-derived outer
shape with softly mottled surfaces; the lower-right wall remains a large field of
repeated smooth rectangular blocks. In Drowned, the fronts around the openings
still read as clean ashlar grids with uniform dark joints. The new openings help
identity, but most of the mass lacks the varied dressed returns, local wear and
material-specific edge behavior visible in the benchmark. At play size the rich
distant city and simplified nearby structure still look like different levels of
finish. Refine those broad forms and interfaces while retaining exact contact
edges; adding unrelated ornament or darker seams alone will not close the gap.

### Foliage is materially better, but not a blanket finish approval

The bush has attached leaves with visible laminas and branching stems. Root ends
are tapered into the substrate instead of ending as floating blunt tips. The wet
fronds have a tall/low hierarchy and curved, tapered surfaces. In the native rooms
these replace the earlier sparse bristle appearance with recognizable plants.
The source/reimport pairs show that this is real delivered geometry.

**Remaining minor finding at current play size:** repeated clusters remain easy
to recognize along the floor and both terraces, and many underwater leaves read
mostly as dark silhouettes. This does not currently defeat collision readability
or the room's identity; it is not the primary AAA blocker. Controlled differences
in growth direction and local material response would help more than increasing
the number of constantly moving leaves. The macro foliage asset's visible branch
crossings also should not be presented as cinematic hero-asset acceptance.

### Hero objects and inhabited meaning are unfinished

**Remaining visual blockers.** The face's large, comparatively smooth rectangular
stone field, regular eyelid lines and simplified mouth transitions still read
more like a soft modeled mask than the reference's carved mineral mass. Its
light response is meaningful, but does not repair that construction gap. The
Vault mural backing still resembles a separate irregular slab placed in front
of the room wall; its outline remains obvious even when its pigment disappears.
Improve the carving's planes and transitions, and seat the mural into a coherent
wall surface, without reducing response visibility.

The spiral door, lantern and astronomical instrument carry the setting more
effectively, but the restrained props and creatures still vary in finish. No
approval of the new supports excuses these existing in-scope differences.

**Remaining content blockers.** The hunter and aquatic native specified in the
response tables still lack their authored behavior. Face-mouth bubbles, several
drowned props/responses and the final ambience/voice pass are unfinished. The
drowned facade openings are now delivered dressing and must no longer be listed
as missing. There is still no completed ending and accepted deeper discovery
sequence. These are known misses, not merely unverified quality judgments.

## Design principles and perceptual limits

The scene remains wordless and camera-locked, with no new player verb, combat or
gate introduced by this architecture pass. The current Hold relationships offer
useful second readings: carrying light conceals the mural and darkens windows;
a sinking stone receives an answer; a floating lamp gathers reachable fish.
These effects are actual implemented behavior, not an implied interaction from
decorative art. The city tones deliberately extend the original audio palette;
original-event preservation is not a claim that the audio is identical.

The original route, escape and state/event preservation suites are the appropriate
behavioral safeguards. The architecture audit adds 4,976 map-derived landing
probes, 112,640 terrain mask samples, front-envelope/recess checks and adversarial
fixtures on the actual delivered arrays. They cannot establish inherent
playfulness, successful wordless teaching, dread, an unexpected route through a
finished world, or the final reversal required by the design law.

`SURPRISES.md` largely records earlier saltworks/gaff experiments and unassigned
rooms. Those records should be preserved as history, but do not demonstrate that
the current room composition arose from observed surprises with its current toys.
Likewise, controlled state fixtures are not cold discovery tests. Human sessions
with sound need to record what people try, misunderstand, notice and ask, including
whether the dark drowned doorways imply access that the world never acknowledges.
That possible affordance conflict is unverified here, not silently accepted.

This review inspected still images and source/export comparisons, not prolonged
live play or a listening session. Motion comfort, audio balance and sustained
immersion remain unverified. A short local native benchmark can document that
machine's frame-loop cost; it cannot establish hardware-wide or browser performance.

Retain the improved supports, openings and foliage, resolve the remaining
foreground/hero craftsmanship and authored behavior gaps, then gather human
discovery and sensory evidence. A fresh whole-brief review is required for release.
