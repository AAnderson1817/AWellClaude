# City prop acceptance

**Asset release state: review_candidate.** Editable models and runtime deliveries exist and have been visually reviewed. This record does not certify the whole game's AAA goal, immersion, or reference-library requirement.

The source repository correction changed the active brief to the existing Vault Mouth and Drowned Quarter. The accepted scope here is a sealed five-by-six-tile spiral door and distant decorative city orrery. The original bell concept is not integrated.

| Gate | Status | Evidence and judgment |
|---|---|---|
| Identity and silhouette | pass | `assets/blender/review/door-front-clay.png`: solid round-headed portal, six petal relief, distinct center; `orrery-runtime-reimport.png`: three nested rings, swept supports and a weighted plinth. |
| Construction | pass | Door front/side/back clay renders show genuine depth, seated jambs, continuous sealed backing and plate thickness. Source orrery views show foot plates, opposing bearings and rear support struts. Rear of door is intentionally plain and wall-mounted. |
| Detail hierarchy | pass | The independent root-agent review found the six-petal silhouette and hub readable at the intended 160–300px scale. Its blocking concern about hovering spiral fragments was repaired by removing the separate bead; the six curved patinated seams now carry the spiral. |
| Material identity | pass | Current source and clean reimport studio renders show broad bronze response, basalt rest areas and localized patina. City glass emits green-white only; no runtime amber-emitting city material. |
| Delivered geometry | pass | `assets/blender/verification.json`: measured imported triangles, indexable material meshes, zero degenerate triangles and zero invalid normals. Source and delivery representations are separate. Main petal curvature is preserved; lower-priority delivery geometry is reduced. |
| UVs, textures, bakes | not_applicable | Geometry/material-color representation contains no image texture or normal-map dependencies. |
| Portable source and interchange | pass | `celestial-foundry.blend` reopened; each GLB imported into a clean scene; bounds and triangle counts agree with the C header generation. Source materials are supported Principled metallic/roughness. |
| Destination presentation | review_candidate | Current `assets/review/depth-v10` captures show successful integration, subdued distant instrument and local bronze patina. Independent [artistic review](artistic-review.md) confirms improved depth separation while leaving material coherence open. Studio appearance alone does not establish final shading acceptance. |
| Overall AAA/game acceptance | unverified | Requires the full game's environmental composition, performance, playtesting, sensory hierarchy and reference-library work. These two props cannot establish that result. |

## Repairs made during review

- Removed a floating continuous spiral bead which crossed the petal relief as detached fragments. The sealed petal seams retain the spiral identity.
- Removed collapsed bevel triangles from delivery meshes after decimation; preserved source models and authored normals.
- Kept the door's broad petal geometry at source resolution while reducing low-importance details, preserving the combined runtime triangle budget.
- Corrected active city materials from the earlier speculative palette to the repository's green-white light rule.
- Excluded the factory scene and staging floor from GLB selection after detecting unwanted selected objects in the first exports.

## Remaining observations

- Minor: broad bronze surfaces are stylized and cleaner than excavated metal. Patina is localized in recesses. This is deliberate to keep the 160–300px reading calm; final room lighting may warrant adjustment.
- Minor: stone cap bevels and reduced orrery rims show small irregular highlights under a 900px studio macro view. Check them at target scale before spending more geometry.
- Unverified: final root-game shader, emission response and frame-time cost with both props plus the remaining scene. No animation or moving-gimbal clearance is claimed; runtime assets are static.

The independent review was performed by the root agent from the current brief and actual door image. It was not user approval. Final source, clean imported models, and target capture should be assessed together before promoting this release state to final.
