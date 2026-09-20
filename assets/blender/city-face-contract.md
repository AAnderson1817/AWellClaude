# Drowned Quarter: the carved witness

This is an original fixed-view interpretation of the colossal submerged face in
`claude/ROOMS.md`, using `03-drowned-face-v2.png` as the governing material and
construction reference. The source is an editable, volumetric stone relief, not
a background image. The figure belongs to the city's unnamed culture; no text or
real religious iconography is introduced.

The asset fills the authored back-wall region cols 21–28, rows 16–20. A broad
limestone carving, recessed eyes, a single downturned mouth cut, restrained eclipse seal, and old
bronze orbital repairs establish the form. Metallurgy appears as visibly seated
repair staples and cast strips; astronomy appears as the single orbit/eclipsed
disc; ritual meaning remains ambiguous. No part moves except runtime light.

Source axes are Blender Z up, front toward -Y. Delivery axes are X right, Y up,
Z front. One unit is one eight-pixel gameplay tile. Place the face at world
`(25,1,-1.8)` without scaling. The two separate opal eyes sit at local
`(-1.5,3.5,.165)` and `(1.5,3.5,.165)`. The renderer controls their intensity.
The eyes have a .213-unit radius. The model is behind the simulation plane and
has no collision or horizontal shelf affordance.

The conventional real-time delivery budget is 43,000 triangles for the face plus
one eye model, at most seven material groups, and no image textures. This is a
project-specific allowance for a unique room landmark, not an AAA criterion.
The v14 candidate contains 39,666 face triangles and 880 eye triangles, across
five face material groups and one eye group. The dense editable head evaluates
to 308,124 triangles; only the delivery representation is reduced to 25,000 head
triangles. This distinction preserves editable source construction while keeping
the runtime allowance explicit.
Models use opaque PBR material constants; the engine adds its common underwater
lighting and stone response. Planar UVs are provided but no UV-mapped image or
normal bake is required. Editable source and triangulated GLB/C header delivery
are distinct representations. The head is built from an editable skull scaffold,
explicit nasal and lip lofts, actual nostril and mouth subtractions, and a joined
mineral surface. Surface raycasts fit bronze plate grids to that final surface.
All parts are original authored source geometry.

Current local delivery bounds are `(-3.737350, .026767, -.260000)` through
`(3.737425, 4.972122, 1.111818)`. The occupied envelope remains inside the prior
asset's envelope. The eye model and response anchors are unchanged. The source
generator is `tools/art/build-city-face.py`, with sculpt construction in
`tools/art/city_face_sculpt.py`. Clean reimport and embedded float32 validation
are implemented in `tools/art/verify-city-face.py`.

Artistic acceptance requires a readable solemn carving at its room scale, quiet
surrounding stone, seated metal repairs, distinct green-white eye light, and a
clear separation from playable terrain. The source review and clean reimport
prove the modeled form and package; actual game rendering and independent
whole-scene review are still required before release. The v14 candidate is
**work in progress**: primary anatomy was accepted for the secondary stage, but
the current clay exposes skin-smooth stone, horizontal rolling transitions,
synthetic triangular edge cuts, and some proud lower-band ends. Technical checks
do not waive these artistic findings. See `city-face-artistic-review-v14.md`.
The intended target remains a finished stylized game environment, including the
user's AAA quality request.
