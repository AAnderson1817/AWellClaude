# Independent artistic and affordance review

Reviewed 2026-09-20. The reviewer did not edit the renderer or models. This is an independent agent assessment, not user approval or a human playtest.

**Current judgment: v10 is a clearer, more coherent 3D presentation of the existing two-room slice than v9. It does not yet fulfill the whole AAA reinterpretation brief.** The concrete v9 response regressions listed below have been repaired in the reviewed source. Water contact and depth separation visibly improve in the supplied v10 frames. Remaining artistic, production, and experiential requirements stay open; passing simulation tests and delivering models cannot settle them.

## Evidence and scope

Read against the governing [design law](../claude/DESIGN-LAW.md), [premise](../claude/PREMISE.md), [room response tables](../claude/ROOMS.md), and [production brief](production-brief.md). The review also compared relevant original drawing code in `room.c`, `props.c`, `player.c`, `items.c`, and `life.c` with `depth.c`.

Images directly inspected:

- Original [Vault Mouth baseline](../assets/review/baseline/f0005.png).
- v9 [Vault Mouth](../assets/review/depth-v9/vault/f0005.png) and [Drowned Quarter](../assets/review/depth-v9/drowned/f0120.png).
- v10 [Vault Mouth](../assets/review/depth-v10/vault/f0005.png) and [Drowned Quarter](../assets/review/depth-v10/drowned/f0120.png).
- Governing references for [Vault Mouth](../public/art/references/vault-mouth/01-vault-mouth-governing-view.png) and [Drowned Quarter](../public/art/references/drowned-quarter/01-drowned-quarter-governing-view.png).

These are selected native captures at 1280 × 720. The renderer's 1920 × 1080 render target is source evidence, not an independent 1080p visual inspection. This bounded review did not run the game, listen to its sound, inspect every reference, or observe complete interaction sequences. A source-verified response is distinguished below from a response actually observed in a frame. Asset technical reports and preservation tests are supporting records, not substitutes for this distinction.

## Independent v9 findings

The locked composition, original traversal layout, readable player, and generally identifiable landing lips survived the move to 3D. Distant architecture suggested a much larger city and provided atmospheric depth. The restraint in color and the absence of interface clutter were appropriate.

The principal visual problems were:

1. **Water contact was ambiguous.** In the Drowned Quarter frame, a nearly invisible waterline made the floating body resemble a body hanging in air. This weakened the lamp/stone buoyancy teaching that depends on seeing the medium.
2. **The decorative orrery occupied the interactive stage visually.** Its crisp dark silhouette and shelf-like base appeared amid reachable platforms. It attracted an interaction hypothesis that the renderer supplied no response for.
3. **The detailed matte and playable geometry did not share a convincing material language.** Repeated smooth blocks, clean platform strips, and primitive-looking foliage/props sat in front of a much richer city image. Broad forms were readable, but the scene looked assembled from different fidelity levels.
4. **The vault/city transition was weak.** The cold grade dominated hunter timber, cloth, raw stone, and city masonry alike. The starting door remained an anchor, but the camp was difficult to read as evidence of a previous inhabitant.
5. **The Drowned Quarter lacked its defining inhabited scale.** The face, native and fish described in the response tables were absent. The frame conveyed submerged architecture more strongly than an indifferent occupied place.

The source comparison also identified these original-presentation losses: a bulb crown below its real collision height, the dead lamp's missing nearby-lamp glint, missing bush proximity lean, whole-body tint when only part of the player was underwater, and a reset fade drawn over the eyes that originally remained legible after the fade.

## v10 reassessment

