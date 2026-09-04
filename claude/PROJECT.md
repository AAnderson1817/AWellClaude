# Project state

## Where this is

**Rebuilt from scratch as a single room.** Running and jumping, plus two bulbs.

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

Water and weight. Room transitions. A second verb. Puzzles. They come back one at a
time, each after the thing under it has been played and accepted.

## Controls

Arrows or WASD. Z, X, C or Space to jump; hold it longer to go higher. Down to drop
through a shelf. Land on a bulb to bounce; press jump as you land on it to bounce
higher.
