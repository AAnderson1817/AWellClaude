# Terrain architecture revision 12

This is a presentation revision within the existing Animal Well project and its
two immutable 40 by 22 tile rooms. It provides modeled Drowned Quarter house
facades and engaged stone columns for both rooms. It does not add rooms, gates,
collision, or changes to player motion. The original `src/room.c` mask remains
the authority for every exterior boundary and landing lip.

The governing references are Drowned Quarter `14-drowned-doorway.png`,
`20-carved-cornice.png`, and `13-sunken-column.png`, plus Vault Mouth
`16-city-column.png`, under `public/art/references`. Their construction cues are
deep inhabited architecture, worked pale stone, substantial supports, bronze
repairs, and restrained orbital ornament. The fictional culture remains unnamed;
the repeated pointed arcade can suggest devotional architecture without text or
a real religious symbol. Astronomy is carried by physical orbital grille joints
and inclined shaft inlays; metallurgy is visible in seated bronze bindings and
repair straps. These motifs do not emit light or claim gameplay rewards.

The two door cavities occupy world X/Y rectangles `(6.29,7.73,10.16,12.76)` and
`(31.27,32.71,10.16,12.76)`, corresponding to the authored columns 6–7/31–32,
rows 9–11. The four lower slot rectangles are `(4.18,4.78,7.14,8.82)`,
`(6.33,6.93,7.14,8.82)`, `(31.38,31.98,7.14,8.82)`, and
`(33.62,34.22,7.14,8.82)`. Front mass and individual ashlar are split around
these wells. Their curved returns and closed rear walls are real mesh; the door
rear is Z=-1.22 and the slot rear Z=-0.92. They are sealed facade dressing. The
full solid collision mask continues through them. Fixed slot grilles and quiet
sills distinguish the facade from an exit or a playable shelf.

Column axes are X=19.5, Z=-1.50. Vault base/top are Y=2/9; Drowned base/top are
Y=1/10. Actual turned shaft courses, flared capitals, stone base profiles, and
bronze collars sit inside the original X=18..21 pier footprint. Abacus and
footing preserve the original exact standing/contact edge at Z=0. Other
decorative geometry remains at or behind Z=0. No collision file is generated.

Blender authors with Z up and front toward -Y. GLB and C delivery use X right,
Y up, Z front. One unit is one eight-pixel tile. Both assets are drawn at origin
with scale 1; their full bounds are `(0,0,-2.96)` through `(40,22,0)`. Names
remain `FOUNDRY_VAULT_TERRAIN` and `FOUNDRY_DROWNED_TERRAIN`, preserving the
renderer interface. Runtime capacity is 12 material meshes per room. The
combined terrain allowance is 100,000 triangles; this is a project constraint,
not evidence of artistic quality.

Materials are opaque PBR constants. The game supplies its shared lighting and
world-space surface response. There are no external images, baked textures,
texture UV requirements, or external dependencies. The source retains named
pieces and editable bevel/normal modifiers. Delivery copies quantize coordinates
to six decimals, weld and dissolve subpixel bevel slivers, remove microscopic
degenerate faces, and repair invalid evaluated corner normals before both GLB
and C export. Clean GLB import and direct generated-C validation are separate
checks because float precision and normals must survive the whole pipeline.

Required evidence is reopened editable source, neutral full-room/column/facade
renders, clean imported GLBs, complete vertex/normal/triangle checks, exact
projected mask and contact checks, then actual fixed-camera runtime captures.
The visual target remains the user's AAA request. This revision may only be
described as work in progress until that actual scene is convincingly crafted;
technical success and added geometric complexity do not satisfy that target.
