# Architecture alignment and review contract

This is a bounded technical and visual review contract for the architecture pass
after depth v11 (`3d57199`). It does not certify AAA quality, discovery, sensory
comfort or immersion. The original room maps and movement remain authoritative.
The reviewer of this contract did not author the terrain, supports or renderer.

## Independent checks

Run from the repository root:

```powershell
python tools/check-architecture.py --self-test --require-integrated --report docs/evidence/architecture/current.json
```

The checker reads the **delivered C vertex, normal, material and index arrays**,
the actual `MAPS` declaration in `src/room.c`, and the camera/draw bindings in
`src/depth.c`. It does not trust the generators' run manifests or re-create their
mesh construction. Reports include source hashes. Changing an input invalidates
an earlier report for that input; historical reports are retained as dated evidence.

The locked perspective camera is at `(20,11,52)`, facing `(20,11,0)`. Its vertical
field of view is 24.415162 degrees, with a 16:9 render target. At Z=0 this maps the
original 320 by 180 pixel view to a 40 by 22.5 tile stage. There are 22 collision
rows and eight original pixels per tile. The independent projection is:

```text
screen_x = 20 + (world_x - 20) * 52 / (52 - world_z)
screen_y = 11 + (world_y - 11) * 52 / (52 - world_z)
```

A model's world-coordinate footprint alone is insufficient: a receding edge
moves toward the frame center under this perspective. Landing lips must remain
at their original projected locations. Rear construction can use deliberate
forced perspective without changing the camera or the collider.

| Gate | Measured requirement and rationale |
| --- | --- |
| Delivered storage | Array counts agree with mesh declarations, each mesh fits 16-bit indices, indices are valid and triangles are complete. Positions and normals are finite. |
| Surface validity | Normal length is within 0.02 of one; twice-area below `1e-10` is rejected; an average corner normal opposing its geometric triangle normal by more than 0.05 is rejected. This catches export slivers as well as gross winding faults. |
| Materials | At most 12 material meshes per asset; valid opaque RGBA; roughness and metallic in `[0,1]`; zero architecture emission. Support substrate count matches material count, using only approved codes: 0 metal/none, 1 stone, 2 raw rock, 3 wood, 5 rear rock, 6 rear stone, 7 modeled endgrain without longitudinal mapping. Terrain families are restricted to 0/1/2. Codes 5/6 give receivers their separate atmospheric treatment. City response lights remain separate authored effects. |
| Stage bounds | Frontmost geometry is no farther forward than Z=0, with `1e-5` numeric tolerance. Rear architecture stays at or in front of Z=-8. Projected bounds stay within a half-tile trim allowance around the 40 by 22 map. Declared extents agree with delivered bounds within 0.001 tiles. |
| Landing contact | All exposed solid tops and all one-way shelf runs are derived directly from the map. Along each run, probes every 1/32 tile must hit the front geometry half an original pixel below the landing and miss half a pixel above it. Half a pixel at each endpoint is excluded. |
| Front envelope | Triangles are clipped to Z>=-0.10 before projection. A half-original-pixel raster must stay within the authored solid mask; supports may also fill a fascia at most 0.55 tiles below a one-way landing. The mask has a half-original-pixel boundary tolerance. |
| Recessed supports | Geometry outside the allowed solid/fascia envelope must be behind Z=-0.45. Receivers may extend upward behind a cantilever; they must not masquerade as a new collidable front mass. |
| Closed terrain footprint | Orthographic XY coverage of the delivered terrain must match the original solid `#`/`*` cells at all 56,320 original-pixel samples per room. Recessed doors and windows must retain a sealed back surface. |
| Renderer binding | `--require-integrated` requires a recognized literal identity draw for each world-coordinate asset. The `ClassifiedAsset` loop must bind each terrain/support asset to its corresponding loaded model and per-material family arrays at identity. Metal, rear receivers and endgrain require their intended families. This is a source-level binding check, not a substitute for native rendering. |

Whole-depth projected pixels outside the collision mask and the fraction of open
stage covered by them are **diagnostic only**.
A visible rear return may legitimately project above a landing even though the
front edge is exact. Requiring every rear surface to equal the 2D collision mask
would remove the requested depth. Conversely, a deep backing can still mislead a
player despite passing these checks, so it needs an actual play-view review.

