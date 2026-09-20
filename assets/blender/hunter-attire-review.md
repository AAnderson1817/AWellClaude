# Hunter attire: frozen integration candidate

2026-09-20. The bounded modeling/export task is delivered; **overall asset state: work_in_progress**, awaiting native pose/material review. This is not an AAA or complete-hunter acceptance. I authored this model and its technical checks. Root independently inspected front, back, clay-interface and 96-pixel study views against the camp/pack references and approved primary construction for export. It explicitly withheld material finish approval pending the actual camera.

## What was inspected and corrected

The first study is preserved in `review/hunter-attire/primary-v01`. It established the broad soft brim and visible left/rear pack, but showed a band penetrating the crown, a round sack-like pack and two superficial raised wrinkle strips. Those were rejected. The band now follows sampled crown vertices with a thickness allowance; the pack has broad sewn panels, an overlapping flap and surface-following welts. Its few tension folds deform the fabric surface rather than sit on it. A fitted inner sweatband closes the visible head-seat gap.

Existing-eye-position guides were added only to the excluded reference collection, at the renderer's original 0.73-body-height location. The hat leaves them visible in front, three-quarter and native-scale studies. They are not new eye geometry in the GLB or header. The body remains the unchanged 6×11-pixel seed.

Source audit found three collapsed buckle quads caused by a tube-frame axis switch, although final triangles were nondegenerate. A stable per-path frame fixed the editable source. Non-destructive final triangulation now follows cloth-thickness modifiers, so the source's crown shading matches delivery. The final source audit has no evaluated-geometry warnings or errors. Base-mesh boundary warnings are the intentional editable cloth/strap surfaces before Solidify; the evaluated and imported meshes are closed.

Inspected final evidence in `review/hunter-attire`: source and reimport front/side/back/three-quarter/underside, neutral clay front/back/interface, wire and native-scale views. The 96×96 study gives the unchanged body approximately 36×66 pixels. The actual game camera, camp illumination, sitting posture, gaze, hand contact and voice synchronization are still required; studio size equivalence is not native integration evidence.

## Delivery and technical scope

- `hunter-attire.blend`: editable hat/pack collections, non-destructive thickness/triangulation, excluded reference body/eye guides and studio. Source reopens successfully.
- `../../public/models/hunter-attire.glb`: 19 named component meshes, 7,020 triangles, six unique materials, no external images. Clean reimport has zero degenerate triangles, invalid normals, boundary edges or nonmanifold edges.
- `../../src/generated/hunter_assets.h`: `FOUNDRY_HUNTER_HAT` (3,952 triangles, five material meshes) and `FOUNDRY_HUNTER_PACK` (3,068 triangles, four material meshes). Both use the seed's common ground-center origin and X-right/Y-up/Z-front axes. **Six distinct materials are shared across nine runtime material groups**; this is not a six-draw-call claim.
- Actual C float32 arrays pass finite-value, index, triangle-area, normal-length and bounds checks. Maximum normal-length error is 7.16e-7. A C99 include/descriptor probe compiled and executed.
- Ten matched source/reimport image pairs differ by at most one 8-bit channel value; mean absolute channel differences are below 0.00003 on a 0–255 scale. This establishes export consistency, not artistry.
- `life-props.blend` and `life_assets.h` hashes are preserved. Only this new asset and its reports were written; no existing renderer or simulation hook was changed.

Reports: `hunter-attire-manifest.json`, `hunter-attire-source-audit.json`, `hunter-attire-verification.json`, `hunter-attire-array-verification.json`. Manifest hashes identify the frozen files. Header SHA256: `da25a8245ad84339d99a2da8145bdda9f32f56f667e4f39a4f8b0959b867c7a9`.

## Acceptance disposition

| Gate | Status | Consequence |
|---|---|---|
| Primary identity and construction | pass for export | Distinct hat/pack silhouette, seated band, readable flap, eye clearance and continuous anchored straps seen in neutral studies. Root independently reviewed this gate. |
| Reference material finish | fail at close-up benchmark | Felt/canvas remain clean compared with the worn camp/pack references. Determine the consequential missing variation at native scale before adding detail. This is not waived by low triangle counts or the studio lighting. |
| Geometry and interchange | pass | Source audit, GLB reimport, actual C arrays and source/reimport comparisons agree within documented numerical precision. |
| UVs, textures, skeletal deformation | not_applicable to this delivery | Constant opaque PBR materials and rigid separate attire assets; no baked textures or deforming rig. |
| Native composition, movement and material response | unverified | Root must pose the attire with the existing seed body and inspect idle/warm/tend/carry/pickup states under the actual camp lighting. |
| Complete hunter / AAA immersion | unverified and incomplete | Hands, gestures, voice synthesis/synchronization, native review and human discovery/sensory acceptance are outside this frozen asset checkpoint. |

Use the root's next actual-camera review to decide whether finish needs material variation, a silhouette adjustment or pose repair. Do not increase ornament or wrinkle density to compensate for an unreadable gesture.
