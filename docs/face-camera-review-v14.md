# Independent face camera review, v14 trial

2026-09-20. **Verdict: rework for the requested visual finish; useful frozen integration checkpoint.** I did not author this face, its geometry, materials or renderer. I authored earlier city-response logic and the separate hunter attire. This review compares the actual complete room image `assets/review/depth-v14/face-trial-water/f0120.png` with `public/art/references/drowned-quarter/03-drowned-face-v2.png`. I also read the maker's self-review for package context; the observations below are from the images.

The face now reads as a large, watchful object in the drowned city. Its orbital bands, broad nose, single downturned mouth and forehead disc preserve the reference's identity. In the actual scene it sits behind the column, foliage and right-hand masonry, with the foreground floor continuing across it. It does not suggest a new platform. The subdued eyes remain legible without overpowering the player, and the flooded interval above the face leaves useful visual rest. The visible metropolis behind these objects still supplies depth.

The strongest remaining defect is **broad material form**, especially the nose-to-cheek transition and lower lip/chin. Their continuous soft highlights read as smooth sculpted clay or inflated forms. The reference's weathered mineral has changes of plane, heavier worn cuts and less uniform transitions. This difference survives the native view, where the face occupies roughly 300 pixels of width; it is not merely missing macro-camera texture. The current face's softened, reassuring character also weakens the premise's quiet indifference. Preserve its still gaze and expression while making the carving feel cut from stone; adding anger or a threat would change the brief.

At scene level, the next priorities are:

1. Establish a few deliberate carving planes across the nasal base, cheeks and lip/chin masses, without producing crunchy faceting or altering the frozen eyes/collision envelope. Compare the complete room at the same camera and light before approving the change.
2. Give the face and nearby column convincing mineral response at this scale. A restrained scanned surface may help, but it cannot replace the missing primary planes. Avoid covering the entire face with equally strong cracks, speckle, barnacles or decorative fragments.
3. Continue judging the foreground masonry and column together with the face. Their broad smooth courses and regular dark joints remain conspicuous beside the much richer background painting. Increasing the face's detail alone could widen that mismatch. The required gameplay outline can remain clear while internal stone surfaces become less mechanically uniform.

The isolated edge notches and a few proud orbital plate ends are lower priorities than those broad forms in this frame. They should be revisited when the main mineral treatment settles; their small screen contribution does not justify ignoring a known seating defect, but neither should they consume the pass while the cheeks still read soft. The reference's luminous lenses, mineral weathering and mouth bubbles are not all present in this still; current response timing and the absence of the authored bubbles require separate behavior/content evidence.

This image supports a judgment about composition and still-image craft. It does not verify the seven-second eye cycle, two-second sinking-stone response, audio, frame performance, motion comfort, human discovery or immersion. The missing native creature and unfinished content/ending remain part of the wider game's incomplete goal. A prettier face cannot establish the user's AAA target or all Animal Well design acceptance tests.

## Evidence identity

- Room image SHA256: `0e6ce7b24bb91c30e92fc04b2578e202f81f3829b31a535993e4934130071ceb`.
- Governing reference SHA256: `8f995be1b435ca631423b41eda5ec64909e3a7dc25ef735647c3786fd9289e8c`.
- Frozen face header at review: `584a85eae70803b884e6e1468a7bae3e0abdeaa28f5e7d6a4cc48dc753a36d91`.

The capture folder contains its frame log, but not a complete renderer/terrain hash manifest. This verdict is pinned to the image above; it must not be represented as review of a later combined terrain, lighting or scanned-material build. The face's own technical package is documented separately in `assets/blender/city-face-manifest.json` and `city-face-verification.json`.
