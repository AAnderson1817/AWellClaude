# Production status

This branch extends the existing two-room C/raylib game. It is a working 3D
presentation and city response candidate, with original mechanics retained and
native and web builds. The overall goal remains active. This status describes the
inhabitant revision 15 after the architecture, materials and city response milestones.

## Delivered

- A locked perspective camera calibrated to the original gameplay plane, 3D terrain
  following authored collision silhouettes, layered city mattes and foreground framing.
- Embedded modeled door, astronomical instrument, terrain, pot, lantern and creature
  parts, with editable Blender sources and clean GLB reimport verification.
- Blender-authored timber/stone shelves, narrow recessed anchor piers, deep sealed
  façade openings, turned columns and rooted volumetric foliage. Geometry-derived
  contact shading replaces the old depth-based darkening. Source/GLB packages and
  actual C arrays are checked separately against the original landing edges.
- Unequal masonry courses, broad rock fracture planes, a rebuilt volumetric carved
  face, seated column profiles and local illumination from existing light sources.
- Credited scanned city stone plus authored basalt and timber maps, embedded with
  mip filtering and separate endgrain treatment. Editable Blender sources and
  [material provenance and evidence](materials.md) accompany the delivery.
- Existing responsive props, wildlife, bulbs, lamp/stone behavior, water and reset
  presented in 3D; flat presentation available with F2.
- Authored mural concealment/recovery, four reactive windows, seven-second face eye
  pulse with a two-second sinking-stone answer, and eight fish that scatter from the
  body and gather beneath a reachable floating lamp. Both modes consume the same
  response state, with quiet city murmurs and a low note extending original audio.
- An integrated camp hunter tending actual cairn stones, accepting nearby offerings,
  yielding to player pickup, and responding to fire and visits with wordless phrases.
  Blender attire, rigid body poses and fixed-length arms share the real state in
  flat and depth modes. No new player verbs or collision gates are added.
- Current sanitizer checks, 21,160 headless and 6,640 native-rendered baseline
  comparisons, 35 route checks and 36 escape checks pass. Six full game-loop hunter
  scenarios compare all item and owner traces across 8,506 frames in both modes.
  Final native captures and a bounded two-room WebGL smoke check record the build.
  Geometry/material audits and controlled light timings retain their v14 scope;
  those environment assets are unchanged in this increment.
- Two complete reference collections of 30 images each and a local art review gallery.

## Acceptance still open

The [current whole-brief review](artistic-review-v15.md) returns **rework**. The
playable surfaces, structural supports and inhabited architecture remain below the
governing references. The new face, fish, windows and mural have observed responses,
connecting material culture with behavior, but do not settle the broader quality
target. Source-asset technical verification and the repaired window-pane bounds
are separate from artistic acceptance.

The slice still has no completed ending or full deeper discovery structure. The
hunter's stone conservation and responses are tested, but cairn shape, warm-seat
occlusion and grasp transitions still need craft work. The native,
face-mouth bubbles, remaining response-table
dressing and final sound pass remain unfinished. Implemented responses are described
in [city-response-implementation.md](city-response-implementation.md) and
[hunter-implementation.md](hunter-implementation.md); automated and
controlled visual fixtures do not establish spontaneous discovery or immersion.
No new gates or player verbs conceal the remaining gaps.

Further production should refine the playable materials, hero objects and creature
craft from the reference collection, implement the existing response tables through actual observed
toy interactions, and run blind playtests with sound. Record what players discover
and misunderstand. AAA craft, immersion, sensory comfort, wordless teaching and the
full design law cannot be marked complete from automated preservation tests.

Current technical evidence is in [verification.md](verification.md); source-derived
principles and a concrete blind-play protocol are in
[design-principles.md](design-principles.md).
