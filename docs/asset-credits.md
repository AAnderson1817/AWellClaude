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
