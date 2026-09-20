# Local surface illumination — revision 14

The depth renderer now samples the existing carried/set lamp, lit campfire,
suspended city lamp and reactive windows as nearby light sources. Up to four
sources add directional diffuse/specular response to the modeled surfaces. The
original occluded warm/cool grid remains the broad reflected-light field.

The local sources are presentation snapshots. Lamp position follows the existing
item; suspended light follows the current chain angle; fire requires its existing
persistent lit flag; window radiance follows its existing concealment/recovery
value. Snapshot reads neither advance state nor consume random numbers. The
source order fits the current room inventories: Vault has one portable lamp,
fire, suspended lamp and window; Drowned has the portable lamp and three windows.
Other emitters retain the original grid contribution.

The added shadow test samples the fixed two-dimensional opaque-tile mask along
the ray toward a source. It excludes the receiving/source cells to avoid shading
a front face with its own block. This preserves the existing solid-room envelope;
it is not a general three-dimensional shadow map and does not model shadows from
every prop or leaf. The four source loops skip inactive/out-of-range emitters.
Fine surface normals participate in shading, while contact occlusion still uses
geometric normals. The camera, simulation, collision and audio are unchanged.

Window glass also emits light visibly inside its recessed frame. Its brightness
uses the same response value, so approaching with the hunter's lamp still darkens
the window. No screen bloom or exposure animation accompanies the change.

Development native captures are in `assets/review/depth-v14/` for the start,
camp, floating lamp and darkened window. Final matched controls are recorded in
`docs/evidence/craft-v14/native/`, with the exact source and header hashes.
The same emissive panes appear in both controls; only local source illumination
is disabled in the comparison. The harness rests between measured frames to
reduce GPU demand. It measures local frame-loop wall time, not GPU-only time.
`AWELL_DEPTH_NO_LOCAL_LIGHTS` is a review-only build control, not a gameplay option.

Primary forms, material craftsmanship and human sensory acceptance remain open.
Improved lighting does not replace the face/stone construction work in progress.
