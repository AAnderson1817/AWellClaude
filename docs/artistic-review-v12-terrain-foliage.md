# Revision 12: terrain self-review and independent foliage review

Current verdict: **rework toward the requested visual standard**. Both asset
families contain actual editable 3D geometry and survive delivery; that is an
improvement over flat presentation. Neither technical checks nor this review
establish AAA quality or player immersion.

The reviewer authored the terrain revision and the earlier carved face, so the
terrain assessment below is a disclosed self-review. The foliage assessment is
independent: the reviewer did not author its Blender source, renderer, or models.
This is a scoped asset review, not release acceptance for the entire game brief.

## Terrain: self-review

The governing images inspected were Drowned `14-drowned-doorway`,
`20-carved-cornice`, `13-sunken-column`, and Vault `16-city-column` under
`public/art/references`. Current source and clean imported GLB renders are in
`assets/blender/review/architecture-v12/`: full rooms, complete columns, and a
close Drowned facade. The source was reopened for the GLB verification.

The two actual wells, four fixed barred slots, worked arch blocks, seated jambs,
cornice supports, and limited bronze repairs give the house facades meaningful
construction. Front mass and ashlar genuinely stop at the openings. Their rear
walls remain closed, so the old solid mask is unchanged. Column bodies now have
volume, course joints, base profiles, and a flared capital; the orbital ribs sit
on the curved form. Source and reimport show the same shapes. Coplanar cap faces
and tiny evaluated bevel slivers found during this pass were corrected.

The main artistic limitation remains broad surface treatment. Most ashlar is
regular, shallow, and clean, and its repeated soft edges still read like a
stylized kit. Sparse chipped courses improve the facades but do not produce the
reference's layered erosion. The shaft's broad shading is smoother and more
uniform than carved stone, and the arcade ribs read as fine applied ornament
rather than deep carving. The neutral source render consequently resembles a
clean model more than an old drowned building. This cannot be solved by more
small bronze lines. Further refinement should prioritize the large cut planes,
material response, and supported shadow transitions visible from the game
camera, with less attention to details that disappear at room scale.

The exact contact lip must remain visually legible. Door and window recesses are
quiet non-interactive dressing, and the complete original solid footprint stays
sealed behind them. The available runtime occlusion capture shows deep dark
openings, clear slot grilles, and a dimensional capital. It also shows that the
large repeated brick fields and smooth shaft remain visible as quality gaps.
Final integrated captures with the narrowed supports were not yet available at
the time of this asset assessment and must be reviewed separately.

Technical delivery: Vault 27,682 triangles in 11 material groups; Drowned 70,634
triangles in 9. `assets/blender/terrain-verification.json` records zero invalid
vertex/corner normals, zero degenerates, and zero mismatches in 7,920 GLB mask
samples per room. `tools/check-architecture.py --terrain-only` separately passes
the generated C arrays' bounds, normals, material attributes, projected masks,
and contact probes. The original room masks and collision source were not
modified. These results establish compatibility, not aesthetic acceptance.

## Foliage: independent review

Evidence inspected directly: `vault-mouth/25-vault-bush.png`,
`drowned-quarter/11-submerged-fronds.png`, both assets' front, side, and
three-quarter source renders and clean reimport renders under
`assets/blender/review/foliage/`, plus actual game frames
`assets/review/depth-v12/occlusion-vault/f0005.png` and
`assets/review/depth-v12/occlusion-drowned/f0120.png`. Findings apply to those
versions; later improvements require a fresh visual review.

1. **Vault blade shape and material are too uniform.** Broad cupped ovals,
   thick raised vein wires, and softly inflated surfaces give the clump a waxy
   or rubber-like appearance. The governing reference has thinner blades,
   uneven curl, edge wear, and a more varied leaf hierarchy. Stylization need
   not copy photographic detail, but it needs that larger-scale variety.
   Reduce puffing and vein thickness; vary narrow, broad, curled, and damaged
   profiles with restrained color/roughness differences.
2. **The attachment looks like a bundle placed on the ledge.** Several roots
   end as similarly sized blunt cylinders. In the source they resemble cut
   sticks rather than branching roots. The lower-left Vault plants visibly
   sit above their supporting edge in the game view. Taper and branch the
   attachment, bury its ends, and make its contact follow the supporting
   stone. Do not hide the landing contour with the repair.
3. **Drowned growth needs a stronger height and width hierarchy.** The short
   cluster of similarly shaped S-ribbons and hook-like folded tips is less
   convincing than the reference's long tapering lead fronds, secondary low
   shoots, and occasional irregular twists. Unequal frond widths, several
   low shoots, and fewer identical upper hooks would create a more organic
   clump without adding visual clutter.
4. **The underwater material largely collapses in the actual lighting.**
   Most clumps read almost black, so their modeled overlapping leaves lose
   separation. The clump to the right of the face instead becomes bright
   mint under the eye spill. A small amount of muted warm-green edge or
   material variation could preserve form across both lighting conditions;
   increasing global emission would undermine the restrained atmosphere.
5. **Repeated placement exposes the template.** Similar compact shrub and
   frond silhouettes recur along the same horizontal surfaces. A few authored
   variations in dominant lean, scale, spread, and gaps between clusters would
   reduce the regularity. The broad clear traversal paths should remain.

The foliage's modeled leaves and changing orientations are visibly better than
flat icons. Its scale generally stays subordinate to the player and major
landmarks, and it does not fill the rooms with effects. The missing ingredient
is convincing growth and surface hierarchy, not additional polygons. Motion,
sound, and immersion cannot be judged from these still captures.
