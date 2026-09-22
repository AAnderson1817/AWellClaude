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
| A  FORECOURT     | B  UPPER GALLERY | C  GREAT WINDOW  |
| the door (15x),  | colossus head,   | far city, ring,  |
| ropes, flue slot,| niche, lamps     | sill, sitters    |
| mural threshold  |                  |                  |
+------------------+------------------+------------------+
| D  UNDERCROFT    | E  HALL FLOOR    | F  INNER GATE    |
| camp, hunter,    | colossus lap,    | fireguard, the   |
| dead lamps,      | hand, basin,     | tall ones, the   |
| spilled glass    | corbels, stone   | giant stair      |
+------------------+------------------+------------------+
```

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
The whole far wall is one picture made from the kit (`tools/art/bay_a.py`). The earlier
four door designs are parked in `parked/art-doors/`.

**B, upper gallery.** The colossus's head fills the back wall inside its niche: a
seated tall one in profile, facing the undercroft, one green glass eye. A lamp on a long
chain hangs before its face. The gallery crosses its shoulder; one stretch of it is a
shelf, where the arm comes up through it.

**C, great window.** A round-headed window the height of the screen, three lancets. The
far city fills it, in parallax: its skyline against its own glow just over the sill, the
ring in the window's head, the plain of lights below the sill. The sill is the one long
flat run, and three figures sit on it facing the city. Everything built is rim-lit green.

**D, undercroft.** Low, raw, amber. The camp: bedroll, cairn, the cold fire, the split
pack with its fan of spilled glass, the hunter between cairn and fire. At the back, a
recess with a row of dead lamps and a clean patch at its end. Roots through the roof, the
ropes from the chasm hanging in. The colossus's fingers hang in at the top right, over
the hunters' planks.

**E, hall floor.** The paving at the undercroft's mouth steps down into the basin; the
colossus's shins and feet show through the water as paler shapes in its blues. The lap
is a ledge. Two corbels on the niche's wall and the hunters' planks lead up to the back
of the hand. The stone lies on the paving.

**F, inner gate.** The fireguard: a barred opening in the far wall, its foot in the
water, green beyond it and the tall ones standing in that light. The giant stair climbs
from the water to the sill's far end, treads a jump high: built for them.

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
back to the forecourt (anywhere in the first screen; the door's foot is a drop and a
checked jump from all of it).

## The light

- **Amber**: the seams in raw rock (A, D), your lamp, the fire once lit, and the fire's
  glow on the colossus's fingertips.
- **Green-white**: the window (seeded strong, so it reaches across the hall), the
  fireguard, the glass lamps in the masonry and on chains, the colossus's eye, the
  spilled glass, the prints, the mural's phosphor, the fish.
- The hall is never quite dark: a faint cool floor of the city's light over all of it,
  enough to see the colossus by and not enough to see the floor.
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
| the fish | E, F | small green lights in the basin, through the bars; they turn toward a lamp in the water |
| the sitters | C | two breathe mist a minute apart; the nearest turns a pixel toward your lamp; the third is bones |
| the tall ones | F | bring a lamp near and their light goes out with a murmur; it comes back 5-10 s after you leave, and sometimes one stands a step nearer |
| the draft | E, F | dust comes in low by the fireguard and drifts along the basin; mist lies on the water |

## Not yet (in the lore, not in this build)

- The swimmer at the lamp, the basking trough, the face's hum.
- Flames leaning toward the city's glass.
- The door's crown lamp a band brighter while you stand near it.
- The ring's pulse a band brighter once the fire is lit.
- The mural's scenes are small and schematic; the colossus is sculpted but not yet
  refined (the hand, the face).
- Verb B.

## Tools

- `tools/antechamber.py` draws the room with shapes and writes `src/antechamber.c`
  (tiles, props, city zones, backdrop pieces, drafts). `tools/roomgen.py` is the library.
- `tools/art/colossus.py` sculpts the colossus as a height field (`tools/art/sculpt.py`)
  into `art/colossus.png`; `tools/art/embed.py` compiles `art/*.png` into
  `src/art_data.c`.
- `tools/screens.py OUT` renders all six screens as the game draws them and stitches them.
