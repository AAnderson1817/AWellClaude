# Whole-brief review — depth v11

**Verdict: rework. Overall goal and artistic release state: work_in_progress.**
The branch makes real progress toward the requested game. The 60-reference
requirement is satisfied for its two existing environments, and the new city
responses give the themes observable consequences. The actual playable surfaces,
construction and creature presentation remain substantially below the governing
visual references. Neither immersion nor complete design-law acceptance is proven.

Review date: 2026-09-20. This assesses the user's complete brief: extend the existing
repository on a branch; a AAA 3D rendition retaining the 2D view and animation-style
layered depth; 30 cohesive references per environment; tested Animal Well design
principles; immersion; metallurgy, astronomy and faith provoking investigation.
No unfinished content or known visual gap is excluded from the verdict.

**Authorship disclosure:** this reviewer authored the new carved face source asset.
Its assessment here is self-review, not independent certification. This reviewer
did not implement the renderer, response simulation or mural. The renderer owner
independently rejected an earlier face and requested the subsequent form repairs.
Another reviewer should assess the final whole game before artistic release.

## Evidence inspected

- Actual 1280×720 room captures: [Vault Mouth](../assets/review/depth-v11/vault/f0005.png)
  and [Drowned Quarter](../assets/review/depth-v11/drowned/f0120.png).
- Actual 1920×1080 room captures: [Vault](../assets/review/depth-v11/1080-vault/f0005.png)
  and [Drowned](../assets/review/depth-v11/1080-drowned/f0120.png).
- Native before/after images and their state logs in
  [city response evidence](evidence/city-responses/native/): mural dark/lit,
  window lit/dark, face before/answer/after, shoal capital/floating lamp.
- Both governing room references, the face reference, and both complete 30-image
  contact sheets. A separate read-only check decoded all 60 originals, matched
  their dimensions and SHA256 values to the manifests, confirmed 30 sequential
  IDs per room, and found 60 unique hashes.
- `claude/DESIGN-LAW.md`, `claude/ROOMS.md`, `SURPRISES.md`, current renderer and
  city response code, city response tests and current native/headless results.
- Face source, clay and clean GLB-reimport renders, plus its measured export
  verification. These establish that the asset is real editable geometry; they
  do not establish AAA quality.

The references govern craftsmanship and visual direction, not replacement
collision layouts. The existing footprints, controls, player identity and locked
camera are legitimate constraints. This review does not require copying the
reference's invented walkways or introducing new game verbs.

## Requirement disposition

| Requirement | Current disposition | Evidence and limit |
| --- | --- | --- |
| Existing repository and branch | Satisfied for the work reviewed | Changes extend the existing C/raylib modules and embedded asset pipeline; no replacement application. Publication of the latest commit remains the repository owner's final check. |
| 3D treatment with a retained 2D view | Present | Modeled terrain, props, face and mural are viewed through a fixed camera. Matte architecture, fog, playable geometry and edge foliage form distinct depth planes. No scrolling or added depth controls. |
| At least 30 references per environment | Satisfied for both existing rooms | 30 decoded unique PNGs per room, with prompts, subjects and review notes. This does not establish that the corresponding runtime objects match their quality. |
| Thematic cohesion of every element | Partial | The reference collections share weathered stone, cast/forged metal, warm hunter flame and cool city light. The runtime preserves that broad family, but small flat-looking foliage, smooth modular masonry and simplified props lack the reference's common level of craftsmanship. |
| Visual and sensory splendor without overload | Not achieved / partly unverified | The layered distant city establishes scale and the view is restrained. Foreground construction and material gaps prevent the requested visual finish. Sound comfort and prolonged motion were not assessed by this still-image review. |
| Vigorously tested design principles | Partial | Route/escape, original behavior preservation and new city-response tests provide meaningful technical evidence. Cold-player discovery, self-teaching, dread, an ending and deeper-layer revelation remain unverified or incomplete. |
| Immersion | Unverified | Coherent reactions now reward actions, but human attention, curiosity and sustained immersion cannot be inferred from a working build or screenshots. |
| Metallurgy, astronomy and faith interwoven | Material progress, incomplete acceptance | Bronze repair, repeated orbital forms, the disc procession and a stone-responsive face connect the themes through objects and behavior. Whether players infer these relationships remains untested. |

## Consequential findings

### Foreground construction still falls below the art direction

**Blocking for the AAA target.** In the 720p Vault image, timber shelves around
x190–770, y135–490 read as thin horizontal bars over a much richer city painting.
The center column is a plain fluted extrusion; its capital and connections are
far simpler than the governing column/architecture references. The top rock boundary
still exposes a stepped rectangular grid. In Drowned, the two large island fronts
(roughly x96–352 and x992–1152) remain repeated flat ashlar fields with little
variation in construction or use. They do not yet read as convincing flooded homes.

