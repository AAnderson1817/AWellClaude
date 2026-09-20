# Hunter presentation review

2026-09-20. Bounded inhabitant integration; this does not establish AAA visual acceptance or human discovery/listening acceptance.

## Rendering contract

`src/depth.c` and `src/render.c` consume a detached `HunterView`. Neither renderer advances the inhabitant or changes an item. The 3D body is the original `FOUNDRY_PLAYER_SEED` at uniform scale one, with the verified hat and pack sharing its ground pivot and rigid rotation. No body squash, body scaling, vertical hover, new light source, weapon or UI is added. Mouth opening uses the audio owner's envelope snapshot. The flat body retains the same 6×11 pixel dimensions and hat/pack identity.

The depth renderer suppresses the old decorative cairn when the real inhabitant exists. The actual item array supplies every visible cairn and held stone. A tending stone's existing center receives its snapshot rotation; the flat hook replaces that same item's normal draw, using its original five-by-four silhouette and palette. There is no extra held-stone proxy.

## Initial actual-camera evidence

Initial captures and source/executable hashes remain in `docs/evidence/hunter/native/manifest.json`.

- `vault/f0120.png`, `return/f0010.png` and `return/f0104.png`: hat and pack distinguish the hunter at the actual camera. Body scale relates clearly to the adjacent player. No obvious floating hat, detached pack, clothing/body intersection or baseline hover was visible in these samples. The dark pack intentionally remains quieter than the face and hat.
- `return/f0050.png` and `return/f0076.png`: the real removed stone and replacement are visible, with no duplicate decorative cairn. Small gaze and mouth cues remain subtle at full-room scale.
- `fire/f0008.png`, `fire/f0126.png`, `fire/f0160.png` and `fire/f0260.png`: the hunter visibly relocates and responds to the fire's existing light. At the warm seat the regular tall cairn obscures part of the left body/pack. The identical smooth oval stones read as a stack of eggs more than irregular balanced stones. This remains a craft issue; presentation work does not silently resize or reposition those items.
- `tend/f2228.png`, `tend/f2276.png` and `tend/f2324.png`: lift/turn/replace is visible, but the initial interpolated rods lengthen dramatically to reach the raised stone. This pose was rejected; it is not accepted evidence of compliant rigid articulation.
- `flat-return/f0050.png` and `flat-return/f0104.png`: rigid body and hat silhouette carry across, while dark local lighting and the original small pixel target obscure subtle palm and gaze changes. Increasing character emission merely for attention would conflict with the quiet scene and was not done.

## Fixed-length correction

The shared pure `src/hunter_pose.h` now supplies both renderers with rigid shoulders, rounded-stone grip contacts and a two-link elbow solve. Each link is exactly 4.2 original pixels; shoulders attach at 68% of body height. Elbow bend uses a continuous down/forward plane with a small left/right bias. The 3D limbs are tapered ellipsoid volumes with a rounded elbow, replacing the original rods. The flat renderer projects the same chain. Neither body dimensions nor actual stone centers are changed by the helper.

The original four-pixel upward tending arc was geometrically out of reach, especially at six stones. The simulation owner therefore stages a short approach to 5.5 pixels left of the cairn and supplies the real item's revised path: 0.3-pixel unseat, 5.2-pixel left slide, then inspection at 5.5 pixels above the floor and a retrace. The complete action remains 96 ticks after approach. The stone clears the remaining stack before lowering. Placement uses the same approach distance and a rigid forward lean, including the sixth stone. Original player movement, pickup, item identity and reset homes remain owned by the simulation and preservation tests.

A local native probe sampled the delivered helper for all 1–6 stone heights, each tenth of a tick, both hands, both renderers, and body positions at the approach target and ±0.2-pixel stopping tolerance: 69,192 solves, zero unreachable contacts, maximum endpoint reach 8.3506031 pixels against 8.4 available, and maximum segment-length error 0.00001812 pixels. This is construction evidence, not a substitute for real simulation snapshots or camera review. `depth.c` and `render.c` both compiled to isolated native objects after the correction. The simulation owner's `tools/tests/hunter_responses.c` subsequently passed 3,276 actual tending/placement arm solves, including the sixth-stone placement and full six-stone inspection: maximum reach 8.297761 pixels, segment error below 0.0001 pixels. It also checks blocked approach, facing after arrival from the warm seat, pile clearance, exact retrace and room exit/resume with the same real held item.

An impossible arm is never silently stretched or clamped away from the real stone. The caller omits an unsolved chain; reaching such a state is a release-blocking presentation defect and must fail the state audit.

## Corrected actual-camera verdict

Reviewed the final `docs/evidence/hunter/native-v15/manifest.json` capture set at the full room camera: `vault/f0120.png`; tending frames 2238, 2262, 2286, 2310 and 2334; return frames 0050 and 0078; fire frame 0160; and flat-return frames 0050 and 0078. The three presentation source hashes match this manifest exactly: depth `7929d550…`, render `5851a5d5…`, pose helper `dabe0c71…`. Executable SHA256 is `458ccaeb657f8c744238b3286ed283fba6560084a7aa98deac573241d9d958af`.

The rejected overhead rod stretch is resolved in these samples. At `tend/f2286.png`, the elbows fold around the real stone held at the belly, leaving the face readable; both palms remain at its sides. Frames 2262 and 2310 show the stone moving in front of the face during the horizontal transfer, then moving down and back, rather than floating on an elongated diagonal arm. Frame 2334 returns it to the actual pile. Rounded limb volumes read more naturally than the first rods, although their small silhouettes still look deliberately simple. The hat, pack and rigid body remain registered, with no obvious new clipping or hovering. The return/place sample retains contact with the same item. Flat presentation remains legible primarily through the hat, body and changing pile silhouette; it does not convey the full 3D elbow construction at the original pixel resolution.

This is acceptable for the bounded hunter integration checkpoint, not final character-animation or AAA art acceptance. The regular oval cairn and warm-seat occlusion remain visible. The transfer briefly obscures the eyes; entry into the grip is not yet a blended grasp, and sampled stills do not prove temporal continuity, voice quality or player interpretation. Sitting is communicated by a rigid lean and tucked feet rather than a compressed body. Full six-stone reach and conservation are validated by actual simulation tests; the inspected native tending sequence shows the initial four-stone pile. No production changes followed these captures.
