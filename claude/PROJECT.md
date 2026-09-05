# Project state

## Where this is

**Two rooms now: the chamber, and a flooded chamber under it.** Running, jumping,
two bulbs, and the water.

The user played the from-scratch room and asked to keep it as the base. First addition
on top of it: the bulb (below).

The previous build (two verbs, four rooms, water and weight) is parked. The one before
that (two verbs, 25 rooms) is on the branch `claude/archive-two-verb-slice` and the tag
`phase4-two-verbs`. `parked/` holds both: `nav-4room/` is the source of the four-room
navigation slice including the buoyancy code, `gaff.c` and `rooms-25/` are the two-verb
work, and `parked/tools/` holds the world-scale analysers that a one-room game does not
need.

Nothing from those is deleted. The buoyancy in `parked/nav-4room/player.c` is the one
thing anyone has liked so far and it comes back when navigation is signed off.

## Why it was rebuilt rather than trimmed

The four-room slice was still the two-verb codebase with parts removed: a world grid of
one dimension, three nested arenas with nothing to allocate, a room-transition path that
could not fire, a text-to-header room compiler for four rooms. Every one of those was
load-bearing for a game that no longer existed, and each was a place for a bug to live
where nobody would look for it.

So: new `src/`, six files, ~950 lines, nothing in it that a room with platforms does not
need. No allocator, no world grid, no room format, no entity system.

## What is in it

| file | what it holds |
|---|---|
| `src/aw.h` | every shared type and constant. 140 lines, the whole surface |
| `src/room.c` | the map (authored as text, in the file), tile flags, the light, all drawing |
| `src/player.c` | movement, collision, the body |
| `src/fx.c` | motes, drips, landing dust. Nothing here is read by a rule |
| `src/render.c` | palette, 320x180 target, the CRT pass |
| `src/main.c` | window, fixed timestep, the switches the headless checks need |

Resolution, tile size and the CRT pass are unchanged from before: 320x180, 8px tiles,
40x22 room, scanline depth modulated per pixel by luminosity, static dither against
banding. Movement tuning is unchanged too, minus everything load-related.

### The light

The one genuinely new system. Light is baked once by relaxation over the tile grid: a
mineral seam pushes into the open space beside it, that space pushes into its neighbours
at 0.796 per step, and stone receives light but never passes it on. The result is
sampled at tile corners into a 41x23 texture, drawn back over the frame multiplied and
then again additively, with bilinear doing the smoothing. The body carries a small
occluded aura of its own.

That is why the room reads as having depth: the far wall is a coursed pattern barely
distinguishable from the dark, and you only see it where something is lighting it.

## Movement, measured

    tap jump         0.90 tiles
    3 frames held    1.65
    6 frames         2.59
    10 frames        3.53
    full             4.01

    run              1.45 px/frame, 0.24 accel on the ground, 0.16 in the air
    turnaround       1.9x accel when pushing against your own momentum
    coyote           4 frames of grace after walking off an edge
    buffer           6 frames of jump-press remembered before landing
    apex             gravity x0.62 while |vy| < 0.70 and the button is held

The room is built to that vocabulary: 2 and 3 tile rises everywhere, no 4s.

### The bulb

