# Existing-room terrain delivery

**State: review_candidate.** This is the terrain asset portion of the user's existing-game 3D presentation request. It does not certify AAA quality for the whole game.

The generated source reads the two `MAPS` arrays from `src/room.c` without modifying them. Only solid `#` and `*` cells are included. The first room uses the same city rectangles as `ZoneAt`; the Drowned Quarter is dressed as city throughout, following the active 3D renderer's `IsCity` convention. This is presentation classification and changes neither collision nor light logic.

The Vault Mouth has continuous raw slate masses, irregular stretched cleavage plates, recessed seams and restrained mineral variations. The city has staggered long ashlar courses, recessed mortar, continuous standing caps, quoin returns and the existing central pier dressed as a continuous fluted shaft. All added material families are non-emissive.

## Runtime interface

`src/generated/terrain_assets.h` includes `foundry_assets.h` and uses the existing `FoundryAssetData` / `FoundryMeshData` definitions. It exports:

- `FOUNDRY_VAULT_TERRAIN`: 17,076 triangles, nine material meshes.
- `FOUNDRY_DROWNED_TERRAIN`: 23,592 triangles, five material meshes.

Total: **40,668 triangles**, below the task's 60,000-triangle budget. Material meshes use 16-bit indices and contain positions, normals, sRGB color, roughness, metallic and zero emission. There are no image textures or runtime file dependencies.

Both assets are authored in absolute world coordinates. Draw at `(0,0,0)` with unit scale and no centering transform. X spans 0–40, Y spans 0–22, front Z is at or behind zero. Vault depth reaches -2.8; city depth reaches -2.55. Editable Blender source uses Z-up and -Y-front, converted on export to the runtime axes.

The runtime renderer must replace its old solid-terrain boxes and raw-rock mass with these meshes. Keep the existing one-way shelves, grates, seam emitters, plants, moss, water, actors and props; they are intentionally excluded here. The original tile collision remains authoritative.

## Source and verification

- Editable source: `assets/blender/terrain.blend`.
- Alternate interchange files: `public/models/vault-terrain.glb`, `public/models/drowned-terrain.glb`.
- Rebuild: isolated Blender 5.2 background process running `scripts/blender_terrain_assets.py`.
- Verify: open `terrain.blend` in a fresh background process and run `scripts/blender_verify_terrain.py`.
- Reports: `assets/blender/terrain-manifest.json`, `terrain-verification.json`.
- Actual source and clean-import renders: `assets/blender/review/*terrain*.png`.

Verification reopens the editable source, imports each GLB into an empty scene, checks normals and zero-area triangles, verifies export counts, and samples each room's projected silhouette at nine points in all 880 cells. Each room passed **7,920 samples with zero solid-mask mismatches**, zero invalid normals and zero degenerate triangles. A separate generator check found no vertices in empty collision footprints and no vertices protruding into positive Z. The verification script also rejects stale assets if the source MAPS change.

## Visual review

The actual initial render revealed overly regular horizontal raw-rock stripes. This was revised to broken, stretched fracture cells. An intermediate flat-shaded version exposed the sampling lattice, so shared vertices and smooth internal normals were restored while preserving fractured shape and exact perimeter coordinates. The final source and clean-import images were inspected.

The city is intentionally regular, with course sizes and joints giving larger architectural masses than one cube per collision tile. The column's separate continuous flutes distinguish its structural role. The raw-rock treatment remains a restrained stylized interpretation; final target-shader review is necessary to judge its roughness and contrast alongside actors and environmental art. The clean neutral renders alone cannot establish that integration gate or the overall AAA target.