| Finding | Current evidence | Judgment |
|---|---|---|
| Bulb crown below the bounce contact | `BulbMeshes` now derives height from `(BULB_H - press) / TS` and centers the dome at half that height, retaining a fixed base. The unpressed top matches the six-pixel collision crown. | Source repair confirmed. v10 also restores the original pink bulb palette, clearly separating bulbs from bushes. An actual landing sequence remains useful to judge contact and compression. |
| Dead lamp no longer acknowledged the live lamp | `PR_PACK` restores the original `LampPos` proximity checks and frame phase. The dead material is dark and non-emissive; a separate small glint is conditional. | Source repair confirmed. The selected frame does not show this event, so glint visibility at play scale remains unobserved here. |
| Bush proximity acknowledgment missing | `Foliage` now adds a lean away from a nearby player, in addition to the inherited shake input. | Source repair confirmed. Its new vertical proximity condition is sensible for the spatial presentation; no moving before/after sequence was supplied for visual verification. |
| Whole body tinted by partial immersion | The body uses its dry material; the masked water compositor tints only the water region. Absorption and a narrow surface accent are stronger. | Visibly improved: the v10 player intersects a readable surface, with a submerged portion/reflection beneath it. The frame no longer primarily reads as suspension in air. Wave/contact behavior needs motion review. |
| Reset fade suppressed the original eye cue | `ResetEyes` projects the eye positions and draws the appropriate open/half-closed state after the fade. | Source repair confirmed. Closure and reopening timing were not observed in these two frames. |
| Orrery falsely read as part of the stage | Stronger depth fog reduces its contrast substantially; the extra geometric canopy has been removed. | Improved. It now reads materially farther away. Its horizontal plinth still warrants a cold-player check, particularly in the Drowned Quarter. |
| Primitive small prop silhouettes | Modeled pots show identifiable mouths, rims and shoulders. Modeled hunter lantern and rigid creature bodies are integrated. | Improved at target scale. Lantern, body, and creature refinements are subtle in the wide capture; successful mesh integration alone does not establish final material or animation quality. |

**No remaining blocking instance of the five named original-response regressions was found in the current source.** This statement is deliberately narrower than certifying every response, every frame, or the entire renderer. The water fix has direct frame evidence; most of the small conditional reactions still need an observed sequence.

The response-table face, native, fish, reactive windows, mural and hunter have no implementation in the inherited current gameplay source or the 3D renderer. They are outstanding project design content, not regressions caused by this presentation branch. Adding them would be a deliberate gameplay/content step, not a cosmetic renderer repair. Do not imply that a decorative image of one has fulfilled its response contract.

## Subsequent archive and runtime evidence

After the bounded v10 review, production completed **30 final reference PNGs per
environment**. The root reviewer inspected both final contact sheets for broad
cohesion and distinct subjects. The library's decode/hash checks and individual
caveats are documented in [reference-library.md](reference-library.md). This closes
the numerical reference-delivery gap below, without changing the artistic verdict.

Actual native bulb, lamp, fire and reset captures, including enlarged inspection
views, now supplement the source findings in
[response-review.md](evidence/current/response-review.md). Both rooms also received
live browser rendering checks, recorded in [web-smoke.json](evidence/web-smoke.json).
These later checks do not constitute human immersion acceptance.

## Whole-brief judgment

