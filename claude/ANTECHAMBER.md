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

## The six screens: one wheel

```
+------------------+------------------+------------------+
| A  THE CROWN     | B  THE KEEPER    | C  THE WINDOW    |
| the door's crown,| its head against | on the rim, open |
| the hunters' way | the heart's light| on the archive;  |
| up to the flue   | its shoulder     | the sitters      |
+------------------+------------------+------------------+
| D  THE DOOR      | E  THE LAP       | F  THE STAIR     |
| you wake at its  | the heart's foot,| the tall ones'   |
| foot; the camp   | the basin, the   | treads up to the |
|                  | tall ones        | window's lip     |
+------------------+------------------+------------------+
```

The user asked for the door and the start in the lower left, one grander chamber, and the
six screens as one composition -- and for nothing to cut across the colossus. So the room
is one volume, wall to wall and floor to roof, and its far wall is one picture
(`tools/art/archive.py`) with one idea: **the hall is a cell the size of the hall.**

- **The heart is its lens**, at the centre of the six screens: a round grate 290 px across,
  green light beyond it going in ring after ring, and the tall ones standing in it. The
  keeper sits before it in profile, a silhouette on the light, its hand out toward the
  door. Below the water line the grate is drowned.
- **Its iris is drawn back** into five spokes, ribs laid along radii from the heart's ring
  out to the rim, each with its vein running in to the heart.
- **Its ring is the rim**, a circle through all six screens where the archive is cut into
  the raw rock of the mountain; the collection is inside it in its courses, the seed
  drawers in its outermost band, awake toward the heart and dark toward the rim.
- **On the rim, at the two ends of one diameter through the heart**: the door, low left,
  where you wake, and the window, high right, open on more of the archive. Door, heart,
  window: the one line the whole room is composed on, and the way through it -- in low at
  the left, up and out at the right.

Rows that matter: the floor is row 38, and the basin's surface is level with it (the
basin runs from col 50 to col 96). The door's crown is row 17, the keeper's shoulder row 14,
the window's lip row 20.

**D, the door.** The largest cell you will stand before: 224 px, twenty times your height,
its ring cut with the catalogue, nine blades closed on a lens that is awake, its foot sunk
below the floor. You wake at its foot. The hunters' camp is to its right, out of the draft
-- bedroll, pot, cairn, the hunter, the cold fire, the pack and its spilled glass -- under
the keeper's fingertips, which reach in from the right. Their planks are driven into the
joints of the door's ring, up its right side.

**A, the crown.** The planks lead up the door's ring to its crown, and from the crown up
the raw rock outside the rim to a ledge under the flue, where one of them climbed toward the
light and stayed. The flue is an amber slot in the roof, out of reach; its leaves come down
on the ledge. Over the door, the index band, where the mural is carved. The spoke up-left
from the heart crosses the top.

**B, the keeper.** Its head in profile against the heart's light, one eye awake; a lamp on
a long chain before its face. Its arm is the climb, and at its top is the shoulder, where
you stand eye to eye with it. Nothing crosses it.

**E, the lap.** The heart's foot and the keeper's lap, shins and seat -- a drum of their
stone -- the tall ones standing in the light between its shin and its arm. The paving at
the camp's end steps down into the basin; the stone lies on it; the dead lamps are in a
recess by it. Weed stands up from the basin's bed.

**C, the window.** A cell on the rim, its iris drawn back, open. Through it, more of the
archive (`city.c`): a hall of stacks seen down its length, ribs arching over it bay after
bay, a vein down its floor to another cell far off, awake. The three sitters are on its lip,
inside the ring, facing out.

**F, the stair.** The tall ones' treads, grown by turns out of a rib standing outside the
rim, from the floor at the basin's far end up to the window's lip; each tread a bed. The rim
comes down behind it into the water, a root along it.

## The climbs (all checked by `tools/route.py`)

- **The door** (D1-D10): the floor, six planks up the ring, the crown, two planks up the
  rock, the ledge under the flue.
- **The keeper** (U0-U9): the paving, a plank, a corbel, the hunters' planks, the back of
  the hand, two bands of the forearm, the elbow, the band on the upper arm, the shoulder.
  The lap from the elbow.
- **The stair** (S0-S5): the floor, five treads, the window's lip.
- **Out of the water** at either end (W0, W1).
- **Starting over** from the basin floor holding the stone, the stone going home, and
  letting go early (X0-X2).

The plank and the corbel are one-way, so the floor under them stays open: carrying the
stone you jump too low to climb, and must still be able to walk into the basin and sink.

`tools/escape.py` drops a wandering bot on every surface and asks whether it ever gets
back to the door's screen (standing anywhere in D). All 31 do.

## The light

- **Amber**: the raw rock outside the rim -- its seams, and the cracks of amber in the
  far wall's rock, which light warm -- the flue, your lamp, the fire once lit, and the
  fire's glow on the keeper's fingertips.
- **Green-white**: the archive's own light -- its glass, its lenses, its veins -- which
  seeds the bake where it is; the heart (seeded over its whole disc, so the middle of the
  hall is lit from behind the keeper), the window (seeded strong), the glass lamps in the
  masonry and on chains, the keeper's eye, the spilled glass, the prints, the mural's
  phosphor, the fish.
- The hall is never quite dark: a faint cool floor of the archive's light over all of it,
  enough to see the colossus by and not enough to see the floor.
- The building is a gradient round its heart: the stacks more awake near it, more asleep
  and dark toward the rim.
- The far wall keeps faint shapes in the dark; buried stone keeps its texture.

## What answers you (from the lore)

| thing | where | what it does |
|---|---|---|
| the prints | D | come up on the door's lower face only with no flame within ~8 tiles; gone at once when one comes. The one at your height is brighter, and fades over the first three minutes |
| leaves | A | one every one to two minutes from the flue, spinning down onto the ledge under it; they pile until the reset |
| the mural | A | phosphor procession of tall figures in the dark; under your lamp it fades and the carved small figures in its gaps show |
| the hunter | D | cold fire: tends his cairn now and then. Lit: hands out to it, hums. Lamp near: head turns a pixel. Lamp set on the clean patch among the dead lamps: looks up and hums once |
| the fire | D | lit by the lamp held near (D6: it stays lit until the reset); its glow rims the colossus's fingertips |
| the spilled glass | D | brighter as your lamp nears |
| the fish | E | small green lights in the basin; they turn toward a lamp in the water |
| the sitters | C | on the window's lip, facing out; two breathe mist a minute apart; the nearest turns a pixel toward your lamp; the third is bones |
| the veins | all | now and then a pulse of light runs the length of one, always toward the heart: the archive reading |
| the tall ones | B, E | beyond the heart's grate; bring a lamp near its foot (the paving, the planks, the water before it) and their light goes out, the whole heart dark behind the keeper, with a murmur; it comes back 5-10 s after you leave, and sometimes one stands a step nearer |
| the draft | E, B, C | dust comes out of the heart low and rises behind the keeper and out of the window; mist lies on the water before it |

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
  on `paint.py` and `sculpt.py`): the wheel -- rim, spokes, the heart, the door, the
  window, the stair's rib -- the stacks and seed drawers, the colossus sculpted as a height
  field (`colossus.py`), roots and growth. It writes
  `art/archive.png`, its light `archive_glow.png`, and its veins `archive.veins`;
  `--preview DIR` also shows it with the map's stone laid over it.
  `tools/art/embed.py` compiles `art/*` into `src/art_data.c`.
- `tools/screens.py OUT` renders all six screens as the game draws them and stitches them.
