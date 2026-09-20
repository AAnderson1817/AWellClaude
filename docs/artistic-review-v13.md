# Whole-brief review — material revision v13

**Verdict: rework. Overall release state: work_in_progress.** The new material
library is useful surface work, but the actual game still misses the requested
AAA visual standard. The dominant gaps are foreground form and construction,
hero-object finish, and incomplete authored inhabitants/discovery. Human
immersion, sound comfort and unaided understanding remain **unverified**.

Review date: 2026-09-20. This assesses the complete original request: extend the
existing repository on a branch; create a AAA 3D version retaining a 2D perspective
and animation-style layered depth; provide 30 cohesive references per environment;
vigorously confirm the design principles; create immersion; interweave metallurgy,
astronomy and faith so that players ask questions and seek answers. A surface-map
checkpoint does not remove any of those requirements.

**Authorship disclosure:** this reviewer wrote the runtime texture packer, the
earlier city response simulation and its tests, and the original architecture
alignment checker. The reviewer did not create these source materials, the
renderer, environment geometry or hero assets. The image assessment is independent
of those makers. Packaging validity and earlier response code are not presented
as independent artistic approval or human acceptance.

## Evidence represented by this verdict

The primary evidence is the final native 1920×1080
[Vault Mouth](evidence/materials/native/with-materials/room-0.png) and
[Drowned Quarter](evidence/materials/native/with-materials/room-1.png), compared
with the matching [Vault control](evidence/materials/native/without-materials/room-0.png)
and [Drowned control](evidence/materials/native/without-materials/room-1.png).
The control disables the surface texture integration; it retains the same scene
and geometry. All five current renderer/asset hashes matched the
[manifest](evidence/materials/native/manifest.json), captured at
`2026-09-20T20:59:23Z`. This is the final packed basalt revision, not the earlier
candidate-native images.

The governing benchmark remains both complete 30-image reference collections,
their room views and the previously inspected column, cornice, raw-rock seam and
drowned doorway details. The references establish coherent built forms, carved
transitions, material-specific wear and selective detail at game scale. Their
invented collision layouts and explorer costume are not required replacements
for this game's authored map or player identity.

For this increment I also inspected the city-stone source/reimport slab pair,
its unlit 3×3 repeat, and the lit basalt and timber repeat views in
`assets/blender/review/materials/`. Source/reimport appearance agrees in the city
pair. The current `LifeStep`, `CityStep`, prop grid, premise and response report
were checked to establish which content gaps remain. The earlier
[v12 review](artistic-review-v12.md) supplies dated evidence for unchanged work;
its verdict was not simply carried forward.

## Requirement disposition

| Requirement | Current result | Evidence and limit |
| --- | --- | --- |
| Existing repository and branch | Satisfied for the work reviewed | The existing C/raylib implementation and embedded delivery continue; no replacement project. This review does not establish publication of this checkpoint. |
| 3D treatment with retained 2D view and layered depth | Present | The camera stays locked, modeled foreground remains on the authored play plane, and narrow supports preserve views through to the distant city. Near construction still reads flatter than the benchmark. |
| At least 30 reference images per environment | Satisfied for the two existing environments | The 60-image library and sub-element coverage remain unchanged from the previously verified collection. |
| AAA visual splendor | **Fail** | More coherent surfaces do not resolve the visible form, interface and hero-carving gaps described below. |
| Splendor without sensory overload | Visual hierarchy retained; overall comfort **unverified** | The texture change does not introduce an obvious busy overlay or remove important silhouettes in these frames. Still images do not establish motion or audio comfort. |
| Cohesion across every element | Partial | Warm hunter flame/metal and cool city stone/light remain consistent, but foreground craftsmanship and unfinished inhabitants prevent complete acceptance. |
| Vigorously confirmed design principles | Partial; full requirement not met | Current rendering preserves tested original behavior. Ending/deeper-layer design is unfinished, and cold-player teaching, surprise and dread tests remain absent. |
| Immersion | **Unverified** | Atmosphere and responses support the intention; no human session establishes sustained immersion. |
| Metallurgy, astronomy and faith provoke inquiry | Partial; interpretation **unverified** | Metal joints and repairs, orbital instruments, the disc procession and stone-responsive face suggest a shared material culture. Player recognition and investigation have not been observed. |

## Findings, in priority order

**1 — Foreground form and construction remain a visual blocker.** At 1080p, the
Vault's left rock face and ceiling still present rounded mottled masses inside a
conspicuously stepped outline. The lower-right wall and both Drowned building
fronts remain broad grids of smooth blocks, uniform dark joints and regular edge
profiles. The recessed openings now give the fronts architectural purpose, but
their surrounding mass still lacks the reference's varied dressed returns,
structural transitions and local wear. These are large visible forms, not missing
microscopic texture detail. Work on representative wall/rock modules and their
connections under the actual camera and directional lighting, while preserving
exact playable edges, should precede stronger texture contrast.

