# Shelf and support asset review

The editable [supports.blend](supports.blend) replaces the one-way platform boxes with modeled timber assemblies, carved stone cornices, curved braces/corbels, and recessed anchor spines. The source and exports are a verified integration checkpoint. Their release state remains `work_in_progress`: the requested AAA craftsmanship has not passed at the actual game camera.

## Construction and integration

| Asset | Runtime symbol | Triangles | Material groups |
| --- | --- | ---: | ---: |
| Vault shelves and supports | `FOUNDRY_VAULT_SUPPORTS` | 71,588 | 12 |
| Drowned shelves and supports | `FOUNDRY_DROWNED_SUPPORTS` | 7,184 | 7 |

Both assets use absolute runtime coordinates and an identity draw transform. The generated `*_SUBSTRATES` arrays assign each material group its runtime substrate: metal `0`, stone `1`, wood `3`, rear rock `5`, rear masonry `6`. The two rear codes permit receiver-specific atmosphere without changing geometry or front shelf materials. The original map provides all 16 one-way runs; each front lip remains at `z=0`, its exact original X span, and height `22-mapRow`. Receding geometry is calibrated to the existing perspective camera so its apparent contact surface stays on that line. No collision, room map, interaction, or camera behavior is changed by this asset delivery. [supports-manifest.json](supports-manifest.json) records the complete runs and transforms.

Timber assemblies have three thick planks through depth, cut endgrain, recessed growth lines, forged straps, pegs, seated bearers, and curved iron braces. Stone assemblies have a continuous calibrated cap, individual ogee courses, bronze keys, and curved corbel masses. The shaft grates have a thin continuous top rail with separate bar noses below it, which remain visible at the locked camera.

Receiving structures are only 0.46-tile chamfered spines at the actual anchor axes. Their front is approximately `z=-5.055` and their back is `z=-5.898`. Shared anchor intervals merge, and the spines connect into the floor or ceiling silhouette. The earlier wide receiving bays were rejected after native rendering because they covered the city depth and resembled flat plaques. They are absent from this candidate. Braces and corbels outside the near fascia remain behind `z=-0.45`.

The governing construction references are [the hewn timber and rope](../../public/art/references/vault-mouth/11-hunters-rope.png), [Vault column](../../public/art/references/vault-mouth/16-city-column.png), [balcony garden](../../public/art/references/vault-mouth/21-balcony-garden.png), and [Drowned cornice](../../public/art/references/drowned-quarter/20-carved-cornice.png). They establish weight, craft, and broad worn transitions; they do not authorize changing the playable silhouettes.

## Inspected evidence

The maker inspected both final source room views, the timber/stone/grate construction views, both clean GLB room reimports, and both clean detail reimports in [review/supports](review/supports). The source and reimports preserve the same construction, material separation, seating, and dimensions. [supports-verification.json](supports-verification.json) records reopening the saved source, exact exported triangle counts and dimensions, zero degenerate triangles, zero invalid normals, and SHA-256 hashes of the source and exports. The source hash is `451cd8bd5d6934c504eb022f95983b914ee69bea00e3aafdb6bceb343398cc9c`.

The actual [Vault camera](../../docs/evidence/architecture/native/with-occlusion/room-0.png) and [Drowned camera](../../docs/evidence/architecture/native/with-occlusion/room-1.png) were inspected after narrow-spine integration. These views prove that the large plaques are gone, the city remains open between the anchors, and the shaft grates resolve as repeated bars. The shared native images may receive later foliage/lighting updates; the source and export hashes above identify this geometry candidate.

| Gate | Status | Evidence and limits |
| --- | --- | --- |
| Editable source and exported geometry | pass | Reopened source; clean GLB reimports; matching counts/dimensions and valid normals. |
| Bounded support form replacement | pass | Actual timber depth, seated braces, profiled stone, and narrow rear anchor spines replace plain platform boxes. |
| Background openness | pass | Both native room views preserve broad open city depth between anchors after removal of the wide receivers. |
| Exact contact / projection / stand-off | pass | [Final independent report](../../docs/evidence/architecture/final-v12.json) checks the delivered narrow-spine header, including receiver substrate codes 5/6. All 1,620 Vault and 876 Drowned contact samples pass, with no front leaks or unrecessed off-mask geometry. |
| Reference craftsmanship at intended scale | fail | Much timber joinery and stone corbel shape collapses into dark narrow fascia in the game camera. Repeated straight anchor pairs remain visually regular. Source closeups reveal modeled construction but do not substitute for game-scale legibility. |
| Whole requested AAA / immersion result | fail | This support checkpoint does not establish the full game's artistic or sensory acceptance. |

## Remaining artistic findings

`blocking` for the broader AAA gate: in the Vault native view, the dark timber shelves at the upper left and middle left retain the impression of thin bars because plank, brace, and receiving-spine values merge. The warmly lit upper shelf shows more construction, so the next correction should first establish a restrained light/value separation at the existing geometry rather than adding ornament. Judge it in the real camera, with background contrast held comparable.

`blocking` for the broader AAA gate: Drowned corbel profiles read more as short rectangular tabs than carved masses in the native view. The source three-quarter inspection confirms curved geometry and seats, but its volume is not communicated sufficiently at the intended projection. A later art pass should test wider, smooth value gradients over the existing profile and only revise the silhouette if the independent contact gate remains satisfied.

`minor` at gameplay scale: endgrain rings and some pegs disappear at distant camera scale. They remain useful editable construction detail for close inspection; making them brighter or denser would add noise without resolving the major value/volume issue.

Rebuild with `scripts/blender_support_assets.py` in a fresh background Blender process; run the same script with `-- --verify` for source reopening and clean imports. Any remaining render job must use at most `floor(logicalCPUCount * 0.75)` threads via Blender's `-t` option, following the user's current resource limit. Additional polygons or successful checks are not an artistic rating.
