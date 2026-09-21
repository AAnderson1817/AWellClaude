# Asset credits

The scanned city and carved-face stone uses **Rock Surface**, created by
**Amal Kumar** and distributed by [Poly Haven](https://polyhaven.com/a/rock_surface)
under [CC0](https://polyhaven.com/license). These source maps are public domain
assets available free from their original publisher. They are not original
photography by this project. Powered by Poly Haven.

Four unmodified 1024-square, 16-bit source PNGs, their download URLs and checksums
are preserved in `public/materials/vendor/rock-surface/provenance.json`. The
material represents a two-meter surface. The game maps that to five world units,
using its provisional 0.4-meter art scale. Albedo has no added lighting; the normal
uses OpenGL +Y and roughness is linear data. The displacement source is preserved
for material authoring and is not runtime geometry.

`tools/import-scanned-stone.py` performs the documented eight-bit conversion and
normal-vector normalization. `tools/pack-material-library.py` filters and embeds
the resulting maps. The earlier procedural city-stone images and
`assets/blender/material-library.blend` remain available as their original source
and a historical comparison. The new scan does not change ownership or attribution
of the project's modeled geometry or its generated art-direction references.

The bounded rock formation above the Vault door uses **Rock Face 01**, scanned by
**Dario Barresi** and distributed by [Poly Haven](https://polyhaven.com/a/rock_face_01)
under CC0. The fitted geometry retains the existing basalt material in the game.
The original Blender source, portable texture dependencies, URLs and hashes are
preserved in [the vendor package](../assets/vendor/polyhaven/rock-face-01/README.md).
[The adaptation record](scanned-rock-v16-review.md) documents the crop, local depth
adjustment beneath the protected landing, and unchanged neighboring geometry.

The in-progress carved-face anatomy study uses **Head - Planar** by **Paul
Kotelevets**, from Blender's **Human Base Meshes 1.4.1**, under CC0. The selected
editable source, embedded author/license metadata, upstream hashes and exact
geometry comparison are preserved in [the Blender vendor package](../assets/vendor/blender/head-planar-1.4.1/README.md).
This anatomical source is not yet part of the production face. The adapted carving
must pass its separate artistic and runtime checks before promotion.
