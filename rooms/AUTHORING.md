# ROOM AUTHORING SPEC — frozen. Do not reinterpret.

## The format
A room is 22 rows of exactly 40 characters. Legal tiles, and nothing else:

```
.  empty air
#  rock            solid, blocks light
X  dark            solid, and drawn PURE BLACK. Anything may be hiding in it.
-  ledge           thin one-way platform: stand on it, jump up through it, hold Down to drop
T  timber          scaffold plank, also one-way
~  brine           the salt water. Buoyancy is signed by what you weigh.
=  crust           salt crust. SOLID, but it fatigues under weight and gives way.
R  rim             iron pan lip. SOLID. The only place the kettles can be filled or emptied.
,  salt fringe     decorative, non-solid, leans away as you pass
P  the player start — exactly ONE in the whole world, and it is already placed in (2,2)
```

## Hard rules
1. **Never touch row 0, row 21, column 0 or column 39.** The borders and every doorway
   between rooms are already drawn in `rooms/template.txt`. Copy them exactly. A single
   changed border character fails the build.
2. If your room has a BOTTOM door, the tiles directly above that opening must stay
   passable — a door with rock over it is not a door.
3. If your room has a TOP door, a body must be able to *reach* it from inside the room.
4. Anything at the world's outer rim is sealed rock. That is already in the template.
5. No text, no symbols, no glyphs, no arrows. Nothing that functions as a word.

## The two verbs, exactly as built

**The kettles (verb A).** The player carries a yoke of two kettles and can hold X at a
rim tile to fill (or X+Down to tip out), one kettle per 20 frames, up to 4.
Load changes ONLY things on the gravity axis. Run speed, acceleration and friction are
identical at every load.

| load | jump clears | in brine | crust under you | hook lasts |
|---|---|---|---|---|
| 0 | 4.49 tiles | floats, 2.4 px under | never fails | 257 frames |
| 1 | 3.45 tiles | floats, 5.5 px under | never fails | 129 frames |
| 2 | 2.68 tiles | sinks | fails in 69 f | 89 f |
| 3 | 1.92 tiles | sinks | fails in 67 f | 65 f |
| 4 | 1.34 tiles | sinks | fails in 64 f | 57 f |

Crust fatigue is **dwell time**, not weight: weight only decides whether the clock runs.
Walking across crust heavy is safe. Standing on it is not. Landing on it costs extra,
scaled by impact speed and weight.

**The gaff (verb B).** Hold C near a convex TOP corner of a solid tile within 24 px and
you bite it and become a pendulum. Up/Down slide the grip between 16 and 28 px,
conserving angular momentum, so pulling in whips you faster. Release to launch along the
tangent. The arc never goes above the corner. Corners wear out under weight and shear off.

A corner exists wherever a solid tile has **air above it and air to one side**. That means
the level geometry *is* the set of anchors — and because crust that fails deletes tiles,
the player's own weight manufactures new corners.

## The one-tile gap trap
The body is 12 px tall and a tile is 8 px. **A single tile of clearance is not a gap a
body can enter.** A solid tile one row above a walkable floor seals that corridor
completely, silently, and it will not look sealed in the text.

Every horizontal route needs TWO clear rows. If you want a low place a body squeezes
under, two rows is the minimum and it is already tight. This is also why a staircase
built from solid blocks across a through-route walls it off: build climbs out of one-way
tiles ('-' or 'T') wherever a route passes under them, and use solid stair blocks only
where the corridor genuinely ends.

## Composition rules
- One room is one screen. The camera is locked. There is no scrolling.
- **Each room is one readable still image.** A player should be able to look at it and
  form a hypothesis.
- **Each room teaches exactly one fact.** Not two.
- **No room repeats another's idea.** You are given a specific surprise to showcase.
- Darkness conceals: any `X` tile may be hiding something, and you should use that.
- Do NOT design a puzzle that requires a mechanic not in the table above.
- **Vary your vertical language.** A long diagonal staircase of one-way planks is the
  default answer to "get up there" and it has already been used to death — one room in
  the world has an eleven-step one. Across your rooms, find different ways up: stacked
  landings, a shaft with alternating stubs, hookable beams, a rising crust shelf, a
  column you climb round. `tools/compose.py` will flag PLANK-STAIR at five steps.
- **Use the whole frame.** The camera is locked and all 22 rows are the composition. A
  room whose content sits in the bottom third wastes that, and twenty-five such rooms
  read as one room. `tools/compose.py` flags TOP-EMPTY and SPARSE.
- Do NOT gate anything. Phase 4 builds the world fully traversable with NO gates at all;
  gates are added later, only where testing shows people get lost. Every door you are
  given must remain passable at every load, or reachable by another route in the room.
- **Softlocks are the one unforgivable failure.** If a body can enter a part of your room
  and not get out at any load, it is broken. Check the deep-brine case in particular:
  a load-4 body cannot swim up, so any brine pit deeper than 2 tiles needs either a rim
  at the bottom to tip out at, or a corner to hook, or it is a trap.

## The setting
A drained salt-works of terraced evaporation pans, roofed by collapsing timber scaffold.
Row 0 is the high dry terraces and scaffold; row 4 is the deep pans, still full of brine.
Blind filter-feeders plough slow circuits through the deep brine. Nothing hunts. Nothing
in this world has ever noticed the player.

## What you output
For each room you are given, output EXACTLY this, and nothing else:

```
ROOM x,y teaches=Sn note=<one short line, what the room shows and how>
<22 lines of 40 characters>
```

The `teaches=` must name the surprise from SURPRISES.md that your room showcases. If you
cannot honestly point at one, the room is designed backwards — say so instead of inventing
a room.
