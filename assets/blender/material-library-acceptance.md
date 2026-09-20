# Reusable surface material checkpoint

This library provides editable procedural Blender materials and nine portable 1024-square RGB8 PNG maps for dressed city stone, weathered Vault basalt, and aged timber. It is a material integration checkpoint. The broader asset/game release remains `work_in_progress`; texture delivery does not establish AAA craftsmanship or immersion.

## Contract and provenance

The [editable source](material-library.blend) contains separate `SOURCE | ...` procedural materials and `PNG REIMPORT | ...` PBR materials. The source graphs use periodic coordinates, noise and sparse fracture fields; no reference-image pixels are copied into the maps. The [generator](../../scripts/blender_material_library.py) rebuilds the library in a fresh background Blender session. The source contains packed images and three inspection slabs. These slabs demonstrate surface response, not proposed replacement game geometry.

The [city cornice](../../public/art/references/drowned-quarter/20-carved-cornice.png) and [underwater material](../../public/art/references/drowned-quarter/27-underwater-material.png) govern the cool dressed stone, warm mineral inclusions and restrained surface wear. The [raw rock seam](../../public/art/references/vault-mouth/27-raw-rock-seam.png) governs the basalt family; its angular deep fracture, glowing ore and wet illumination remain separate geometry/lighting responsibilities. The [hewn timber](../../public/art/references/vault-mouth/11-hunters-rope.png) governs the desaturated brown grain and longitudinal fibers. These are artistic interpretations, not measured scans of those references.

Each texture represents a **2 m × 2 m** repeating surface, or 512 pixels/m at source resolution. Mapping this to **5 × 5 game units** assumes 0.4 m/game unit. The runtime may adjust this provisional conversion. Timber grain runs along **+U** and describes longitudinal faces; it is not an endgrain texture. Preserve the actual modeled endgrain rings and pegs, and suppress longitudinal mapping on their exposed ends.

The map contract is:

- `basecolor.png`: intrinsic color, sRGB. No directional lighting, ambient occlusion, cast shadows, joints, waterline or emissive mineral is baked in.
- `normal.png`: tangent-space OpenGL **+Y**, linear/Non-Color RGB. Decode `(RGB * 2 - 1)` and renormalize. The bake target is a triangulated planar 2 m UV square, +Z normal, +U/+V tangents. Bake swizzle is +X,+Y,+Z; margin is zero because the procedural surface itself is periodic.
- `roughness.png`: linear/Non-Color scalar, repeated in RGB. These are dry nonmetal surfaces; wetness changes belong to the environment shader. Metallic is zero.

Do not multiply these absolute albedos directly by the existing dark game palette. Use deliberate basecolor replacement or normalize the sampled color by its corresponding mean before restrained palette modulation. [The verification report](material-library-verification.json) records current means in both encoded sRGB and linear RGB, actual PNG IHDR metadata, source/map hashes, physical roughness bounds and vector statistics.

| Family | Mean sRGB RGB, 0–255 | Mean linear RGB | Roughness range |
| --- | --- | --- | --- |
| City stone | 106.55, 119.10, 124.05 | 0.14648, 0.18548, 0.20220 | 0.7608–0.8510 |
| Vault basalt | 60.24, 66.04, 68.95 | 0.04558, 0.05457, 0.05945 | 0.6980–0.8392 |
| Aged timber | 105.07, 83.79, 59.60 | 0.14171, 0.08836, 0.04468 | 0.7294–0.8314 |

The 1024 source textures occupy 27 MiB as uncompressed RGB8, or approximately 48 MiB if uploaded as RGBA8 with complete mip chains. A smaller runtime derivative is a separate delivery representation; preserve these sources. All Blender jobs use CPU Cycles with 16 threads on the 24-logical-CPU host. GPU rendering is disabled.

## Actual evidence and acceptance

The maker inspected the source and clean PNG reimport slab renders for all three families, the opposite-light renders, the 3×3 PBR repeat views, and the unlit 3×3 albedo views in [review/materials](review/materials). The same material detail survives reimport, and changing light direction changes surface shading while intrinsic grain/color remains attached. A rejected intermediate basalt version produced a connected crackle network; the current sparse fracture exposure removes that uniform dried-mud appearance.

| Gate | Status | Evidence / limits |
| --- | --- | --- |
| Editable procedural source | pass | Source reopens; 174 editable procedural nodes; portable PNG shaders retained separately. |
| Portable texture encoding | pass | All nine PNGs actually have 1024×1024 RGB8 IHDR values. sRGB is used only for albedo; normal/roughness are data. |
| Normal validity and clean PBR reimport | pass | Normal vector length errors remain below 0.007 after RGB8 quantization. Source/reimport response and opposing light were inspected. |
| Seam continuity | pass | Periodic value/derivative construction; actual wrap deltas comparable to neighboring pixels; no discontinuous border appears in the 3×3 views. |
| Repeat concealment / reference craftsmanship | fail | Seamless repetition can still be recognized in the large unlit repeat views. These surfaces remain cleaner and simpler than the governing references. |
| Game-scale triplanar mapping / mip response | unverified | Requires the actual renderer and its packed runtime derivatives. Studio views do not establish this gate. |
| Overall AAA / sensory acceptance | fail | This bounded material checkpoint does not prove the full game goal. |

Known artistic limitations are consequential: broad color variation can reveal a repeat over large uninterrupted walls; timber lacks the reference's torn fibers and split silhouette; basalt's deep angular cleavage cannot be supplied by a shallow normal map. Major fractures, block edges, bevel damage, actual joints and object-specific contact wear must remain modeled or authored in the appropriate object context. Increasing texture contrast to imply those forms would flatten the scene again.

An independent reviewer inspected the third candidate's source/reimport views, opposite-light city stone, unlit city repeats, and lit basalt/timber repeats against the governing close references. Verdict: **rework** for reference craftsmanship. The reviewer confirmed source/reimport correspondence, identified recognizable city macro-color repetition, described the basalt as a usable restrained micro-surface only when macro geometry supplies cleavage, and identified timber's clean, nearly parallel grain as less fibrous than the reference. The most direct future texture correction is lower city macro-color contrast; globally stronger normals would not address the construction gaps.

The subsequent [native Vault](../../docs/evidence/materials/candidate-native/with-occlusion/room-0.png) and [native Drowned](../../docs/evidence/materials/candidate-native/with-occlusion/room-1.png) material candidate views were also inspected. These captures used the pre-final basalt runtime package, so they are diagnostic rather than final texture-package acceptance. Relative to v12, the surface change is subtle. Repeated flat ashlar faces, regular black joints and uniform bevels still make the foreground walls read as a machined grid. The Vault's large rock forms remain rounded, while the reference has angular cleavage. The Drowned face still reads as a shallow frontal mask with an obvious rectangular mount. Dark narrow shelf fascias remain stronger visual marks than their modeled supporting volume. These findings require broad form and directional light work; the tile library cannot resolve them by itself.

The separate [final runtime package report](../../docs/evidence/materials/runtime-package.json) matches every delivered source PNG hash. Its 512-square derivatives retain +Y normals and preserve the original maps. Its packaging checks are independent of artistic acceptance and of the final game-camera review.
