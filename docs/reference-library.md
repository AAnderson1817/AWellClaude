# Reference library: the Vault Mouth and the Drowned Quarter

The library contains **60 final, individually generated reference PNGs: 30 for each of the game's two existing rooms**. The built-in `image_gen` tool generated each subject separately. Three Drowned Quarter outputs were corrected during review; only their final versions count. Contact sheets contain display thumbnails of these originals and do not count as additional references.

The authoritative brief is [PREMISE](../claude/PREMISE.md) and the existing [room response tables](../claude/ROOMS.md). These images guide materials, visual depth, lighting, silhouettes and storytelling. They are not collision maps, final character sheets, runtime screenshots, or evidence that the game has achieved the illustrated fidelity. The frozen room geometry, Hold verb and authored responses remain authoritative.

For comparison, these are separate actual runtime captures: [Vault Mouth](../assets/review/depth-v10/vault/f0005.png) and [Drowned Quarter](../assets/review/depth-v10/drowned/f0120.png). They show the implementation's current appearance; the 60 images below are production references.

## Review and provenance

- [Vault Mouth: all 30 thumbnails](../public/art/references/review/vault-mouth-contact-sheet.jpg)
- [Drowned Quarter: all 30 thumbnails](../public/art/references/review/drowned-quarter-contact-sheet.jpg)
- [Interactive art-direction gallery](art-direction.html)
- [Vault Mouth manifest and complete prompts](../public/art/references/vault-mouth/manifest.json)
- [Drowned Quarter manifest and complete prompts](../public/art/references/drowned-quarter/manifest.json)
- [File verification: dimensions, sizes and SHA256 hashes](../public/art/references/review/verification.json)

Every manifest entry includes its ID, title, filename, subject focus, full final prompt, generation mode and individual visual-review notes. Corrected edits also preserve their original generation prompt. Each final image was inspected at generation, and both contact sheets were reviewed for palette, material continuity and distinct subjects.

The verification script decoded all **60 PNGs**, found **60 unique SHA256 values**, confirmed exactly **30 numbered images per room**, and recorded each image's actual dimensions and byte size. Byte uniqueness verifies distinct files; visual inspection supplies the separate check of subject diversity. Reproduce the file checks and review thumbnails with `public/art/references/verify-library.py` using Python with Pillow. The script never edits original images.

Four initial concepts for a superseded invented environment are archived in ignored `.local/exploratory-references/`. Three rejected Drowned Quarter versions are archived there too. None are included in the final count or room gallery.

## Cohesion rules

The hunters' identity is amber flame, raw rock, timber, rope, canvas and worn iron. The city uses blue-grey dressed stone, dark bronze and copper patina, teal textiles, glass, and **green-white emitted light**. Reflected amber can fall on city material from a hunter's lamp; city fixtures themselves remain green-white. The final banner correction removes incidental warm window emissions from the initial output.

Multiplane depth comes from near silhouettes, crisp middle-plane interaction shapes, layered architecture and softer distant masonry. The two image-02 mattes deliberately omit gameplay platforms and characters so they can sit behind the actual 3D scene. Broad dark intervals and isolated light sources preserve visual rest. Large perspective or decorative structures in concept images do not authorize extra rooms or platforms.

Metallurgy appears through casting, abrasion, repairs, corrosion, mineral deposits and contact polish. Astronomy appears in the door's spiral, quiet orbital incisions, the procession's disc and window metalwork. Faith appears through the hunter's repetitive cairn care, mended belongings, ambiguous procession and the enormous face's minimal response. These are visual interpretations of existing content; they do not introduce a second verb, a new puzzle, or explanatory writing.

## Implementation caveats

Generated concepts are not exact blueprints. The Vault Mouth governing view interprets the floor grate upright; reference 17 gives the correct horizontal grate. Some scene images alter ledge proportions or contain purely decorative distant structures. Character proportions vary in incidental figures; use the existing player and hunter silhouettes. The Drowned Quarter's pack study captures material and dead-lamp glint, but shows a waterline composition; keep the pack on its authored flooded-floor location. Its drain study uses a round outlet; retain the actual shaft. Manifest review fields record the other subject-specific limits.