The first draft allowed only quarter-tile projected trim, equal to the frame's
vertical overhang. The narrow Vault receivers intentionally terminate at Y=-0.35,
0.10 tile **outside** the visible frame. That cannot cover a visible landing or
background. The gross off-stage guard was therefore corrected to half a tile;
the original failing [overscan report](evidence/architecture/final-v12-overscan-review.json)
is preserved. Front contact and collision-mask tolerances were not relaxed.

The self-test uses a small handwritten two-tile front, independent of both asset
generators. It verifies acceptance of a correct contact and rejection of shifted
and receded contacts, positive-Z intrusion, reversed normals, emission and an
invalid index. A separate handwritten shelf accepts a recessed receiver and
rejects that same receiver moved inside the rear stand-off. These demonstrate
bounded failure detection, not exhaustive proof.

## Play-view acceptance

The governing references are the two complete 30-image room collections in
`public/art/references/`. This contract inspected both full contact sheets and
the detailed column, raw-rock seam, drowned doorway and cornice references, along
with the actual v11 720p room captures. References govern craftsmanship and
construction language; their invented platforms do not replace the source maps.

The next native review must examine both rooms at 720p and 1080p, including a
moving player and a lamp near the dressed surfaces:

- A landing reads as a continuous, narrow front edge with a visible material
  thickness. The player's feet do not float above it or disappear into it.
- Timber has a seated joint or bracket and believable grain direction. City
  cornices have a coherent cap, fascia and corbel system. Grates retain open bars
  and metal attachments. Rear receivers meet those parts without coplanar shimmer,
  floating seats or repeated disconnected plaques.
- The city column reads as a load-bearing shaft, capital and base. Drowned
  openings have dressed returns and visible recess; they remain distinguishable
  from playable passages. Raw rock has broken geological form while its landing
  silhouette remains faithful to the collision map.
- New masses stay behind the player and preserve quiet negative space. The
  player, lamp, door, mural, fish and responding face remain legible at play size;
  added support detail must not compete with their responses.
- Contact shading is local to actual neighboring surfaces. Look for grain,
  stripes, halos, crushed joints and unstable edges in motion. A same-build
  enabled/disabled comparison is needed to attribute a change to the shading pass.
  Uniform world-Z darkening does not establish contact occlusion.

Automated masks cannot judge these points. They do not prove watertight meshes,
occluded topology quality, export parity with a GLB, lighting correctness, frame
time, actual input behavior, sound comfort or the player's interpretation of a
background doorway. Clean-source/import inspections and original behavior
preservation remain separate required evidence.

## Evidence history

- [Pinned v11 terrain](evidence/architecture/baseline-v11.json): both rooms passed
  the technical checks. This is compatible with the v11 artistic **rework**
  verdict and demonstrates why an alignment pass is not artistic acceptance.
- [Initial v12 terrain](evidence/architecture/terrain-initial-v12.json): the
  Drowned delivery failed with 26 invalid normals, 12 degenerate triangles and
  one reversed-normal sliver. Contacts and collision silhouettes passed. The
  terrain author repaired evaluated export geometry before both C and GLB output.
- [Intermediate v12 candidate](evidence/architecture/candidate-v12.json): the
  repaired terrain and first support delivery passed the asset checks. Its
  support renderer binding was still reported as unverified at that snapshot.
  This report is not final source/import or integrated visual acceptance.
- [Initial integration](evidence/architecture/integrated-initial-v12.json): all
  four asset checks and recognized identity draw bindings passed. The subsequent
  actual `depth-v12/support-vault/f0005.png` capture was **rejected visually**:
  broad, rectangular receiving walls obscured much of the distant city and
  collapsed the intended layered space. Narrower receiving piers are being
  revised separately. The technical pass does not waive that visual defect.
- [Final v12 technical report](evidence/architecture/final-v12.json): all four
  delivered assets, substrate arrays, adversarial fixtures and identity draw
  bindings pass after the narrow-pier and receiver-fog revisions. There are
  4,976 contact probes and 112,640 terrain mask samples, with zero mismatches.
  The final native capture manifest matches the checked renderer/asset hashes.
  [Independent whole-brief review](artistic-review-v12.md) accepts the repair of
  the broad-wall regression while retaining an overall **rework** verdict.

The complete brief remains `work_in_progress`. Human discovery, sustained
immersion and sensory acceptance remain untested, regardless of architecture
check results.
