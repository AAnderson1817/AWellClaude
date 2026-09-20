# Mural asset review

The asset replaces the temporary black rectangle and cylindrical stick figures with an original procession painted over a modeled basalt return. It preserves the existing mural's light-response ownership. Its origin is `(9, 2.3, -1.3)` in runtime coordinates; the source uses Blender Z up and faces -Y. No collision geometry, verb, or room-layout change is included.

The source is [mural.blend](mural.blend). Rebuild using `scripts/blender_mural_assets.py` in a fresh background Blender process; run the same script with `-- --verify` to reopen the source and import both GLBs into a clean scene. The generator writes `src/generated/mural_assets.h` using the existing `FoundryAssetData` layout.

| Asset | Runtime symbol | Triangles | Material groups |
| --- | --- | ---: | ---: |
| Opaque basalt | `FOUNDRY_MURAL_ROCK` | 10,720 | 4 |
| Phosphor pigment | `FOUNDRY_MURAL_PIGMENT` | 12,544 | 4 |

The rock has actual front relief, sparse fissures, irregular side/crown silhouettes, a closed back, and a buried foot. The complete painted layer is separate so runtime visibility can fade its alpha together. The pigment intentionally uses open surface-following geometry. It has no metalness, does not cast a decal shadow, and does not need image textures or UV sampling. Exported GLBs carry geometry and material values; Blender ray-visibility settings are not a portable glTF feature. The game has no shadow pass for this geometry.

The six bearers share swept hoods, long robes, circular shoulder marks, connected gestures, and a dark disc. Sparse orbital marks use that same visual vocabulary. The [phosphor reference](../../public/art/references/vault-mouth/13-phosphor-mural.png) and [lamplit reference](../../public/art/references/vault-mouth/14-mural-lamplit.png) guide the motif and material intent. Their full rendered stone texture is not reproduced by this geometry-only asset.

## Inspected evidence

The maker inspected the final [source front](review/mural/source-front.png), [source three-quarter](review/mural/source-three-quarter.png), [unpainted rock](review/mural/source-rock-only.png), [clean reimport front](review/mural/reimport-front.png), and [clean reimport three-quarter](review/mural/reimport-three-quarter.png). Source and reimport preserve the same procession, fissure breaks, crown, and paint placement. Earlier iterations with thin-stroke triangulation failures and pigment intersections were replaced; the final strips use segment quads.

The verifier [mural-verification.json](mural-verification.json) proves the saved source reopens, GLB counts/dimensions agree with the embedded asset, and both reimports have zero degenerate triangles and zero invalid vertex normals. Total delivery geometry is 23,264 triangles, below this asset's provisional 28,000-triangle budget. That count is a delivery limit, not an artistic rating.

| Gate | Status | Evidence / remaining work |
| --- | --- | --- |
| Editable source and clean GLB reimport | pass | Source reopened; final source/reimport images inspected; dimensions/counts/normals verified. |
| Local form replacement | pass | Broken crown, buried foot, relief-bearing rock return, and broad painted figures replace the rectangle and stick limbs. |
| Reference craftsmanship at close range | fail | Stone and pigment remain cleaner/simpler than the reference's weathered surface. Some fissure breaks are visibly angular in enlarged review images. |
| Intended gameplay-scale readability and floor integration | unverified | Root renderer integration and an actual room render are required. These studio images do not prove contact with the existing camp floor. |
| Light-response fade and WebGL display | unverified | Must be observed in the real game with the lamp/fire response; no behavior code was changed by the asset task. |
| Whole requested AAA / immersion result | fail / work_in_progress | This asset is one local improvement; broader game construction, sensory review, and human acceptance remain open. |

This is a verified editable integration candidate, not an AAA release approval. The intended runtime view is approximately 195 by 120 pixels at 1280 by 720; inspect that scale and the whole room before accepting its brightness or deciding whether more texture detail improves the result.
