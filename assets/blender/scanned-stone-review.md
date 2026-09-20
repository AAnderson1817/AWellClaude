# Scanned stone: editable material source review

2026-09-20. **Technical delivery passes; source-library task complete.** This is a material setup and diagnostic studio, not a new game mesh or approval of the game's visual finish. The author of this setup also performed this self-review. The runtime conversion and game-camera integration were performed separately.

The material uses [Rock Surface by Amal Kumar](https://polyhaven.com/a/rock_surface), published by Poly Haven under [CC0](https://polyhaven.com/license), with a stated physical width of 2 m. The four original 1024², 16-bit PNGs in `public/materials/vendor/rock-surface/` are packed byte-for-byte into [scanned-stone.blend](scanned-stone.blend). The [verification](scanned-stone-verification.json) records the vendor hashes, dimensions, bit depth, color spaces and shader links. The older procedural `material-library.blend` remains unchanged, SHA256 `f212cf541e7504bd5d05cb1992cffb532799bdc278733a85ee333d08db6b3c6c`.

## Editable setup

The asset-marked material, labelled nodes, packed images and in-file README remain editable. Diffuse is sRGB; OpenGL +Y normal, roughness and displacement are Non-Color. The normal graph explicitly decodes `RGB * 2 - 1`, normalizes the vector, re-encodes it and feeds Blender's tangent Normal Map node at strength 1. It does not flip green. The source normals are not all unit length: decoded lengths range from 0.499803 to 0.999643. The normalized maximum length error is below 1.2e-7. Roughness ranges from 0.600381 to 0.955444. Metallic is zero; IOR 1.48 is an editable material assumption, not a measured property of the scan.

No maps were repainted, rebaked or combined with AO. The optional displacement image is preserved and connected to a labelled node, but its scale is zero and its output is disconnected. Height amplitude is uncalibrated; enabling it without calibration could exaggerate or double-count relief already represented in the normal map. The build script is [blender_scanned_stone.py](../../scripts/blender_scanned_stone.py).

## Inspected evidence

I inspected all eight source renders: material from opposite lights, normal-only clay from opposite lights, geometry-only clay, an oblique material view, and both light directions on the 3×3 repeat. I also inspected the reopened material, normal-only and repeat views. All eight independently reopened images are pixel-identical to their source counterparts; [comparison results](scanned-stone-reopen-comparison.json) record zero channel difference. Source SHA256: `2c84ebd8a20c94d80b29a8875bd5610139332b451de03ce46bf47dea2a01cf4f`.

- [Material slab and sphere](review/scanned-stone/reopened-material-left.png) show subdued mineral grain and irregular shallow relief. Highlights remain broad and restrained; the result does not have the uniform crackle of the replaced procedural city-stone trial.
- [Opposite-light normal clay](review/scanned-stone/reopened-normal-clay-right.png) changes the relief shading consistently with the light direction. [Geometry clay](review/scanned-stone/source-geometry-clay.png) remains smooth, separating the mapped response from the diagnostic mesh.
- [Three-by-three repeat](review/scanned-stone/reopened-repeat-3x3-left.png) shows no conspicuous hard tile boundary. A repeated diagonal mineral pattern remains discernible when searched for; continuous edges do not eliminate visible repetition. Boundary pixel differences are comparable to interior adjacent differences in both axes, a useful diagnostic rather than proof that every mapping will conceal tiling.

The slab is 2 m wide and uses one UV square; the 6 m repeat plane uses three. The sphere is a curved diagnostic with equatorial tile scale matched to 2 m. Its latitude compression, poles and rear UV seam are expected limitations of that diagnostic mapping, not evidence that the material is seamless on arbitrary meshes.

## Acceptance limits

This package preserves and exposes the adopted scan correctly and survives a clean Blender reopen. The studio renders do not establish actual-camera scale, palette modulation, temporal stability, web performance, artistic cohesion of the entire environment, or AAA acceptance. Those require the separate game frames and whole-brief review. No production geometry, collision, gameplay, runtime texture package or original material library was changed by this source task.