**2 — Hero-object integration remains a visual blocker.** The face's rectangular
mount, broad smooth cheeks and simplified mouth still resemble a shallow modeled
mask beside the more richly resolved distant city. Its glowing eyes are legible;
the response does not repair the carving. The Vault mural's backing still reads
as a separate irregular slab. In these captures the pigment is concealed by the
nearby light, consistent with the existing rule; the slab outline remains obvious.
Improve carved planes, edge transitions and seating into the surrounding wall.
The spiral door and astronomical instrument retain a clearer material identity,
but do not establish a uniform finish for the rest of the scene.

**3 — The living-world and discovery scope is still incomplete.** Current
`LifeStep` advances bushes, birds, the ledge animal and speaking plants; it contains
neither the authored hunter nor the aquatic native. `CityStep` still supplies the
mural, windows, face and shoal responses. The Drowned `PROPS` grid remains empty,
so its specified falling roof pots, hanging rope/banner responses and submerged
camp evidence have not appeared through that system. The modeled facade openings
are delivered separately and are **not** missing. Face-mouth bubbles, the remaining
inhabitant/ambience work and a completed ending/deeper discovery sequence remain
known gaps. These are failures of whole-goal completeness, not merely a lack of
review data. The postponed second verb must follow the premise's play-and-density
process; adding one speculatively would not satisfy that requirement.

**4 — The material change helps locally but does not close those blockers.** The
paired images show restrained variation on lit stone and rock faces rather than
a broad change in composition. The player, narrow landing edges, fish and face
eyes remain legible, and the supports do not recreate the rejected broad-wall
occlusion. I see no conspicuous new texture border or block-shaped artifact in
these stills. Timber remains a dark, narrow mark from this camera; its fine grain
does not by itself make the shelf read as a convincing constructed volume.

The city material's unlit repeat reveals recurring light and dark patches despite
continuous tile boundaries. That is a real library limitation, though it is less
prominent than the geometric grid in the actual rooms. The basalt repeat is a
restrained shallow surface, not a substitute for angular geological cleavage.
Timber grain remains clean and parallel compared with the reference's torn fibers
and hewn edges. Lowering repeated broad color contrast and adding object-specific
interfaces would be more useful than stronger global bump or indiscriminate
weathering. These source findings do not justify claiming a visible seam where
the native images do not show one.

**5 — Existing foliage and framing gains survive.** The rooted plants, unequal
frond heights, open space between rear spines and separated distant city planes
remain intact. Repeated plant clusters and parallel support rhythms are still
recognizable, but they are secondary to the construction and hero-object blockers.
Preserve the current quiet hierarchy while improving those larger forms.

## Technical evidence and human limits

The [runtime package](evidence/materials/runtime-package.json) records six 512²
textures, preserved source hashes, +Y normal filtering, exact embedded bytes and
memory estimates. Those checks concern delivery, not visual success. The current
[architecture/material integration audit](evidence/materials/architecture-material-integration.json)
passes contact/mask and per-mesh binding checks, including excluding longitudinal
wood texture from modeled endgrain. The current
[rendered preservation report](evidence/materials/render-preservation.json)
records 6,640 baseline comparison frames. Original audio events are preserved;
the separately counted city murmurs and hums remain intentional extensions.

The native benchmark records mean frame-loop times of 3.1674/2.5668 ms with maps
and 3.0954/2.4301 ms in the control for Vault/Drowned, over 600 measured frames
after 120 warmup frames. These are local wall measurements, not isolated GPU
time, hardware-wide performance acceptance or proof of comfortable motion.
This review used stills at 1080p; it does not certify smaller-view readability,
temporal texture stability or current browser rendering.

Wordless presentation, a locked camera and the unchanged Hold responses remain
consistent with the implemented design direction. The light/mural reversal and
stone/face acknowledgment offer real second readings. But the current surprise
log still chiefly documents older saltworks/gaff experiments; it does not prove
the present complete discovery arc. Human play with sound must establish what
people infer, try and miss, including whether the drowned doorways imply access
that the simulation never acknowledges. No listening or cold-player session was
performed for this review.

Retain the restrained material delivery, then prioritize construction and hero
forms, authored inhabitants and completion structure. Evaluate those changes in
the real game and record human discovery/comfort evidence before a fresh whole
brief release review. This checkpoint remains **work_in_progress**.
