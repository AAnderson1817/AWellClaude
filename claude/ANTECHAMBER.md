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

## The six screens: a balcony on the archive

```
+------------------+------------------+------------------+
| A  THE CROWN     | B  THE KEEPER    | C  THE WINDOW    |
| the door's crown,| its head against | a cell open on   |
| the hunters' way | the chamber: the | the same chamber;|
| up to the flue   | pillar of light  | the sitters      |
+------------------+------------------+------------------+
| D  THE DOOR      | E  THE LAP       | F  THE STAIR     |
| you wake at its  | the basin, the   | the tall ones'   |
| foot; the camp   | temples, the tall| treads up to the |
|                  | ones on a bridge | window's lip     |
+------------------+------------------+------------------+
```

The user asked for the door and the start in the lower left, one grander chamber, nothing
across the colossus -- and then for the great portal behind it to go: it was a second
portal beside the window, and seen through, its light did not line up across the two
screens it spanned. So the antechamber is a balcony. Its far wall is open behind the
keeper, between two great piers leaning in toward an arch out of sight, above a low
parapet; the basin is on this side of it. Through the opening is **the rest of the
archive: a chamber miles across**, its cities and temples and constructs in their own faint
light. The keeper sits at the edge with its back to all of it and its hand out toward the
door. The window, the one portal left, looks into the same chamber from the right end.

**The view is a multiplane** (`tools/art/vista.py` paints it, `src/city.c` moves it): five
layers, each at its own distance, drawn behind the far wall wherever the wall is cut away.
Each follows the camera by its depth p (0 the wall, 1 the screen), so when the view slides
the near ones cross the frame and the far ones hardly move:

| layer | p | what is on it |
|---|---|---|
| 0 the haze | 0.80 | the chamber's air, dark overhead and glowing blue-teal at the horizon; shafts of light down through it; the far ceiling's courses of lights; falls of water; the pillar of light, the chamber's heart |
| 1 the horizon | 0.72 | constructs the size of mountains: domes, stepped mesas, an aqueduct, the tower the pillar rises from; another keeper, seated as ours is, a mile off |
| 2 the city | 0.60 | the plain below the horizon, lights in perspective, districts lit and dark, avenues running to the pillar's foot; its skyline |
| 3 the middle distance | 0.42 | a stepped temple with its tiers lit and a rose shrine on top; a domed temple, its oculus amber; the bridge between them with lanterns; spires; a lantern the size of a house hung on a chain |
| 4 near | 0.22 | towers rising out of the dark below; roots and chains hanging; mist; the near bridge where the tall ones stand |

It is composed for the home view between the keeper's two screens, so that each screen
that sees it has its own picture: from B you look up the chamber, the pillar beside the
keeper's back and the lantern over the temple; from E, down on the city, the temples, and
the tall ones on their bridge; through the window from C, the pillar framed in the ring.
Stitched screenshots of the six screens will not line up behind the keeper -- each is its
own camera -- and that is the point: in play the wall slides past the chamber.

What lives in it: lights far off change their minds now and then (the cataloguing); one
light climbs the pillar; a pulse runs up an avenue to its foot; lanterns rise slowly out of
the depths; a procession carries lanterns across the bridge. The near lights -- the
temples', the towers', the bridge's -- are the ones that answer: bring a lamp to the
parapet and they go out one by one, with a murmur, and the tall ones stand dark; they come
back after you leave, and sometimes one of the tall ones is a step nearer.

Rows that matter: the floor is row 38, and the basin's surface is level with it (the
basin runs from col 50 to col 96). The door's crown is row 17, the keeper's shoulder row 14,
the window's lip row 20. The opening is cols 36-86, rows 1-36.

**D, the door.** The largest cell you will stand before: 224 px, twenty times your height,
its ring cut with the catalogue, nine blades closed on a lens that is awake, its foot sunk
below the floor. You wake at its foot. The hunters' camp is to its right, out of the draft
-- bedroll, pot, cairn, the hunter, the cold fire, the pack and its spilled glass -- under
the keeper's fingertips, which reach in from the right. Their planks are driven into the
joints of the door's ring, up its right side. The left pier and a sliver of the view.

**A, the crown.** The planks lead up the door's ring to its crown, and from the crown up
the raw rock to a ledge under the flue, where one of them climbed toward the light and
stayed. The flue is an amber slot in the roof, out of reach; its leaves come down on the
ledge. Over the door, the index band, where the mural is carved.

**B, the keeper.** Its head in profile against the chamber, one eye awake; a lamp on a long
chain before its face. Its arm is the climb, and at its top is the shoulder, where you
stand eye to eye with it. Nothing crosses it.

**E, the lap.** The keeper's lap, shins and seat, the basin at its feet along the
parapet. The paving at the camp's end steps down into the basin; the stone lies on it; the
dead lamps are in a recess by it. Weed stands up from the basin's bed.

**C, the window.** A cell at the right end, its iris drawn back, open on the same chamber;
the pillar of light stands in it. The three sitters are on its lip, inside the ring,
facing out. The right pier.

**F, the stair.** The tall ones' treads, grown by turns out of a rib, from the floor at the
basin's far end up to the window's lip; each tread a bed.

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
  seeds the bake where it is; the great opening (a wash over the middle of the hall, the
  chamber's light), the window (seeded strong), the glass lamps in the masonry and on
  chains, the keeper's eye, the spilled glass, the prints, the mural's phosphor, the fish.
  The view beyond is in its own light and not lit by anything here.
- The hall is never quite dark: a faint cool floor of the archive's light over all of it,
  enough to see the colossus by and not enough to see the floor.
- The stacks either side of the opening are more awake near it, where the chamber's light
  comes in, and dark toward the rock at the two ends.
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
| the veins | all | now and then a pulse of light runs the length of one, down the piers and out over the parapet toward the heart of the chamber: the archive reading |
| the tall ones | E | on the near bridge beyond the parapet; bring a lamp to the parapet (the paving, the water, anywhere along the opening's foot) and the chamber's near lights go out one by one and they stand dark, with a murmur; the lights come back 5-10 s after you leave, and sometimes one of them stands a step nearer |
| the draft | E, B, C | the chamber's air comes in over the parapet with a little dust, rises behind the keeper and goes out of the window; mist lies on the water |

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
  on `paint.py` and `sculpt.py`): the opening and its piers and parapet, the door, the
  window, the stair's rib, the stacks, the colossus sculpted as a height field
  (`colossus.py`), roots and growth.
- `tools/art/vista.py` paints the chamber beyond as five layers (`art/vista_0..4.png`) and
  a table of the layers' depths and the lights that answer (`art/vista.ints`); `--preview
  DIR` composes all six screens with the wall over them, without the game.
- `tools/slide.py OUT.gif TX,TY PLAN FIRST LAST` records a camera slide as the game draws
  it: the one way to see the multiplane work. It writes
  `art/archive.png`, its light `archive_glow.png`, and its veins `archive.veins`;
  `--preview DIR` also shows it with the map's stone laid over it.
  `tools/art/embed.py` compiles `art/*` (pictures, veins, tables) into `src/art_data.c`.
- `tools/screens.py OUT` renders all six screens as the game draws them and stitches them.
