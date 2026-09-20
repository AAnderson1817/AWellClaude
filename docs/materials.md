# Authored material surfaces — revision 13

The game now embeds Blender-authored city stone, Vault basalt and aged timber
surfaces. Each has a 1024-square RGB8 base color, tangent normal and roughness PNG
in `public/materials/`. The editable `assets/blender/material-library.blend`
contains procedural source materials and reusable shaders with packed PNGs.
These are authored interpretations, not measured scans. The source manifest and
acceptance record distinguish technical delivery from artistic work still open.

`tools/pack-material-library.py` preserves those source files and produces a
512-square runtime pair per material. It filters base color in linear light and
normal vectors as normalized vectors, then packs normal RGB and roughness alpha.
All six PNGs are embedded in `src/generated/material_textures.h`; the game does
not stream them from disk. `--check` validates exact bytes, source hashes and
measurements without rewriting files. Synthetic probes cover color conversion,
vector cancellation, quantization, channel packing and mean calculation.

The runtime PNG payload is 1,131,120 bytes. The six textures with full mip chains
need approximately 7 MiB for RGB8 plus RGBA8 storage, or 8 MiB when the driver
expands the color maps to RGBA8. Source provenance, hashes, dimensions and these
assumptions are in [the packing report](evidence/materials/runtime-package.json).

## Integration

The fixed-stage renderer uses world-space triplanar mapping at five game units
per tile, equivalent to a provisional two-meter material repeat. Both maps flip
vertically together on upload to preserve the Blender +Y tangent convention.
Signed tangent directions keep normal relief consistent on opposite faces;
normal perturbations are projected onto each geometric surface. Mipmaps,
trilinear filtering and repeat wrapping limit fine-detail shimmer.

The shader applies measured map variation relative to its encoded color mean,
preserving the authored object palette. This is the existing stylized lighting
pipeline, not a claim of calibrated physical lighting. Roughness and normal maps
give material-dependent light response. The geometry occlusion pass retains
unperturbed geometric normals so pores do not become contact shadows.

Per-mesh classifications distinguish dressed stone, basalt, timber, rear receivers
and metal. Bronze remains in its existing patina shader. Modeled plank ends, pegs
and incised endgrain use family 7 and retain their existing construction without
receiving the longitudinal timber map. The source supports retain their broad
wood tags; the exporter assigns this more specific runtime family by material
identity. No geometry or collision data changed in this material increment.

## Evidence and limits

The [native comparison manifest](evidence/materials/native/manifest.json) records
the exact renderer and headers. `with-materials/` and `without-materials/` contain
both rooms at the same frame with a compile-time surface-map control. Contact
shading is enabled in both; the control is not a player option. This verifies
that the authored maps contribute to the actual frame, separately from the
Blender review slabs and tiled inspections.

At 1920×1080 on this RTX 5080 host, the 600-frame native loop averaged
3.1674/2.5668 ms with maps and 3.0954/2.4301 ms without maps for Vault/Drowned,
after 120 warmup frames. These local wall times include the entire frame loop;
they are neither GPU-only timings nor general hardware performance guarantees.
[The rendered preservation comparison](evidence/materials/render-preservation.json)
passes 6,640 original-state frame comparisons across 15 cases in both views.

The material change is deliberately restrained at play scale. It does not repair
the regular ashlar grid, rounded rock masses, shallow hero carving or incomplete
inhabitants. The complete visual target and human immersion remain open; see
[the whole-brief review](artistic-review-v13.md).
