# Vault scanned-rock patch: bounded acceptance

The rock above the Vault door now has irregular ledges, recessed breaks and broad calm faces at the actual game scale. This is a local primary-form improvement over the framed analytic planes. The whole environment and AAA brief remain **rework**. The upper shadow is still rather continuous, adjacent rock remains visibly procedural, and this change does not resolve the face, cairn, wider inhabitants or immersion gaps recorded in the v15 review.

The review compared the existing [raw-rock reference](../public/art/references/vault-mouth/27-raw-rock-seam.png), neutral source/reimport views, and the same native room/frame with unchanged lighting and materials. [The matched crop](evidence/terrain-v16/native/comparison-2x.png) shows the local change. [The final production frame](evidence/terrain-v16/native/production/room-0.png) is byte-identical to the accepted isolated candidate. The full-frame difference from control is confined to pixel bounds `[0,268,279,470]` at 1920×1080, including local occlusion.

## Construction and provenance

The source is [Rock Face 01 by Dario Barresi / Poly Haven](https://polyhaven.com/a/rock_face_01), CC0. Original files and portable dependencies are in [the vendor folder](../assets/vendor/polyhaven/rock-face-01/README.md), with hashes and URLs. The original source has 20,174 triangles. This implementation is an adaptation of third-party geometric data, not an original scan or a texture-based substitute for volume.

The fit occupies only world `x=0..5, y=13..16`. It uses a rigid dominant-plane alignment, uniform scale 1, an interior crop, and no LOD reduction. The retained front has 8,210 triangles after removing two zero-area clipping remnants. In the upper 0.85-tile region, the scan is seated under the immutable contact arris: depth changes start at `y=15.15`, finish by `15.87`, and the final 0.13 tile is the protected landing transition. The maximum local depth shift is 0.9745 tile; mean shift among affected vertices is 0.3725. The lower 2.15 tiles retain the rigidly transformed scan geometry.

Concave side/bottom polygons follow the actual cut boundary. Original production top and rear triangles remain in place. This is an assembly of terrain surfaces, not a new collider or a claim of one welded manifold. The vendor's imported normal layer is replaced after fitting with normals derived from the actual triangle planes. The open face is explicitly oriented toward the camera; an early reversed-winding candidate was rejected after native back-face culling exposed it.

[The existing terrain builder](../scripts/blender_terrain_assets.py) calls [the bounded scan module](../scripts/blender_scanned_rock.py). The editable [terrain source](../assets/blender/terrain.blend) retains the untouched scan and aligned undeformed crop in a hidden source collection. The portable [Vault GLB](../public/models/vault-terrain.glb) is exported from the same final arrays used by the game. Equal position/normal corners are shared in the GLB so acute cap triangles retain valid geometric normals on reimport.

## Preserved behavior and validation

The independent audit confirms that all 24,422 nonpatch Vault triangles retain positions, normals and material slots. The Drowned header block, all Drowned arrays/metadata, and its GLB are byte-identical to v15. Protected upper contact and rear faces are unchanged. The map, renderer, shaders, player, items, one-way shelves and gameplay simulation were not edited in this task.

- [Independent preservation report](evidence/terrain-v16/independent-preservation.json): patch replacement is 699 old triangles to 8,627 new triangles, with no changes outside the authorized domain.
- [Independent architecture report](evidence/terrain-v16/independent-architecture.json): all four terrain/support assets pass; 1,300 Vault landing probes and 56,320 Vault mask samples have zero failures. Normal, degeneracy, front-leak and material contracts pass.
- [Source/GLB verification](../assets/blender/terrain-verification.json): the saved source reopens; both complete-room GLBs reimport with zero degenerate triangles or invalid normals, and zero mismatches across 7,920 mask probes per room.
- [Matched source/reimport render manifest](../assets/blender/review/terrain-v16/matched-render-manifest.json): the final saved source and production GLB have been visually inspected from the front and at grazing angle.
- [Exact native-array equivalence](evidence/terrain-v16/native-array-equivalence.json): the earlier accepted candidate and final production header contain exactly equal parsed arrays and materials. A fresh executable built from current production sources produced the same PNG, confirming culling and the actual final view.

The Vault now has 33,049 terrain triangles, up 7,928; both rooms total 102,393. The temporary aggregate ceiling is 110,000, with 10 Vault and 9 Drowned material meshes and 16-bit indices retained. These numbers describe the delivery; they do not certify AAA quality or runtime performance. Native and web builds pass. The bounded browser observations are recorded separately in [web-smoke.json](evidence/terrain-v16/web-smoke.json). No throughput benchmark was performed for this patch.

## Reproduction

Run Blender 5.2 from the repository root with `--background --factory-startup --disable-autoexec -t 8 --python-exit-code 1 --python scripts/blender_terrain_assets.py -- --skip-render`. This regenerates the original architecture before applying the bounded patch. Omit `--skip-render` for the full-room source views.

Reopen `assets/blender/terrain.blend` with `--disable-autoexec -t 8 --python scripts/blender_verify_terrain.py -- --skip-render` for full-room import and mask checks. Run `scripts/blender_review_scanned_patch.py` against the same saved source for the matched patch views. Run `tools/check-architecture.py --self-test --require-integrated` for the independent runtime-array contract.

All heavy shells use affinity mask `65535`; Blender rendering is CPU-only with eight threads. The final native frame uses the same 60 fps cap and 4 ms inter-frame rest as its control, with no timing claim. The terrain change does not edit shaders or gameplay.