A 12x6 dome authored with `o` in the map, sitting on a tile floor. Not solid: you walk
through it, cannot stand on it, and it only answers a fall -- the same crossing test a
shelf uses, in `BulbCrossed`. Landing throws you 5.2 tiles above the crown (a full
jump is 4.0). Press jump as you meet it and it is 6.2: a press counts from 6 frames
before contact (it is the jump buffer, spent on the bulb instead) to 4 frames after
(`bulbGrace`; the lift is added to the bounce already in the air, which gives the same
arc a few pixels behind). A held button is not a press. The timed bounce flashes
brighter and further; that is its only tell. The pad deforms, the body never does (L10).
A bounce is the bulb's, so the jump cut does not apply to it (`player.launched`).

    measured   no press 5.24   timed 6.24 (bonus 1.00 tile; the user asked for the
               first cut's 0.80 to be raised 25%)   window 6 frames early to 4 late
               7 early / 5 late 5.24   held all the way down 5.24

It was first built with an escalating chain (5.2 / 6.7 / 8.7 over three consecutive
landings). The user asked for that to be removed; the constant bounce is what stands.

Placement: one on the left floor under the row-12 shelf -- a bounce and a nudge left
reaches the row-15 shelf -- and one on the right floor under the row-16 shelf, which a
bounce lands you on.

### The flooded room

Reached through the shaft: the whole floor between the pillar and the lip (cols 21-26)
is a one-way shelf over an opening in the border -- a grate. Walk over it like floor;
press Down on it and you fall seven tiles into water. Tap Down and you stop on the shelf
set across the chimney's throat one room down; hold it and you go all the way. The
water is the buoyancy from the earlier build, carried over unchanged: gravity and
buoyancy blended by how much of the body is under, so it floats a quarter submerged and
settles instead of bobbing. Jump or Up held swims you upward; a tap of jump while
floating is a smaller jump off the surface (2.5 tiles, 0.8 of a real one). Out of the
water onto an island, then shelves, then the shelf across the chimney's throat (A1),
then one jump puts you back on the grate above. The approach shelves A2 and A3 each poke
one tile into the chimney, so a plain vertical jump from their end lands on A1 -- no
run-up, no steering into a slot under a ceiling. That was the first thing the user found
frustrating, and it was geometry.

Rooms are stacked; positions are continuous across the seam (y shifts by one room), so
nothing about the motion changes at the cut. Outside a room is stone at the sides, and
past the top or bottom is the NEXT ROOM'S TILES: a body straddling the seam collides
with what is really there. (The first version treated the rows beyond as open air, so a
shelf just inside the next room did not exist until the room switched -- "I fell
straight through A1". The second thing the user found.) Entering a room rebuilds its
tiles, bulbs, baked light, surface and specks; nothing carries over but you.

Which room is shown: down switches when the body's centre crosses the seam, so a body
landing on A1 from above is shown in the room its feet are in. Up switches only when
the centre is 16px past the seam, because a plain jump from A2 to A1 pokes 13px into
the room above at its apex and would otherwise flash the camera there and back. The
cost: for those few frames the body is above the top edge and not drawn.

Light under water dies faster (x0.90 per step) and goes cold (red x0.6, green x0.9),
so the seams on the flooded floor and the pillar under the surface read as things you
can see and cannot reach. You cannot: you are too light to dive. That is deliberate and
it is the seed of the weight verb, if it comes back.

    measured   float 23% under, settles within 0.5px
               surface jump 2.49 tiles; swim up lifts you clear of the water
               shaft down: Down on the shelf -> floating in room 1
               shaft up: one jump from the throat shelf -> on the shelf above
               thirteen flooded-room checks close by search (tools/route.py, W0-W12):
               the hops, the shaft both ways, standing jumps from A2/A3 onto A1, a
               failed exit from A1 landing back on A1 (through it only with Down held)

### Sound

Everything is synthesized at startup in `src/audio.c` from sines, noise, one-pole
filters and a small Schroeder reverb (four combs, two allpasses) that stands in for the
cave. Nothing is loaded; there is nothing to stream. 22050 Hz, mono, about 750 KB of
static PCM. The palette, quiet on purpose:

    step / shelf   a dull tap on stone; a hollower knock with a ring on a shelf,
                   on each bob of the walk cycle, and on a small step down
    land           a thud that sweeps down, louder with the fall, a little of the room
    jump           barely a breath (peak 0.03)
    splash         into the water: a bloop under a rush that darkens; out: lighter,
                   with a few drops after. One per arrival: the settling bob crosses
                   the surface a few times and each crossing is not a new splash
    swim           water moved aside, every 19 frames while you move through it
    drip           a plink whose pitch falls in its first milliseconds, then 1.7 s of
                   cave; panned to where it fell; lower and softer into water. Seldom
    bulb / bulb!   rubber: a low tone with a wobble that settles, soft-clipped; the
                   timed bounce a fifth up and brighter, with a second voice
    ambience       per room, six seconds looped with the seam crossfaded: brown noise
                   under 200 Hz and a three-partial drone that breathes; the flooded
                   room adds a band of water noise that swells. rms 0.01 -- a room,
                   not a sound. (First shipped at 0.02; the user heard it and it was
                   still too much. Halved.)

One effect is not baked in: when the surface is above your head the whole mix goes
through a low-pass that opens and closes smoothly (`AttachAudioMixedProcessor`). You
only hear it on the plunge, because you cannot dive.

`./build/game --wav out.wav` writes every sound in a row with gaps, and the
spectrogram of that file is how the palette was checked: it is what caught the
ambience clipping (a gain that turned brown noise into full-band noise). Headless
runs (`--nodraw`, `--mute`) synthesize everything and count what would have played;
the trace line carries `sfx=<name>`. There is no audio device in this container, so
nothing here has been listened to -- only measured and looked at.

### What lives here

`src/life.c`. Flat arrays and switch statements, no entity base class. Nothing here can
be hurt or hurts you, nothing counts anything, nothing opens anything (L9: dread from
indifference; L3: neither of these is a verb).

- **Bushes** (`b`): a tile, drawn by the room, that leans away from a passing body and
  shakes and rustles when pushed through. Bird perches, too.
- **Birds**: three in the chamber, two below. They sit on the ends of shelves and stone
  runs and on bushes -- perches are derived from the map, never authored -- and when
  you come within about five tiles, or land hard nearby, they leave for a perch far
  from you: a climb-then-settle flight, wings beating, a flutter of air. Left alone
  they turn their heads, sing a two-note chirp when you are not near, and sometimes
  move for no reason. A single eye, drawn after the light pass.
- **The animal** (`m`): long and low, on the right-hand ledge of the chamber. Its ledge
  is the standable run it was set down on, so it walks between that run's ends, turns,
  pauses, sits with a chirr, and when you are within about six tiles it stops, lifts
  its head, and watches you -- the head follows you. It never approaches, never runs.
  Soft pads underfoot. Tail is a lag chain. One green eye, drawn after the light.
- **The plant** (`f`): a stalk with three fruit, one on the upper-left shelf of the
  chamber and one on the right island below. When you are near, the fruit lean toward
  you and it speaks: four to eight syllables, each from one pod, whose mouth opens for
  it, each a voice-like tone through a moving formant at that pod's pitch. Then quiet
  for four to eight seconds. It notices a jump (the pods perk). The pods glow, more
  while speaking. **It says nothing in words** -- L5 -- and nothing you can use. It is
  saying it anyway. This is what "a talking plant" is in a game with no text.

Trace runs report `LIFE birds startled N, plant phrases N, beast turns N, rustles N`.

### Debug tags

Press L (or run with `--labels`) and every standable run gets a two-character tag,
reading order, letter+digit: A1..A9, B1..; shelves in shelf colour, stone in white,
bulbs ^1, ^2 with a dome glyph, the room number top-left. Off by default and not part
of the game -- it exists so a conversation can say "B3" instead of "the third shelf
from the left, the short one". `--labels` also prints the table:

    R0 A1 shelf row  4 cols 19-23
    R0 C4 shelf row 20 cols 21-26      <- the grate over the shaft
    R1 A1 shelf row  1 cols 21-26      <- the shelf across the chimney's throat
    R1 A8 stone row 12 cols 18-20      <- the pillar under the surface

Letters that read as digits at 3x5 (I, O, S, Z) are skipped.

## The rules this build is under

- **L5, no text.** There is none in the game and none is planned.
- **L10, no juice defaults.** No screenshake, no squash and stretch. The body's
  animation is second-order lag only.
- **L11.** One room, one screen, locked camera.
- No combat, no counters, no collectibles, no third verb.

## How it is checked

`tools/probe.py` runs the game headless with a scripted plan and parses the trace.
`tools/route.py` uses it to check every hop of the room's climb by **searching** input
timings -- take-off column, when you jump, how long you hold it, when you stop pushing.
A hop counts as makeable only if some way of playing it lands it.

    tools/build.sh              # web, linux, windows
    tools/check.sh              # UBSan regressions for the input latch and Hash2, plus
                                # Python tests for probe/route; needs no build/game
    python3 tools/route.py      # every hop; currently "none -- the route closes";
                                # exits nonzero on an unmakeable hop
    ./build/game --wander 6 --frames 400000
                                # a dumb bot; currently reaches all 94 surfaces

Two bugs were caught this session by tools rather than by luck, and both are the kind
that look fine from the outside:

1. A mineral seam walled in on all six sides lights nothing and is indistinguishable
   from one that works. `RoomLoad` now warns.
2. A map row one character short reads its last column as the string terminator and
   opens a silent hole in the wall. Hand-editing did exactly that. `RoomLoad` now
   checks every row's width.

And one tool bug: the wander bot read the tile *at* the feet, which names the empty
tile above every one-way shelf, so it reported that no bot had ever stood on a shelf
when they had been standing on them the whole time. Reading half a pixel lower fixed it.
That is the same shape of error as the shelf bug the user found by playing -- a
convention borrowed from one context and used in another where it is off by one.

## What is deliberately not here

Weight. A second verb. Puzzles. Anything that speaks in words. They come back one at a
time, each after the thing under it has been played and accepted.

## Controls

Arrows or WASD. Z, X, C or Space to jump; hold it longer to go higher. Down to drop
through a shelf. Land on a bulb to bounce; press jump as you land on it to bounce
higher. In water: Left and Right to swim, Z or Up held to swim up, Z tapped to jump
off the surface. L toggles the platform tags.