The initial Drowned Quarter governing view put a creature on a dry terrace; the final v2 removes it. The initial face view added an underwater torch; the final face v2 is a tight submerged carving with green-white eyes. The final native studies have a pale aquatic form, head frill and exactly three green-white flank lights; keep the existing gameplay dimensions and non-contact behavior.

## Vault Mouth inventory

| ID | Reference | Focus |
|---|---|---|
| 01 | [Vault Mouth — Governing Room View](../public/art/references/vault-mouth/01-vault-mouth-governing-view.png) | environment |
| 02 | [Vault Mouth — Distant City Matte](../public/art/references/vault-mouth/02-vault-city-distant-matte.png) | background |
| 03 | [Spiral Door — Shut and Unanswering](../public/art/references/vault-mouth/03-spiral-petal-door.png) | setpiece |
| 04 | [Hunter Camp — Settled Rather Than Escaping](../public/art/references/vault-mouth/04-hunter-camp.png) | environment |
| 05 | [Vault to City — Material Boundary](../public/art/references/vault-mouth/05-vault-to-city.png) | transition |
| 06 | [Cold Fire — Waiting for a Lamp](../public/art/references/vault-mouth/06-cold-fire.png) | prop |
| 07 | [Campfire — First Persistent Change](../public/art/references/vault-mouth/07-lit-fire.png) | lighting |
| 08 | [Cairn — Care Without Explanation](../public/art/references/vault-mouth/08-hunter-cairn.png) | narrative |
| 09 | [Bedroll — Cloth Remembers Weight](../public/art/references/vault-mouth/09-bedroll.png) | prop |
| 10 | [Previous Hunter — Pack and Dead Lamp](../public/art/references/vault-mouth/10-pack-dead-lamp.png) | narrative |
| 11 | [Hunter Rope — Weight and Friction](../public/art/references/vault-mouth/11-hunters-rope.png) | prop |
| 12 | [Ceiling Roots — Sources of Rain](../public/art/references/vault-mouth/12-ceiling-roots.png) | flora |
| 13 | [Mural — Visible in Darkness](../public/art/references/vault-mouth/13-phosphor-mural.png) | setpiece |
| 14 | [Mural — Lost Under Lamplight](../public/art/references/vault-mouth/14-mural-lamplit.png) | lighting |
| 15 | [Shelf Bones — Previous Visitor](../public/art/references/vault-mouth/15-shelf-bones.png) | narrative |
| 16 | [City Column — Worked Stone and Metal](../public/art/references/vault-mouth/16-city-column.png) | architecture |
| 17 | [Shaft Grate — Iron Above Depth](../public/art/references/vault-mouth/17-shaft-grate.png) | architecture |
| 18 | [Paved Street — Footfall History](../public/art/references/vault-mouth/18-paved-street.png) | material |
| 19 | [Balcony Pot — Balanced at an Edge](../public/art/references/vault-mouth/19-clay-pots.png) | prop |
| 20 | [Teal Banner — Living Cloth](../public/art/references/vault-mouth/20-teal-banner.png) | prop |
| 21 | [Balcony — A Built Alcove](../public/art/references/vault-mouth/21-balcony-garden.png) | architecture |
| 22 | [Window — Someone Beyond](../public/art/references/vault-mouth/22-grille-window.png) | setpiece |
| 23 | [City Floor Lamp — Glass and Growth](../public/art/references/vault-mouth/23-floor-lamp.png) | prop |
| 24 | [Hanging Glass — Weight in Suspension](../public/art/references/vault-mouth/24-hanging-lamp.png) | prop |
| 25 | [Vault Bush — Quiet Organic Counterpart](../public/art/references/vault-mouth/25-vault-bush.png) | flora |
| 26 | [City Bulb — Grown Craft](../public/art/references/vault-mouth/26-city-grown-bulb.png) | flora |
| 27 | [Raw Rock — Hunters' Amber Seam](../public/art/references/vault-mouth/27-raw-rock-seam.png) | material |
| 28 | [Vault Mouth — Layered Depth](../public/art/references/vault-mouth/28-vault-multiplane.png) | composition |
| 29 | [Vault Mouth — Sensory Rest](../public/art/references/vault-mouth/29-vault-rest-light.png) | lighting |
| 30 | [Door Glint — A Response Without Opening](../public/art/references/vault-mouth/30-door-lamp-glint.png) | lighting |

## Drowned Quarter inventory

| ID | Reference | Focus |
|---|---|---|
| 01 | [Drowned Quarter — Governing Room View](../public/art/references/drowned-quarter/01-drowned-quarter-governing-view-v2.png) | environment |
| 02 | [Drowned Quarter — Distant Drowned Facades](../public/art/references/drowned-quarter/02-drowned-city-distant-matte.png) | background |
| 03 | [The Face — Too Large for Its Room](../public/art/references/drowned-quarter/03-drowned-face-v2.png) | setpiece |
| 04 | [The Face — A Stone Is Acknowledged](../public/art/references/drowned-quarter/04-face-stone-response.png) | narrative |
| 05 | [Native — At Home Below](../public/art/references/drowned-quarter/05-aquatic-native.png) | fauna |
| 06 | [Native — Patient First Contact](../public/art/references/drowned-quarter/06-native-lamp.png) | narrative |
| 07 | [Native — Inspecting the Weight](../public/art/references/drowned-quarter/07-native-stone.png) | narrative |
| 08 | [Native — A Breath Breaks Stillness](../public/art/references/drowned-quarter/08-native-breath.png) | fauna |
| 09 | [Fish — A Small Living Constellation](../public/art/references/drowned-quarter/09-fish-shoal.png) | fauna |
| 10 | [Fish — Gathered Under the Floating Flame](../public/art/references/drowned-quarter/10-fish-lamp-response.png) | narrative |
| 11 | [Fronds — The Water's Memory](../public/art/references/drowned-quarter/11-submerged-fronds.png) | flora |
| 12 | [Dry Fronds — Above the Flood](../public/art/references/drowned-quarter/12-dry-cornice-fronds.png) | flora |
| 13 | [Sunken Column — A Capital Still Lit](../public/art/references/drowned-quarter/13-sunken-column.png) | architecture |
| 14 | [Drowned Facade — A House Under Water](../public/art/references/drowned-quarter/14-drowned-doorway.png) | architecture |
| 15 | [Roof Terrace — Life at the Edge](../public/art/references/drowned-quarter/15-roof-terrace.png) | environment |
| 16 | [Roof Pot — About to Tip](../public/art/references/drowned-quarter/16-roof-pot.png) | prop |
| 17 | [Sunk Pot — Settled on the Street](../public/art/references/drowned-quarter/17-sunk-pot.png) | prop |
| 18 | [Quarter Window — Unshared Interior](../public/art/references/drowned-quarter/18-drowned-window.png) | setpiece |
| 19 | [Window — Retreat From the Flame](../public/art/references/drowned-quarter/19-window-doused.png) | lighting |
| 20 | [Cornice — Weight Above Water](../public/art/references/drowned-quarter/20-carved-cornice.png) | architecture |
| 21 | [Throat — Iron and Falling Water](../public/art/references/drowned-quarter/21-throat-grating.png) | architecture |
| 22 | [Rope — A Hunter's Descent](../public/art/references/drowned-quarter/22-drowned-rope.png) | prop |
| 23 | [Quarter Banner — Teal Above the Water](../public/art/references/drowned-quarter/23-drowned-banner-v2.png) | prop |
| 24 | [Drain — The Source of the Drips](../public/art/references/drowned-quarter/24-drain-rhythm.png) | lighting |
| 25 | [Lost Pack — Someone Tried the Deep](../public/art/references/drowned-quarter/25-lost-pack.png) | narrative |
| 26 | [Bubbles — A Quiet Exhalation](../public/art/references/drowned-quarter/26-mouth-bubbles.png) | setpiece |
| 27 | [Submerged Metal — Patina and Silt](../public/art/references/drowned-quarter/27-underwater-material.png) | material |
| 28 | [Quarter — Five Depths in Water](../public/art/references/drowned-quarter/28-drowned-multiplane.png) | composition |
| 29 | [Deep Stillness — Sensory Restraint](../public/art/references/drowned-quarter/29-underwater-rest.png) | lighting |
| 30 | [Surface — Two Kinds of Light](../public/art/references/drowned-quarter/30-drowned-dawnless-surface.png) | lighting |
