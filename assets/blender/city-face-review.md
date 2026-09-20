# Carved witness: acceptance record

**Overall asset state: work_in_progress.** The integrated model is a substantial
new room landmark. The user's AAA target is not established by this package.

The governing concept is `public/art/references/drowned-quarter/03-drowned-face-v2.png`.
The current editable candidate is `city-face.blend`; matching GLBs, embedded C
arrays, measured dimensions and hashes are in `city-face-manifest.json` and
`city-face-verification.json`. Review images are under `review/city-face/`.

| Gate | State | Evidence and judgment |
| --- | --- | --- |
| Primary identity and silhouette | pass | Front, three-quarter and clay views show one large fixed carved face, inset paired eyes, a closed mouth and an architectural stone mass. A broken rim replaces the first overly clean round plaque. |
| Construction | pass | Side clay confirms real slab thickness and a projecting stone nose. Bronze orbital bands and repair staples are seated on the underlying carving. Separate opal lenses sit within the wells. The back is a flat masonry seat. |
| Geometry and package | pass | Blender 5.2 source reopened; both GLBs reimported into clean scenes. Face: 38,594 triangles, 20,600 export vertices, five materials. Eye: 880 triangles, 442 vertices, one material. Zero degenerate triangles, invalid normals or nonfinite values. Dimensions match. Each export contains exactly its one asset mesh. |
| Material family | pass | Opaque pale stone, weathered bronze, verdigris and green-white opal follow the city's semantic light rule. No amber emission or additional bright focal detail. |
| AAA material/craft quality | fail | The source material remains too uniform compared with the governing reference's mineral density and localized wear. The mouth's broad smooth bands have less convincing carved transitions than the reference. Chiseled nose planes, rim chips and repaired bronze improve the asset but do not close that gap. |
| Source/export visual agreement | pass | Actual source and clean-reimport three-quarter images were inspected. Dimensions, facial planes, repairs and opal eye geometry agree. The final flat-back repair was checked in a new clay render. |
| Current destination appearance and response | unverified | An earlier version was inspected in `assets/review/depth-v11/drowned/f0120.png`: it reads as a quiet submerged landmark, roughly 220 by 160 pixels, and stays behind the physical terrain. The final source revision still requires a refreshed game capture and response review. |
| Independent full-scene artistic acceptance | unverified | Root independently rejected the initial smooth porcelain appearance. Nose, silhouette, lower orbital layering and real erosion were revised in response. Full-scene acceptance of the updated runtime is still pending. |

The original plate-like silhouette, per-face stepped colour patches, weak rounded
nose, intersecting lower orbital tips and back-normal smearing were corrected.
Current lower castings have separated tips and are narrower; this avoids colour
fighting at their terminations while preserving the reference's layered orbital
family. The model adds no movement, collision or gameplay verbs.

Local dimensions are 7.477064 by 4.95 by 1.396233 units. At the documented anchor
`(25,1,-1.8)`, world bounds are approximately
`(21.261468,1.025,-2.06)` to `(28.738532,5.975,-.663767)`.
Eye centers are `(23.5,4.5,-1.635)` and `(26.5,4.5,-1.635)`.
The mouth emission origin is local `(0,1.37,.145036)`.
The whole carving remains behind the simulation plane.

Deliberate construction exceptions: the nose is a deeply seated stone insert;
short nostril shadow insets are open decals on that solid form; the orbital wells
are thin closed inset skins; the stone shell, nose and metalwork overlap at their
seats. These are static independent assemblies, not a deformation mesh. Planar
UVs exist for inspection, but no texture bake or UV-packed image dependency is
claimed. No source lighting, studio camera or staging eye instance is exported.

Build the source with `tools/art/build-city-face.py`; reopen and reimport it with
`tools/art/verify-city-face.py`. Run each through isolated Blender background
processes. Final native/WebGL compilation and whole-room validation belong to
the application integration checks, which must use this header version.
