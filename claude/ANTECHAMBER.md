# The antechamber

The two rooms are replaced by one: the hall you come down into when the vault seals
behind you. The lore is `claude/LORE.md` (the city is cold and cannot make fire; this hall
is its hearth; you are the fire). This file is where things are and why they are there.
Geometry lives in `tools/antechamber.py`, which writes `src/antechamber.c`.

## Size, and the camera (L11, amended)

120 x 44 tiles: three screens across, two down. The user asked for one giant room that
is larger than a screen, where running to the edge brings more of it into the same
window. That conflicts with L11 as written (one room is one screen, the camera locks),
so the law was amended rather than quietly broken:

- The view is always exactly one screen of the room, composed as a screen.
- When your centre crosses a screen's edge, the view **slides** to the next screen in 22
  frames, eased at both ends; the world keeps running under it. It never follows you
  inside a screen and never scrolls continuously.
- Going back needs a little more than crossing the line (5 px at the sides, 18 px at the
  top), so a jump that pokes over the top edge and comes down does not slide the view up
  and back.

The locked frame's reading instruction survives: every screen still invites you to read
the whole composition. What changes is that the composition continues past its edges.

## The six screens

```
+------------------+------------------+------------------+
| A  INTAKE        | B  THE KEEPER    | C  STACKS WINDOW |
| the door (15x),  | colossus head in | the window's cell|
| ropes, flue slot,| its cell, gallery| open on more of  |
| mural threshold  | lamps            | the archive; sill|
+------------------+------------------+------------------+
| D  SEED VAULT    | E  THE GARDEN    | F  THE HEART     |
| camp, hunter,    | colossus lap,    | the grate's cell,|
| dead lamps,      | hand, basin,     | the tall ones,   |
| seed drawers     | corbels, stone   | the stair's beds |
+------------------+------------------+------------------+
```

All six are one building in one language (`claude/ARCHIVE.md`), and the far wall of all of
them is one picture, `tools/art/archive.py`. The hall (B, C, E, F) is one wall: the stacks
from the roof to under the water, and on it three cells at the building's scale -- the
keeper's (300 px across, B and E), the window's (C), the heart's (F) -- a pier standing in
the basin between the first two and parting into two ribs, a rib on the stair. Roots come
down from the roof and hug the great cells' rings; every vein runs down to the heart.

Rows that matter: the door's foot is row 21. The walk along the top (the passage, the
gallery, the sill) stands on row 14. The undercroft and the hall floor stand on row 35. The basin's surface
is row 36, its floor row 43.