The new shader adds bounded mineral and oxidation variation, but its `contact`
factor depends on world Z alone. It is not evidence of occlusion at actual joints.
Surface variation cannot supply the missing brackets, dressed returns, cornice
profiles or facade openings. Build and inspect those forms while preserving every
existing collider and clear landing lip. The original 2D view does not excuse a
large quality discontinuity between the playable stage and the distant matte.

### New cultural objects are readable, but remain unevenly finished

**Blocking for the complete visual target.** The procession makes the Vault's lower
left wall meaningful, and its pigment disappears under the lamp in the actual
captures. The bright figures are legible without text. The rough wall backing still
reads as a separately placed plaque when lit, and its material integration with the
adjacent raw rock needs further work.

The face is now identifiable in deep water and its eyes visibly brighten when a
stone sinks nearby. In the 1080p view, the nose's deliberately planar insert and
repaired lower eyelids are clear. The broad mouth transitions and uniform stone
field still look softer and more manufactured than the reference's carved mineral
mass. These are not grounds to hide the asset; they are unresolved craft tasks.
Its source and export are technically sound, and the whole carving stays behind
the gameplay plane. This reviewer authored it and does not waive its shortcomings.

Foliage remains very simple flat leaves or sparse bristles at many anchors, while
the references describe rooted, weight-bearing plants. This is a visible style and
craft gap at normal play size, not a request to add more constantly moving detail.

### Window aperture defect found and corrected during review

**Resolved with new image evidence.** The first 1080p Drowned capture showed a green
semicircle below the center upper window's cornice around x790–890, y170–195.
The renderer used a full ellipse for the arched glass.
For a three-tile-wide, two-tile-high window, its lower half extends about .57 tile
below the window bottom. The pane needs an actual upper half-disc plus the existing
rectangle. The renderer owner implemented `ArchPaneMesh`, a true upper-half fan,
and produced a new 1080p Drowned capture. This reviewer inspected the replacement:
the hanging green semicircle is gone, the pane stays within its stone frame, and
the cornice's visible landing edge remains clear. This is separate from the crossing
figure's path, which is constrained to the inscribed arch region without changing
body dimensions. The fix closes this bounds defect; it does not change the overall
artistic verdict. Final WebGL rebuild/recapture verification is the integration
owner's remaining platform check, not a result inferred from this native image.

### The response work improves discovery but does not complete the living world

**Blocking for whole-goal completion.** The new interactions are visible and
appropriately restrained:

- The mural's native states are 1.0 in darkness and 0.0 under the lamp; the image
  genuinely disappears while its stone remains. Automated tests additionally
  exercise the persistent fire and two-second return in darkness.
- The window is visibly green when left alone and dark beside the lamp. Tests
  exercise all four windows, departure waits and renewed interruptions.
- The face fixture records one response to an actually sinking stone; frame 48
  has acknowledgment 1.0, frame 180 returns to 0.0. The observed light response
  supports the recorded two-second answer. No movement or opening is implied.
- Eight fish visibly gather higher under a floating lamp. The native mean Y moves
  from 84.448 to 67.041 room pixels. Tests also exercise held-lamp exclusion,
  disconnected pockets, body scattering and water containment.

These consequences improve the second reading of Hold: carrying useful light can
conceal something; setting weight down can receive an answer. The hunter and native
specified in `ROOMS.md` still do not exist with their intended state machines.
Several Drowned dressing/response items remain unfinished. There is no completed
ending or accepted deeper discovery sequence. `SURPRISES.md` largely describes old
saltworks/gaff experiments and unassigned rooms; it is not proof that cold players
discover the current lamp/stone relationships.

## Testing and perceptual limits

The current results record 35 route checks, 36 escape surfaces, 3,600 snapshot
frames, five tool tests, 21,160 headless and 6,640 rendered baseline comparison
frames. These check actual original state/event preservation, not whether the
new art is AAA. The new city hums and murmurs are intentional extensions, tallied
separately; the tests do not pretend they existed in the baseline.

The new unit/fixture checks cover meaningful positive and negative response cases.
They do not prove that players notice those responses, find the controls unaided,
understand affordances, feel unsettled or lose themselves in play. The new audio
was inspected as code and event counts only; no listening judgment is claimed.
Still frames also cannot establish comfortable motion over time. The quiet visual
hierarchy is promising, but sensory comfort and immersion remain open human tests.

Retain this revision's behavioral work and reference library. Prioritize coherent
foreground construction, material/prop craftsmanship, and completion of the
already-authored inhabitants and responses. Then conduct and record cold play with
sound, including what people try, misunderstand and ask. Completion requires those
observations plus a fresh whole-game artistic review; no passing check in this
record establishes the user's entire requested end state.
