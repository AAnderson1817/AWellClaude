# Material and composition review — depth v10

Review date: 2026-09-20. This is a fresh bounded review of the playable presentation, informed by the original AAA / 2D perspective / multiplane / sensory restraint brief. It is not a release approval. Reviewed source is the embedded `FRAG`, `POST`, `Background`, and terrain presentation in `src/depth.c` at commit `10498dd`. No renderer code was changed by this reviewer because shaders are embedded in the file being edited by the renderer owner.

## Evidence actually inspected

- [Vault Mouth, actual 1280×720 runtime](../assets/review/depth-v10/vault/f0005.png).
- [Drowned Quarter, actual 1280×720 runtime](../assets/review/depth-v10/drowned/f0120.png).
- [Vault governing reference](../public/art/references/vault-mouth/01-vault-mouth-governing-view.png).
- [Drowned governing reference](../public/art/references/drowned-quarter/01-drowned-quarter-governing-view-v2.png).
- `claude/DESIGN-LAW.md`, especially the affordance contract, quiet procedural motion, locked camera, and no text / combat / third verb requirements.
- The Blender AAA Assets skill and its artistic acceptance reference. The verdict below distinguishes an implementable material repair from the overall brief.

The reference images establish an artistic direction. They do not justify changing room footprints or treating pictured walkways as new collision surfaces. Their character design is not a request to replace the existing player.

## Three consequential changes available without changing geometry or verbs

### 1. Give the existing material families distinct surface scales

**Blocking for the material quality target.** In the actual Drowned image, the broad left wall (roughly x96–352, y211–682), right wall (x992–1151, y210–711), and central column (x579–671, y391–704) have nearly uniform, clean blue surfaces. Their bevels are visible, but there is little difference between ashlar, column shafts, and rough masonry. In the governing reference, broad material shapes remain readable while local cracks, mineral color, and coherent wear give each architectural mass a surface history. Vault's rough rock is more varied, but the jump from that noisy roughness to very smooth city construction is abrupt.

**Source cause / intervention:** `FRAG` currently applies the same `grain` at `p*23`, sinusoid at `6.1/8.4`, and `age` at `3.4/12` to all objects. Replace this common seasoning with explicit material-family controls. An authored family parameter is safer than guessing stone from its blue color or high roughness; the same shader also draws player, plants, cloth, bone, and pottery.

- Stone: one broad world-space mineral band field, a weak medium-scale cloud, and very sparse flecks. The broad field should survive at 720p; the fine field should not become visible static. Keep most face values quiet. A small static normal perturbation can stop perfectly flat stone faces from resembling molded plastic without moving their silhouette.
- Metal: retain a coherent bronze foundation with oxidation in broad recess-like patches. Do not apply the same mottling to every metal uniformly. The existing thin bronze details must remain distinguishable from stone at the actual screen size.
- Organic / cloth / player: keep this architectural pattern out of them. Existing rigid character proportions and response animation remain protected.

World-space patterning is appropriate for static terrain. Moving portable objects should use an object-space material position, otherwise they swim through the procedural texture when carried; deriving both positions in the vertex shader requires no new geometry.

**Acceptance observation:** side-by-side actual 720p renders should show readable stone mass and weathering before zooming in, while the player, waterline, bulbs, and every platform lip remain at least as clear as v10. Inspect still frames plus a moving lamp to catch shimmering and texture swimming. A grainier image alone does not pass.

### 2. Make broad highlights describe substance and existing construction

**Blocking for the material quality target.** In Vault, the door has a coherent bronze shape, but its large face reads comparatively chalky; the center column remains a dark, almost featureless stripe. Drowned's columns and masonry similarly share a narrow light response. In the references, stone has broad quiet illumination, wet surfaces catch selective glints, and bronze has a distinct broader reflection response. This difference creates volume without requiring more moving effects.

