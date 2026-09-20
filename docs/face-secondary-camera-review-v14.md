# Face secondary geometry: controlled native comparison

**Recommendation: retain the frozen v14 face. Do not promote this isolated
candidate as a complete replacement.** The side cleanup is a small improvement,
but the candidate does not clearly solve the padded lower lip at the actual
camera distance, and its revised forehead partly buries the existing medallion.
Freeze this bounded study as evidence for the next deliberate carving pass.

I authored both face versions and disclose that this is the maker's comparative
review. The native renderer, scanned stone material and comparison fixture are
existing project work. This review makes no performance, immersion or whole-game
AAA acceptance claim.

## Comparison validity

The two separate temporary executables compile one captured copy of all `src`
dependencies. At capture setup, its hashes match the final v14 manifest in
`docs/evidence/craft-v14/native/manifest.json`. Only the candidate's temporary
`depth.c` replaces the face-header include; every other renderer line and every
other dependency is identical between the two builds. Production files were not
edited by this study.

The fixture uses the state from `tools/tests/architecture_capture.c`: room 1,
player tile `(22,3)`, lamp state `(0,1,14)`, fixed simulation step, still depth
presentation, plan `-:1500`, and capture after 120 frames at 1920 by 1080. The
temporary fixture caps rendering at 60 fps and omits the later 600-frame timing
loop. Both processes inherit CPU affinity mask 65535.

The new control image matches **every pixel** of
`docs/evidence/craft-v14/native/with-local-lights/room-1.png`. Thus the capped
pacing did not change the reference state or image. A concurrent root-owned
`src/main.c` edit occurred after the snapshot; it is recorded in the manifest
and cannot enter either executable.

Evidence is in `docs/evidence/face-secondary-v14/`:

- `control/room-1.png` and `candidate/room-1.png`: complete native images.
- `control/face-native-crop.png` and `candidate/face-native-crop.png`: identical
  unscaled pixel crops, rectangle `(980,750)` to `(1360,1025)`.
- `manifest.json`: complete source, fixture, executable and image hashes;
  fixed-state details; pixel comparison; concurrent worktree-change record.
- Both capture logs explicitly state that no performance measurement was made.

The changed pixels occupy `(991,773)` to `(1340,1014)`, including the face and
nearby shading. No changed pixels lie outside the supplied crop. This establishes
the localization of the comparison, not its artistic quality.

## Observations against the governing face reference

The reference remains
`public/art/references/drowned-quarter/03-drowned-face-v2.png`. Its stone has broad
carved transitions, supported orbital metal, a solid nasal base with recessed
nostrils, and a weighty downturned mouth. Those traits matter more than adding
uniform surface detail or angular facets everywhere.

At full-room scale, the candidate preserves the existing watchful expression,
eye anchors, nostril openings, mouth line, foreground occlusion and visual
hierarchy. The face still belongs behind the column, plants and right masonry;
neither version suggests an added platform. Scanned mineral grain is held
constant and helps both versions read as submerged stone.

The reduced lateral rolling transitions are cleaner in the candidate's crop.
Removing the deliberately triangular edge bites also avoids their artificial
punctuation. These are useful local construction changes, but the difference is
small in the complete room and does not create the reference's worn carving
planes by itself.

The nasal bridge is only slightly firmer. The rounded alar transitions and two
nostrils survive, unlike the earlier rejected fully planar study. There is no
new faceted bar across the nasal base. However, this comparison does not show a
substantial improvement in the nose-to-cheek stone transition at roughly
300 pixels of face width.

The chin correction removes a mathematically redundant rise in the supporting
scaffold. It does **not** clearly remove the visible padded lower-lip profile.
The lip remains a broad rounded band; lowering its support can make it appear
more separate from the jaw. This is not yet a convincing improvement in natural
carved form. A future correction should shape the continuous lower lip-to-jaw
surface itself, rather than assume that reducing one overlapping volume fixes
the visible result.

The candidate also introduces a concrete seating regression: the forehead
medallion's lower portion is hidden by the revised stone surface. The control's
disc reads whole; the candidate's disc reads as an unintended partly buried
semicircle. This alters the legibility of the existing ritual/astronomical mark
without an art-direction reason. A future candidate must fit that existing
metal seat to its changed surface before it can replace production.

No extra revision was made during this bounded camera study. The visible gain
does not justify promoting the bundle, and the dominant form issue needs a more
targeted construction decision rather than another small scalar adjustment.

## Frozen evidence identity

| Artifact | SHA256 |
| --- | --- |
| Production face header | `584a85eae70803b884e6e1468a7bae3e0abdeaa28f5e7d6a4cc48dc753a36d91` |
| Isolated face header | `4f6ba2b3a710e2ba7e67204298e25523c837e2244f9d56bd87f5884b2afce1dd` |
| Native control | `fc662a6bf36f6f70163c8fea63d2e77db9d6719f22eb9a46d689a7ae05e68d6e` |
| Native candidate | `cbca70c56b0ac842c3b459c2233e2c626439c03b9906b1b9808e6d1c50ccb2c2` |

The candidate's technical source/GLB/float32 checks remain documented in
`.local/city-face-study-v14/secondary-planes/city-face-verification.json`. Passing
those checks establishes a valid comparison asset, not an artistic pass. Both
versions retain 39,666 face triangles plus the separate unchanged 880-triangle
eye model. Production remains unchanged by this review.
