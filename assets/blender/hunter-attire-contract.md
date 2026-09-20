# Hunter attire asset contract

2026-09-20; bounded asset task, overall game remains work in progress.

Create the camp hunter's worn hat and pack around the existing rigid player seed. `claude/ROOMS.md` governs identity: same size and shape as the player, distinguished by a hat and pack; no weapon, luminous city decoration, text or added mechanic. The source is `assets/blender/life-props.blend` (the requested `life.blend` name does not exist); `FOUNDRY_PLAYER_SEED` is unchanged. Its ground-centered runtime dimensions are 0.75×1.375×0.58 units, matching 6×11 original pixels. At native 1080 height this body occupies roughly 36×66 pixels. Runtime uses X right, Y up, Z toward camera; Blender uses X right, Z up, -Y toward camera.

Governing references inspected directly: `public/art/references/vault-mouth/04-hunter-camp.png` for the soft broad-brimmed hat, hunched pack silhouette and restrained warm materials; `10-pack-dead-lamp.png` for a canvas/leather flap, continuous straps, worn edges and seated metal buckles; `08-hunter-cairn.png` for the quiet worn-cloth care gesture. These generated concepts establish material/construction character, not exact human anatomy for the seed-shaped inhabitant. The existing seed body governs fit and scale. A slightly left-offset rear pack is an explicit fixed-camera interpretation so it remains legible in the front view; no new body anatomy is supplied.

Conventional opaque realtime meshes, Blender 5.2 LTS, embedded `FoundryAssetData` arrays and one GLB; editable source with separate hat/pack source collections and excluded body/studio references. Separate `FOUNDRY_HUNTER_HAT` and `FOUNDRY_HUNTER_PACK` symbols share the seed's ground-center pivot. Provisional combined budget: at most 12,000 evaluated triangles and six unique materials. No image textures, texture memory, skeletal rig, collision or animation supplied. Rigid runtime posing belongs to the renderer; hands/stone contact and sitting extremes require later native review.

| Part | Role and attachment | Material |
|---|---|---|
| Shaped crown and brim | Hollow crown seated around the seed's upper head, soft uneven brim with actual thickness and a rolled hem | Matte weathered felt |
| Crown band | Continuous seated band; one small buckle, no repeating ornamental fasteners | Dark leather, rubbed bronze |
| Pack body | Compressed, asymmetric canvas volume against left/rear body, broad quiet panels and sparse tension folds | Waxed canvas |
| Flap | Thickened overlapping curved panel closing the pack; hem follows its edge | Canvas and worn binding |
| Shoulder straps | Two continuous broad straps routed around the body, seated at upper/lower pack anchors; front portions outside the eye region | Leather |
| Closure | One strap and seated frame buckle over the back flap, attached at the pack base | Leather, bronze |

Acceptance observations: hat/pack distinguish the person at 36×66 pixels; the face stays readable; the brim is cloth rather than a perfect disk; the pack reads as sewn fabric rather than a rounded box; straps touch appropriate seats and route outside the body without floating; undersides and rear remain coherent; only seams/closures survive as secondary details. Neutral front/side/back/three-quarter and interface/wire views precede export. Reopen source, reimport GLB into a clean scene, compare both, and measure finite positions/normals, degenerates, index bounds, dimensions, materials and hashes. Counts cannot establish AAA acceptance. Independent review and actual-camera integration remain separate gates.

Tooling: connected Blender scene was inspected read-only (default Cube/Light/Camera). Construction uses an isolated background Blender process to preserve that live scene and enforce the shared resource allowance: parent affinity 65535, Blender `-t 16`, one coordinated CPU render slot. No existing source asset, simulation or renderer is changed.