**Source cause / intervention:** the current specular lobe uses one fixed `L=(-.45,.8,.6)`, `mix(92.,13.,roughness)` and amplitude `(.035+metalness*.75)`. `baked` affects its strength but not its color, while metal still retains the full diffuse contribution. Roughness broadens the exponent without correspondingly controlling energy. This is a stylized shader rather than a physically calibrated BRDF, so the next change should be judged visually, not described as PBR merely because it exposes metalness and roughness.

- Use distinct dielectric and metallic reflectance, with roughness reducing peak strength as it broadens the lobe. Preserve enough stylized fill to read the stage.
- Add a low-energy broad cool reflection term or grazing fill that reveals existing curved column/door surfaces. It should have a directional shape, not be a uniform exposure increase.
- Let the existing local light color influence the small specular contribution. City light stays green-white; the hunter lamp stays amber. Warm all-over bronze is not a substitute for this separation.
- If underwater wetness is added, bind it to the authoritative water region/height, not to a universal y threshold. Only intended material families should gain that response. Do not simply brighten every submerged face.

**Acceptance observation:** compare the door rim versus its face, center column shaft versus cap, and lit versus unlit stone within the same frame. Bronze, stone, and ceramic should remain recognizably different under both the portable amber lamp and city green-white light. Bright landing edges must not turn into a decorative chain of equally bright distractions. Verify the all-dark resting areas still exist.

### 3. Restore depth hierarchy and contact cues within the existing construction

**Blocking for the construction / presentation target.** Vault's distant matte architecture is richly articulated, while foreground ledges around x192–540, y168–400 are thin, high-contrast strips that look detached. Drowned's sidewalls remain much simpler than the complex city behind them. Consequently the image sometimes reads as flat platforms laid over a detailed painting. More background detail or particle motion would strengthen this mismatch rather than solve it.

**Bounded intervention:** protect the present z0 interaction plane and existing world layout. Add measured contact shading to *actual* intersections/recesses, using authored/baked vertex occlusion or a bounded geometry-derived field if available. Favor the column/capital seam, masonry mortar recesses, and actual wall/ledge contacts. A common dark gradient on every object is not contact occlusion. Treat exposed, unattached ledges honestly; shader shading cannot invent a load-bearing attachment.

The matte is currently tinted uniformly by `DrawBillboardRec(...,{88,112,112,255})`. Its depth hierarchy can be improved with a restrained presentation-only contrast/atmosphere control, tested against the playable silhouettes. Reduce local background contrast only where it competes with the foreground; avoid bleaching the entire image. The distant orrery must remain subordinate to the player and route, while still readable on deliberate inspection.

**Acceptance observation:** at thumbnail and normal play size, the eye should first find the player and nearby route, then architecture, then distant repeated motifs. Compare unchanged contact points and ledge silhouettes before/after. A ledge must never appear to rest on an invented traversable support. The smooth column must read as a mass with a coherent capital seam, not a vertical band stuck to the backdrop.

**Limit of this bounded intervention:** the conspicuously floating timber/stone platforms and simple box construction require authored support/form work to attain the reference's construction quality. Those changes lie outside this no-geometry review task, but remain within the user's overall AAA goal. Materials cannot close that gap alone.

## Whole-brief disposition

**Artistic: rework / fail against the complete requested target. Overall: work_in_progress.** The fixed perspective and restrained broad palette provide a useful foundation. The reference library is cohesive enough to direct production. However, current foreground construction, material specificity, and small prop/creature fidelity remain substantially below the generated visual benchmark. Astronomy is represented by concentric instruments; metallurgy is presently mostly a bronze color/material cue; faith is largely implied by monumental architecture. These themes need observable, coherent environmental relationships and responses, beyond decoration.

**Technical status for this review: unverified.** This task inspected images and shader source, not a new runnable candidate or test execution. Existing route/parity results cannot establish a future shader change's visual quality. Human immersion, no-instruction teaching, and the discovery / ending criteria cannot be confirmed from stills.

After implementation, render both rooms at the same player state and camera as v10, inspect the altered material regions and full frame, play the moving-lamp and submerged cases, and verify WebGL shader compilation. Reuse unchanged geometry evidence, but do not inherit the prior candidate's visual acceptance for changed shading. No completion claim follows from this review.
