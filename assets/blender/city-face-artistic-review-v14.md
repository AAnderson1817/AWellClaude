# Carved witness v14: native-assessment candidate

Status: **work in progress**. This is a scoped review of the Drowned Quarter
face, not acceptance of the complete game or its AAA quality target.

The governing image is
`public/art/references/drowned-quarter/03-drowned-face-v2.png`. The required
identity is a monumental mineral carving belonging to the city's unnamed
culture: watchful recessed eyes, a broad nasal base, one downturned mouth
opening, restrained orbital cast metal and repaired stone. Eye motion, collision,
symbols and game behavior are outside this change.

I authored the sculpt and this self-review. The root agent separately inspected
the primary studies and the current source front/three-quarter views. Its fourth
primary-study verdict accepted the nose, mouth, and softened burial silhouette
for secondary construction, with no claim of finished craftsmanship.

## Inspected evidence

- Dense source: `review/city-face-v14/front.png`, `three-quarter.png`,
  `front-clay.png`, `side-clay.png`, `back-clay.png`.
- Actual GLB reimport: `review/city-face-v14/reimport-front.png`,
  `reimport-front-clay.png`, `reimport-three-quarter-clay.png`, and
  `city-face-reimport.png`.
- Matching 320 by 224 clay views: `source-target-size-clay.png` and
  `reimport-target-size-clay.png` in the same review directory.
- Technical evidence: `city-face-manifest.json` and
  `city-face-verification.json`. The latter covers clean source reopen, both GLBs,
  actual embedded C arrays after float32 conversion, bounds, and protected eyes.

The explicit nasal loft now has a central tip, lateral alar volume and two real
nostril recesses. Upper and lower mouth masses meet at one clean cut. The
25,000-triangle head retains these changes in front and oblique reimport views;
the matching small clay views show no material loss of the primary form from
reduction. Bronze strips are flat plates with a surface-conforming width grid
and embedded underside. An earlier two-edge strip construction visibly clipped
through the brow and was rejected before this snapshot.

## Findings that prevent final artistic acceptance

1. The nose, lips and cheeks still shade like smooth sculpted clay. The reference
   has local carving planes and worn transitions. The shared runtime stone maps
   may help surface identity, but cannot establish those missing modeled planes.
2. Broad horizontal rolls cross the lateral cheeks and surrounding stone. These
   originate in construction profiles and resemble repeated undulations instead
   of carved architectural mass.
3. The two clearly triangular outer-edge cuts read as synthetic notches. They
   need a less emblematic fracture treatment, without changing the protected
   silhouette or substituting a pile of ornamental fragments.
4. Some lower orbital plate ends remain visibly proud at inner corners. Their
   termination needs assessment in the actual camera before revising the fit.
5. The actual native camera has now been inspected in
   `assets/review/depth-v14/face-trial-water/f0120.png`. Nose, mouth and orbital
   bands remain legible at roughly 300 pixels of face width; the carving sits
   behind the foreground column, foliage and right masonry without suggesting
   a new platform. The triangular chips and proud band tips are secondary at
   this distance. Smooth, puffed transitions remain the strongest defect,
   particularly the lower lip/chin and lateral facial mass. The native eye
   intensity is restrained in this frame. Animated response is unchanged by
   this asset, but this one image does not independently validate its timing.

The root agent requested this verified snapshot stay frozen for the v14
checkpoint. It also inspected the native image and authorized an isolated
carving-plane study to address the smooth transitions and lateral rolls. Any
production replacement still requires a convincing study and camera comparison.

The isolated follow-up lives in `.local/city-face-study-v14/secondary-planes/`.
An all-planar nasal/lip construction was rejected because it replaced alar and
lip transitions with uniform polygonal ledges. A restrained variant preserves
those rounded transitions and removes the repeated lateral rolls and triangular
bites. A further localized study addresses a measurable chin overlap: the lip
descends to about local `z=.46` near `y=.71`, while the separate scaffold dome
rises again to about `z=.54` below it. Reducing that second rise should remove
the extra padded roll without altering the lip edge. These experiments are not
part of the frozen production snapshot.

That final isolated chin correction has now been rendered and clean-reimported.
It retains the nasal/alar and mouth landmarks, with a more continuous descent
below the lip. The visual improvement is modest; the broad base still looks
smoothly rounded in clay. It is ready for an actual-camera comparison, not
promoted to production. Local study header SHA256:
`4f6ba2b3a710e2ba7e67204298e25523c837e2244f9d56bd87f5884b2afce1dd`.
Its `city-face-verification.json` passes source reopen, GLB and float32 geometry,
normals, bounds and eye checks. Front, oblique and 320-pixel reimport views are in
the study's `review/` directory. Production remains the frozen candidate below.

An additional root-authored material comparison was inspected in
`assets/review/depth-v14/scan-water/f0120.png` against
`candidate-water/f0120.png`, with identical frozen geometry. The scanned stone
adds quiet mineral grain to the face, column and masonry while preserving the
room's value hierarchy. That direction improves material identity; it does not
remove the lower lip/chin roll or substitute for carved transitions. Source
material provenance, packing and delivery validation belong to that separate
material change.

## Frozen candidate

Face: 39,666 triangles, five material groups. Eye: 880 triangles, one material
group. World anchor `(25, 1, -1.8)`. Eye anchors remain local
`(-1.5, 3.5, .165)` and `(1.5, 3.5, .165)`.

- Header SHA256: `584a85eae70803b884e6e1468a7bae3e0abdeaa28f5e7d6a4cc48dc753a36d91`.
- Face GLB SHA256: `37667baf0017cccd797e89a48cce101b93d61e6e51796659c08fca36277450b4`.
- Source SHA256: `056f1a9dbb325bdcc5a8e91cd671c472f8381fcbf05ac785ddff60d118f565c4`.

No degenerate triangles, invalid vertex/corner normals, nonfinite values or
collapsed C triangles were found in this snapshot. The preserved envelope and
eye contract pass. These are packaging and geometry results, not an artistic
quality verdict.