**A, intake (rebuilt in the archive's language; see `claude/ARCHIVE.md`).** You wake at
the foot of the door: the largest cell in the building, 160 px across, its ring cut with the
catalogue, nine blades closed on a lens that is awake and lights them from within. Two
ribs frame it and lean in toward an arch out of sight overhead; the wall behind is the
collection on its shelves -- sockets, vessels, tablets, gaps -- mostly asleep or dark, this
being the archive's edge. To the right the mountain has pushed back in over the lower
stacks: the chasm drops toward the heart (old ropes over its lip), fallen rock climbs to
the passage, the index frieze runs above it (the mural is carved into it), and a third rib
frames the way into the hall. Every vein runs down into the chasm; pulses run along them.
Its far wall is drawn from the kit by `tools/art/bay_a.py`, into the whole room's picture. The earlier
four door designs are parked in `parked/art-doors/`.

**B, the keeper.** The colossus sits in a cell like every other: a ring 300 px across,
cut with the catalogue, glass set in it, its iris drawn back so only the blades' tips show
round the opening, the recess behind the keeper dark. Its head fills the upper half, in
profile, facing the door, one eye awake; its hand reaches out of its cell. The gallery
crosses its shoulder; one stretch of it is a shelf, where the arm comes up through it. A
lamp on a long chain hangs before its face. The index band runs in from the threshold
and under the ring. A root comes over the ring's top and down its side into the water.

**C, the stacks window.** The window is a cell with its iris drawn back, open. Through it,
more of the archive (city.c): a hall of stacks seen down its length, its ribs arching
over it bay after bay, each a ring of lights smaller than the last, the kept things in
their courses on its walls and up its vault, a vein down the middle of its floor to where
it ends at another cell, awake. Now and then one of those lights changes its mind; the
reading runs down the floor's vein. The sill crosses the window's foot and three figures
sit on it facing the view. The pier on the left, a rib on the right; roots down both sides
of the ring, and on down to the heart.

**D, the seed vault.** The archive's sleeping tier, cut into the rock under the intake:
drawers the size of a seed, packed like comb in cabinets between short piers, a course of
stone every few rows, nearly all of them asleep. The camp is here, out of the draft:
bedroll, cairn, the cold fire, the split pack with its fan of spilled glass, the hunter
between cairn and fire. A recess with a row of dead lamps and a clean patch at its end.
Roots through the roof, the ropes from the chasm hanging in; the chasm's vein comes down
its wall and runs along the floor into the hall. The keeper's fingers hang in at the top
right, over the hunters' planks, trailing growth to the floor.

**E, the garden.** The living tier. The paving at the vault's mouth steps down into the
basin; weed stands up from its bed among drowned stacks; a cell awake over the water's
edge pours growth. The keeper is grown over: moss on every top, a bed on its lap, strands
from its arm, growth hanging from its fingers. It sits on a pier cut short. The lap is a
ledge. Two corbels and the hunters' planks lead up to the back of the hand. The stone
lies on the paving.

**F, the heart.** The heart's cell stands in the water: its ring mossed and gripped by
roots, all its glass awake, every vein in the building ending at it, and its iris a
grate of curved bars -- the fireguard -- with green beyond and the tall ones standing in
that light. The giant stair climbs from the water to the sill's far end, treads a jump
high, each tread a bed.

## The climbs (all checked by `tools/route.py`)

- **Over the chasm** from the door's foot to the fallen rock, and back (T0, T0b). Fall
  short and you land in the undercroft (T2). **Up the fallen rock** to the passage (R1-R3).
- **The long walk**: passage, gallery (over the shelf), sill, never leaving row 14 (T1).
- **The colossus**: hall floor, low corbel, the planks, the back of the hand, two bands
  of the forearm, the elbow, the band on the upper arm, up through the gallery (U1-U8).
  The lap from the elbow (U9).
- **The giant stair**: from the water to the first tread, six treads, the sill's end
  (S0-S7).
- **Starting over** from the basin floor holding the stone, the stone going home, and
  letting go early (X0-X2).

The corbels are shelves so the floor under them stays open: carrying the stone you jump
too low to climb, and must still be able to walk into the basin and sink.

`tools/escape.py` drops a wandering bot on every surface and asks whether it ever gets
back to the intake (standing anywhere in the first screen; the door's foot is a drop and
a checked jump from all of it).

## The light

- **Amber**: the seams in raw rock (A, D), your lamp, the fire once lit, and the fire's
  glow on the colossus's fingertips.
- **Green-white**: the archive's own light -- its glass, its lenses, its veins -- which
  seeds the bake where it is; the window (seeded strong, so it reaches across the hall),
  the heart's grate, the glass lamps in the masonry and on chains, the keeper's eye, the
  spilled glass, the prints, the mural's phosphor, the fish.
- The hall is never quite dark: a faint cool floor of the archive's light over all of it,
  enough to see the colossus by and not enough to see the floor.
- The building is a gradient round its heart: the stacks more awake near it, more asleep
  and dark toward the edges.
- The far wall keeps faint shapes in the dark; buried stone keeps its texture.

## What answers you (from the lore)

| thing | where | what it does |
|---|---|---|
| the prints | A | come up on the door's lower face only with no flame within ~8 tiles; gone at once when one comes. The one at your height is brighter, and fades over the first three minutes |
| leaves | A | one every one to two minutes from the flue, spinning down onto whatever is under it; they pile until the reset |
| the mural | A | phosphor procession of tall figures in the dark; under your lamp it fades and the carved small figures in its gaps show |
| the hunter | D | cold fire: tends his cairn now and then. Lit: hands out to it, hums. Lamp near: head turns a pixel. Lamp set on the clean patch among the dead lamps: looks up and hums once |
| the fire | D | lit by the lamp held near (D6: it stays lit until the reset); its glow rims the colossus's fingertips |
| the spilled glass | D | brighter as your lamp nears |
| the fish | E, F | small green lights in the basin; they turn toward a lamp in the water |
| the sitters | C | facing the window; two breathe mist a minute apart; the nearest turns a pixel toward your lamp; the third is bones |
| the veins | all | now and then a pulse of light runs the length of one, always toward the heart: the archive reading |
| the tall ones | F | bring a lamp near and their light goes out with a murmur; it comes back 5-10 s after you leave, and sometimes one stands a step nearer |
| the draft | E, F | dust comes in low by the fireguard and drifts along the basin; mist lies on the water |

## Not yet (in the lore, not in this build)

- The swimmer at the lamp, the basking trough, the face's hum.
- Flames leaning toward the city's glass.
- The door's crown lamp a band brighter while you stand near it.
- The mural's scenes are small and schematic; the colossus is sculpted but not yet
  refined (the hand, the face).
- Verb B.

## Tools

- `tools/antechamber.py` draws the room with shapes and writes `src/antechamber.c`
  (tiles, props, city zones, backdrop pieces, drafts). `tools/roomgen.py` is the library.
- `tools/art/archive.py` composes the whole far wall from the kit (`tools/art/kit.py`,
  on `paint.py` and `sculpt.py`): bay A (`bay_a.py`), the colossus sculpted as a height
  field (`colossus.py`), the great cells, the stacks, ribs, roots and growth. It writes
  `art/archive.png`, its light `archive_glow.png`, and its veins `archive.veins`;
  `--preview DIR` also shows it with the map's stone laid over it.
  `tools/art/embed.py` compiles `art/*` into `src/art_data.c`.
- `tools/screens.py OUT` renders all six screens as the game draws them and stitches them.
