# Small props and rigid creature bodies

**State: review_candidate.** The bounded modeling task is delivered and technical checks pass. Target-renderer assembly, motion, and whole-game AAA acceptance remain separate review gates.

These original models follow the actual palette definitions in `src/render.c`, dimensions and silhouettes in `src/player.c`, `src/items.c`, and `src/life.c`, and the generated clay-pot and abandoned-lantern references. There are no copied film characters. The pot retains a genuine hollow mouth and thick rim; the lamp uses a seated fuel tank, opal chamber, cap, handle hinges and guards. Opaque opal glass is deliberate to satisfy the alpha-free conventional renderer requirement.

## Runtime contract

`src/generated/life_assets.h` includes the shared `FoundryAssetData` schema. Units and axes match the game: X-right, Y-up, Z-front. All models are local, not room-space.

| Symbol | Dimensions X × Y × Z | Pivot / placement | Triangles |
|---|---|---|---:|
| `FOUNDRY_POT` | 0.602575 × 0.721 × 0.600 | Ground center; replace old pot at `(px,py-.36,.28)` | 4,912 |
| `FOUNDRY_HUNTER_LANTERN` | 0.502 × 0.7585 × 0.36144 | Same center as old `Lamp`; Y bounds -.3125 to .446 include handle | 4,876 |
| `FOUNDRY_PLAYER_SEED` | 0.750 × 1.375 × 0.580 | Ground center at `(x,base-bob,0)`; exactly original 6×11-pixel main body | 1,680 |
| `FOUNDRY_BEAST_BODY` | 1.500 × 0.640 × 0.540 | Body center; positive X is head end | 956 |
| `FOUNDRY_BEAST_HEAD` | 0.530 × 0.525785 × 0.380 | Head center; faces +X, Y bounds -.18406 to .34172 include ears | 772 |
| `FOUNDRY_BIRD_BODY` | 0.615 × 0.462938 × 0.320 | Body center; faces +X, Y bounds -.18794 to .275 include head | 836 |

Total **14,032 triangles**, below the 18,000 target. Each object has two or three material meshes, 16-bit indices, and no texture dependencies. Detailed measured bounds are in `assets/blender/life-manifest.json`.

Keep the existing player's eyes, nubs, feet, rigid motion, blinking and water tint. The player source includes no eyes, nubs, feet, mouth or deformation rig. Keep beast legs, eyes and tail; remove its old primitive head/ear because the head asset contains the ears. The bird body contains its head; keep the existing beak, eyes and animated wings. Rotate around Y for facing changes rather than introducing a negative scale with reversed winding. No behavior, collider, interaction or response-table changes are part of this asset delivery.

The hunter lamp's only emissive material has strength .72 and the original warm glass palette. Runtime must retain existing flicker and set emission to zero for the abandoned dead lamp. Use existing green-white fixtures for city lamps; this warm hunter asset does not replace those. All creature and ceramic materials are non-emissive.

## Evidence and acceptance

- Editable source: `assets/blender/life-props.blend`, organized by asset collections.
- GLBs: `public/models/{ceramic-pot,hunter-lantern,player-seed,beast-body,beast-head,bird-body}.glb`.
- Generator / verifier: `scripts/blender_life_assets.py`, `scripts/blender_verify_life.py`.
- Technical report: `assets/blender/life-verification.json`.
- Actual source and clean-import renders: `assets/blender/review/*-beauty.png` and `*-runtime-reimport.png` for the six names above.

The saved source was reopened and each GLB imported into a clean scene. All counts and dimensions agree, all materials have alpha 1, and all six assets have **zero degenerate triangles and zero invalid normals**. Every review render was inspected. Lantern guard endpoints were moved into the tank and cap after the first image revealed ambiguous attachments.

The small creature shapes intentionally preserve the quiet, compact source silhouettes. The body-only studio renders are incomplete character presentations: facial marks and moving appendages are supplied by the existing renderer. These static checks do not establish assembled character animation quality; target captures and playback are required. Material gloss and color are also subject to the destination shader, particularly the warm lantern's restrained emission and the player's submerged tint.