| Requirement | Current assessment |
|---|---|
| Preserve this game and its fixed 2D perspective | Strong evidence for preservation of the inherited slice. The selected frames retain its room topology and locked side view. Source snapshots keep presentation separate from simulation. Traversal reports should be read at their documented revisions. |
| Classic multiplane-inspired layering | Partly achieved. Dark frame edges, an interactive stage, deep architecture and haze establish layers. The middle architectural layer remains thin, so some platforms resemble strips placed over a painting. More camera motion is not the remedy: the fixed frame is a design requirement. |
| AAA environmental quality and splendor | Not achieved. The distant city supplies scale, but the playable surfaces lack comparable construction detail, variation, contact shading and coherent wear. New meshes improve individual silhouettes without resolving the whole composition. |
| At least 30 distinct references per existing environment | Incomplete at this review snapshot. The archive manifests contain 30 Vault Mouth entries and 4 Drowned Quarter entries. Only the governing views were independently inspected here; counts do not certify distinctness, suitability or faithful implementation. Recheck manifests when the ongoing reference work changes them. |
| Metallurgy | Present as bronze, iron and patina, most clearly at the door and instrument. Foreground metal still needs more legible manufacture and use: restrained seams, fastening, wear and moisture-related oxidation. Uniform procedural noise is not sufficient material history. |
| Astronomy | Legible through concentric door/instrument forms and distant architecture. The orrery must remain unmistakably outside the usable stage. Repeated orbital motifs should support a culture rather than become a collection of unexplained switches. |
| Faith | Weakly established. Monumental/Gothic architecture alone does not communicate practice. The premise calls for evidence in repeated care, placement, wear and inhabitants' actions. The quiet camp/cairn is a useful beginning, but the reviewed frames do not yet make this cultural layer convincing. |
| Splendor with sensory restraint | The restrained palette and broad quiet regions are promising; v10 avoids a bloom-heavy spectacle. Motion comfort, sound balance, response salience and the optional still-atmosphere experience were not assessed by these stills. |
| Wordless affordances and animal indifference | Existing code retains relevant reactions; the repaired responses restore important acknowledgments. Whether a new player notices, interprets and experiments with them remains untested here. The missing Drowned inhabitants limit the intended indifference and scale in that room. |
| Rigorously tested Animal Well design principles | Preservation tests address implementation risks. They do not establish an unplanned toy use, successful wordless teaching, an unsettling but nonmalicious world, or a convincing second understanding. No such blind-play evidence was reviewed. The inherited unfinished ending/deeper-layer structure cannot be certified through a renderer pass. |
| Editable, verified assets and delivery targets | Asset documents report editable Blender sources, clean reimports and measured budgets. This reviewer inspected assembled target frames, not all source/reimport renders. Final asset acceptance, full-scene performance and web-target behavior require their own current evidence. |

The appropriate release description remains a **review candidate for the existing slice's 3D presentation**. It should not be described as a completed AAA game, a full realization of the response tables, or a demonstrated fulfillment of every design law.

## Priorities after v10

1. **Complete response visibility evidence.** Capture short, controlled sequences for bulb contact, live/dead lamp comparison and glint, bush lean, reset eyes, door glint, pot knock/fall/shatter, persistent fire, and water entry/exit. Compare selected events with the original presentation. Look at 720p and 1080p; a matching simulation trace does not prove the cause and effect are readable.
2. **Make the playable materials belong to the same place as the matte.** Refine support masses, undersides, joints, localized wear and contact shadow around the existing door, camp, balcony and column. Keep exact collider lips and retain generous quiet surfaces. Do not fill every empty region with ornament.
3. **Clarify the hunter/city distinction through materials.** Give canvas, weathered timber, raw rock and iron separate, subdued responses. Let the lamp and fire produce local amber. Keep city emission green-white and distinguish reflected warmth from a city light source.
4. **Resolve the Drowned Quarter's content scope honestly.** Its governing reference depends on the colossal face and an indifferent underwater ecology. Either schedule their original response-table implementation as a separate authored step or explicitly retain the narrower presentation-slice claim. A new gate, invented puzzle, or unresponsive set piece would not fix the experience.
5. **Finish and review the reference archive.** Complete the Drowned Quarter collection with genuinely distinct images. Preserve the manifest's notes about unsuitable generated details, and do not transplant reference compositions that conflict with the frozen geometry or the light rule.
6. **Observe cold players with sound.** Test whether they locate the lamp, understand putting it down, notice an acknowledgment, experiment with the stone/water relationship, and distinguish the instrument from a reachable object. Record actual unexpected uses and comfort observations. Keep production checklists outside the wordless game.

No conclusion in this review treats asset counts, code size, triangle counts, or successful automated tests as evidence of immersion or artistic completion.
