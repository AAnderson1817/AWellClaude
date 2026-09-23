# Project state

## Where this is

**One room now: the antechamber, six screens of it** (three across, two down), with a
view that slides a screen at a time. The design is `claude/ANTECHAMBER.md`, the lore
`claude/LORE.md`. What follows below the next section is the history that got here; the
two rooms it describes are parked in `parked/rooms-2/`.

### The antechamber (this build)

- **Engine.** The room is 120 x 44 tiles. `CameraStep` keeps the view on one screen and
  slides it (22 frames, eased) when your centre crosses an edge, with hysteresis; the
  composite reads the camera (`uCam`) and the room's size (`uWorld`). Drawing, motes, air
  and lights are culled to the view; sounds from a place in the room pan by the view and
  fade off it (`SfxAt`).
- **The far wall is one picture of the whole room**, drawn in the archive's language
  (`claude/ARCHIVE.md`) by `tools/art/archive.py` from the kit (`tools/art/kit.py`): the
  antechamber as a balcony, its far wall open behind the keeper (the colossus, sculpted as
  a height field by `tools/art/colossus.py`) between two piers above a parapet; the door
  at the left end, the window at the right, the stacks, roots, growth. It comes with a
  picture of its own light and the paths of its veins. `src/backdrop.c` decodes it once,
  paints the city's stone and the buried rock into it from the tiles, and sends the
  archive's reading along the veins.
- **The view beyond is a multiplane** (`tools/art/vista.py`, `src/city.c`): the rest of the
  archive, a chamber miles across, in five layers each following the camera by its depth;
  its near lights go out when a lamp comes to the parapet. The window looks into the same
  chamber. The room draws only raw rock's edges and the small things each frame.
- **The hall's lives** (`src/hall.c`): the hunter, the sitters, the prints, the leaves,
  the mural, the dead lamps, the spilled glass, the fish, the draft's dust, the tall ones
  dousing. Three new sounds: hum, murmur, leaf.
- **Lighting** (the pass before this): a 27-colour palette, nearest colour by eye (Oklab),
  higher exposure; mean luminance 18 -> 29, vivid pixels under 1% -> 12-17%.
- **Checks**: `tools/route.py` (every hop of the new climb, the three reset cases),
  `tools/escape.py` (every surface back to the door's screen), `tools/check.sh`.

**Two rooms, before this: the chamber, and a flooded chamber under it.** Running, jumping,
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
| `src/props.c` | the dressing that answers you: a props grid per room, text sprites, the door, the camp |

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
                   cave; panned to where it fell; lower and softer into water. Seldom:
                   about four a minute (it was thirty, and the user asked for calm)
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

### The look test (after the user called the game ugly)

The user's verdict: the game is displeasing to the eye, and that is the core problem. The
diagnosis, against Animal Well's own rendering as its maker describes it (research brief,
Rendering): our lighting was the smooth gradient he rejected -- one light value per tile,
blurred bilinear across the frame and multiplied -- so every frame was 23 percent murky
midtone and about nine thousand colours; we had no rim light, which he calls the
breakthrough; and everything was drawn by code as rectangles.

The look test rebuilds the renderer for the Vault Mouth only, with no gameplay change. The
frame is drawn in three layers (what is lit, what gives its own light, the far city) and
one composite pass puts them together (`render.c`):

- light posterised into four hard bands, dithered only across each band's edge, above a
  floor below which dark is black rather than noise
- point lights (your aura, the lamp, the fire, their lamps) lit and shadowed per pixel,
  marched against the tile grid; the seams' bake read per pixel from the same bake
- rim light on every stone edge and shelf top that faces a light; stone interiors are
  silhouettes; stone against the far city is backlit in their green
- every pixel snapped to one palette of 18 (`PAL` in `render.c`)
- the far city (`city.c`): a glow off the horizon, avenues of light running to a vanishing
  point, far towers, near towers black against the glow, a great ring of lights with a
  pulse, one light forever climbing a spire, all self-lit and all theirs. First shown
  through a break in the Vault Mouth's back wall; the user found it unreadable (a cut-out
  with platforms floating in it, in a room not built around a view) and it muddied a test
  meant to judge the renderer. Taken out; parked (`CITY_ROOM -1`) until a room is composed
  around the view
- a small green lamp in the door's crown: lit at the top by them, the foot by you
- the dark keeps faint shapes (the user's call, after the floor made unlit shelves vanish):
  unlit stone sits one palette step above black and every standable top keeps a dim line,
  against a back wall that stays black, so the climb reads without a light

V toggles the old look for comparing (`--oldlook` starts in it).

**The flooded room, converted.** Under the first cut its water went black and its lamps
posterised into rainbow rings: snapping each band to the nearest colour hopped between the
blues and the greens. Now water has one ramp of its own -- a darker blue for water nothing
lights (added to the palette, now 19), the water blue, the lit blue -- and every pixel in a
water tile is placed on it by how bright it is, so the bands stay one hue and anything
under the surface, you included, goes cold. The surface line is always a shape, bright
when lit. Stone under the water may only take stone and blue. And the bright part of any
seam or of their glass now gives its own light rather than being lit, so the seams on the
flooded floor glow instead of dimming. The tile code in the bake's alpha now tells seam
from stone and water from air. Found on the way: L and V were read inside the physics step, which
can run several ticks per rendered frame, so one press could toggle twice and do nothing.
They are read once per frame now.

### The drawn things, redrawn

Every drawn thing that was programmer art -- rectangles placed by code -- is now a sprite
of hand-placed pixels in the palette, in one file, `src/sprites.c`, as rows of palette
letters like the door. Code chooses the pose and where; it no longer draws shapes.

- **You**: a small creature with a head and a narrower neck, not a capsule; poses for
  standing, two walking steps, rising and falling (poses, not squash: the body is the same
  shape in each). The ears still lag the turn, the eyes are still drawn over the light, the
  part under the surface still goes cold.
- **The animal**: an arched back, a darker belly and spots, two walking steps, sitting; a
  head that lifts when it watches you, with its one green eye; a ringed tail on the lag chain.
- **Birds**: perched with a tail and breast, and now and then a head dropped to look at
  something; wings up and down in flight. **The plant's fruit**, closed and speaking.
  **Bushes**, whose crown leans and shakes over a base that stays put.
- **The lamp, the stone, the pots, the bedroll, the pack and its dead lamp, the cairn, the
  bones, the fire ring, the balustrade**; and in palette colours now, the ropes, roots (brown,
  mossed), the chain lamp, the banner, the capital and base, the grate, the bulbs, the fire,
  and the specks in the air.

One rule in the composite came out of looking at it: drawn things are written with alpha
254 instead of 255, and get a clean band edge instead of dither. Dither is right on a wall;
on a body crossing the edge of a lamp's band it was a checkerboard.

`tools/sprites.py` renders every sprite to a sheet three ways (as drawn, in a lamp's band,
in a dim one, snapped to the palette), and `tools/sprite_set.py NAME` replaces one sprite's
rows -- the loop the art was made in.

### The tileset, hand-made

The tiles -- most of every frame -- were the last programmer art. They are sprites now too:

- **Raw rock** is built from quarter pieces, each chosen by the two neighbours it faces and
  the diagonal between them: inside, top (the surface you stand on, a lit lip, a chip now and
  then), side (a ragged cliff face), underside (dark, with drips), outer corner, inner
  corner. Authored for the left quarters, mirrored for the right. Buried rock, which nothing
  sees the edges of, is one fill.
- **The city's ashlar**: courses in running bond, a lit cap where open above, a shadowed
  foot below. Built things keep straight edges.
- **Seams**: amber crystal veins in raw rock; in the city, their glass lamp in an iron frame.
- **Shelves**: timber planks with cut ends and pegs in the vault; stone cornices with
  dentils in the city. **Moss** hangs from stone or tufts a floor; **lichen** on masonry.
- **The far wall**: hewn blocks in the vault, courses in the city, tiled from 16px art so a
  band of light falling on it reveals stone instead of filling a shape.

Stone and shelves go down with alpha 253 (drawn things 254, the far wall 255), and the
composite takes the silhouette from that rather than from the tile grid: the rim light and
the dark's faint shapes follow the drawn, chipped edge, not the square the tile occupies.
Past the room's edges counts as stone -- found when the border rock was being backlit as if
the blank strip outside the room were the far city. `--albedo` shows the art under flat
light, for judging it.

### The air

Animal Well runs a fluid solver over the whole screen on a layer of its own, and sprites
push into it; it is presentational, never mechanical. `src/air.c` is the same idea, small:
stable fluids on a grid of 4px cells over the room (80x44), stone a wall to it and the
water's surface its floor, ten relaxation passes a step.

What stirs it: your body moving through it; a landing, which shoves the air out sideways
and throws the floor's dust into it; a jump; a splash; the fire, whose heat rises and
carries smoke that leans in a draft that comes and goes; the door's settling dust in the
first seconds and when you land hard beside it; a pot breaking; a bird taking off; a draft
up through the grate from the cistern; mist breathing off the flooded water, lying on it;
a little air falling down the drain. What shows it: dust, smoke and mist drawn into the
lit layer as ordered-dither pixels paler than any wall -- so they are seen only where light
falls, and vanish in the dark -- and the motes and landing dust, which ride the currents.

It is quiet on purpose: nothing billows unless something made it. Nothing reads it, and
headless runs switch it off (the bots run a million frames). `AWELL_AIRDBG=1` prints the
field's density and speed every ten frames -- which is how the first cut was found to be
too thin to see: its puffs peaked at 0.2 and a landing's own push scattered them in a few
frames, and it was drawn in the wall's greys, so the light snapped it to the wall.

### Calm (a tuning, after the Vault Mouth was played)

The user heard the rooms as busy. Measured standing still for a minute: 56 unprompted
sounds, thirty of them drips, thirteen chirps, ten wingbeats. Now: the drips come about
four a minute, a bird does one thing every six to eighteen seconds and it is usually
nothing, the animal sits longer and walks less and chirrs on half its sits, the plant
waits ten to twenty seconds between phrases, and a lit fire crackles every three to nine
seconds instead of every second. Birds are halved: two in the chamber, one below.

    measured   standing still a minute: 56 unprompted sounds -> about 8
               drips 30 -> ~4, chirps 13 -> ~2, wingbeats 10 -> ~1, chirr 3 -> <1

### What lives here

`src/life.c`. Flat arrays and switch statements, no entity base class. Nothing here can
be hurt or hurts you, nothing counts anything, nothing opens anything (L9: dread from
indifference; L3: neither of these is a verb).

- **Bushes** (`b`): a tile, drawn by the room, that leans away from a passing body and
  shakes and rustles when pushed through. Bird perches, too.
- **Birds**: two in the chamber, one below (three and two at first; the user found them a bit much). They sit on the ends of shelves and stone
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

### Hold: the lamp and the stones (the first verb)

`src/items.c`. One hand. X sets down whatever you hold, at your feet on the side you
face (at your feet exactly if that would be inside stone); with your hands free, X takes
the nearest thing within reach. So to swap, X twice. Everything held is an object:
airborne, it leaves your hand and falls; it lands on stone and on shelves crossed from
above; it falls through the grate into the room below; set it down and change rooms and
it stays where you left it -- the first things in the game with rooms of their own.

**The lamp**: a small iron lamp with a pale glass, on the floor three steps right of
where you begin. In water it rises and rests with its glass above the line.

**The stones**: two. One on the animal's ledge in the chamber, one on the left island
below. A stone sinks, slowly, to the flooded floor. While you hold one, the gravity
axis changes and nothing else does (the rule from the first build: weight never touches
running): a jump is 0.78 of a jump, so the rise is 0.6 of the rise, 4.0 -> 2.4 tiles;
you fall a little faster and land harder, and the thud says so; a bulb throws you 0.85
as fast; and in water the sign of buoyancy flips, -0.55 -> +0.30, so you go to the floor
and walk it, and hops down there are two-thirds of a tile. Set the stone down under and
you float back up without it. It stays. You can see it. The other stone is how you get
another go -- losing one to the deep is a consequence, not a dead end.

    measured   jump with a stone 2.37 tiles; walking the flooded floor, ground=1 at
               row 21; hop under water 0.65 tiles; set down under -> back at the
               surface in 5 s, stone resting below; lamp held + X = set down, no swap

The point of the stone is the room you already had: the seams under the water were
"things you can see and cannot reach". Now you can reach them, at the cost of your light
(one hand) and your buoyancy. That tension is the first real one in the game and it
comes from two objects and no rule.

Its light adds to the glow you already carry (the user's call: add, do not replace):
reach 7.4 tiles at peak 0.90, against the aura's 4.6 at 0.42, with a slight flicker.
To make it matter, the seams went from seven to three in the chamber (the start, the
ceiling, the right ledge) and from six to three below (the ceiling by the chimney and
the two under the water -- the ones the stone will be for). The rest of both rooms is
dark until you bring the lamp, or go without it on two eyes and a few pixels.

Second readings are not built yet and are not promised: creatures by light, things that
exist only in the dark. First the user plays it cold. X and C came off the jump keys.

`--lamp ROOM,TX,TY` places the lamp for a probe; stones are authored with `s`. The
trace carries `hold=<0|1|2>`, the lamp's and first stone's room and position, and
`fade=` (the reset's lids, 0..1). In a plan, `H` holds R.

### The Vault Mouth (build 1 of the redesign)

The premise changed (see `claude/PREMISE.md`; the salt-works is parked): you are a
treasure hunter and the vault sealed behind you; under it is the city of the people who
built it and of the hunters who came before. The two rooms are being redressed to it, one
build at a time, against the response tables in `claude/ROOMS.md`. Geometry is frozen and
both sweeps guard it. This build is room 0's dressing and everything on it that answers.

**The light rule.** Two bakes now, never mixed: amber is flame and flame is the hunters'
(your lamp, the fire, the seams in raw rock); green-white is the city's (its glass lamps,
the hanging lamp, the grate's underlight from the cistern). A seam in the city zone is
drawn as a pane of glass in an iron frame and lights cool. You learn who made a thing by
what colour it gives off.

**Zones.** Tiles belong to the vault or the city by a few rectangles per room. City stone
is ashlar with offset joints, city shelves are stone cornices with dentils, the city's far
wall is coursed tighter, its moss is lichen. Room 1 stays raw until build 2.

**Props** (`src/props.c`): a second text grid per room, one letter per thing, and text
sprites for the set pieces -- the door is 40x48 rows of palette letters whose spiral is
the same formula the glint follows. What room 0 has, and what each answers:

- **The door**, shut, in the back wall at the top left. You start on the step beside it
  with your lamp at your feet (D5). Dust sifts from the rock above it for the first three
  seconds and again when you land hard near its foot; your lamp within three tiles sends a
  glint once around the spiral. Nothing opens it.
- **Ropes** from the step's edge and under shelf A8, **roots** from the ceiling and under
  A5, and an **iron chain with a green glass lamp** over shelf A3: rigid pendulums, swung
  by passing through them and by a landing near their foot. Rope creaks, chain clinks, and
  the lamp's light swings with it.
- **The camp** on the vault floor: a bedroll that dents under your feet, a **cold fire**
  that catches when your lamp is within two tiles for two seconds and then stays lit (D6:
  the first change you make that persists; the reset puts it out), a cairn, and a pack
  with a dead lamp like yours whose glass glints when your light is on it.
- **Bones** at the right end of A9: land on the shelf and the skull tips with a rattle.
- **Pots**: two on the balcony, one on the edge of shelf B7. A landing within a tile or
  walking through knocks one; knocked twice on an edge it falls and shatters on the grate.
- **The city's edge**: a capital and base on the pillar, a balustrade on the balcony's
  wall, a teal banner on the wall face below it that ripples when you or the animal pass,
  iron bars over the grate lit from beneath, and the balcony's seam as a glass floor lamp.

    measured   route and escape green with the new start (36/36 home)
               fire catches 108 frames after the lamp is set beside it; pot: knock,
               knock, shatter on the grate; bones rattle on a landing; grit on a hard
               landing at the door; creak on passing the rope; clink on the chain

One bug fixed on the way: the landing flag was decremented at the end of the body's step,
so every system that runs after the body -- the birds' startle, now the props -- read 6
on the landing frame and never saw it. It is counted down at the start now, and birds
startle at a hard landing nearby for the first time.

### Starting over

The rule, from the user after playing the stone: **the player is never soft-locked
without a way to reset.** So there is a way to reset, and it is always there. Hold R.
The dark comes in over a second and a half -- your lids, over everything, the lamp's
glass and the creatures' eyes included; your own eyes narrow at halfway and shut near
the end -- and if you hold it through, you wake where you began, with everything where it
began: every item back on its home tile in its home room, hands empty, the rooms rebuilt.
Let go early and the room comes back three times as fast as it went, and nothing has
happened. One reset per press: holding R down through the wake does not chain. The mix
muffles as your eyes close, the same low-pass as under the surface. One sound marks the
waking, a breath let out over a low tone. No text says any of this; the darkening says it
the moment R is pressed.

There is no progress to keep yet, so the reset is total. When there is progress, it is
kept, and the reset returns only what moves.

What the audit found, before the button was built. Positions: from every one of the 36
standable runs in both rooms, the wander bot -- which does not know the route -- got back
to the start tile (`tools/escape.py`: three seeds, forty minutes of play each; the
slowest was A2 in the flooded room at 24 minutes; a first pass with an eight-minute budget
flagged five surfaces, and all five were the bot being slow). Holding a stone never traps
you either: X puts it down and your jump comes back. Items: the lamp floats and is always
reachable; a stone under water is not, and with both stones under water the flooded floor
is out of reach for good. That is the one soft lock in the game, and R is its way out.
Losing a stone to the deep is still a consequence -- the walk back, and the lamp left
wherever you set it down before the reset returns that too.

    measured   hold 90 frames -> reset; 54 frames to wake; early release back in 15
               X0: from the flooded floor, heavy, lamp a room away -> at the start,
               hands empty, room 0; X1: stone taken up, R held -> stone back on its
               ledge; X2: R held 40 frames and released -> nothing moved, fade to 0

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
- **Never soft-locked without a way to reset** (the user's rule, after the stone). Hold
  R. Every state the game can get into has this way out, and the geometry is swept for
  places a body cannot leave (`tools/escape.py`).

## How it is checked

`tools/probe.py` runs the game headless with a scripted plan and parses the trace.
`tools/route.py` uses it to check every hop of the room's climb by **searching** input
timings -- take-off column, when you jump, how long you hold it, when you stop pushing.
A hop counts as makeable only if some way of playing it lands it.

    tools/build.sh              # web, linux, windows
    tools/check.sh              # UBSan regressions for the input latch and Hash2, plus
                                # Python tests for probe/route; needs no build/game
    python3 tools/route.py      # every hop, plus the reset from the flooded floor
                                # (X0-X2); currently "none -- both rooms close";
                                # exits nonzero on an unmakeable hop
    python3 tools/escape.py     # from every standable run, can a bot get back to the
                                # start? currently 36/36; exits nonzero otherwise
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

Arrows or WASD. Z or Space to jump; hold it longer to go higher. X to hold / let go
(the lamp, or a stone; one at a time). Down to drop
through a shelf. Land on a bulb to bounce; press jump as you land on it to bounce
higher. In water: Left and Right to swim, Z or Up held to swim up, Z tapped to jump
off the surface. Hold R to close your eyes and wake where you began, with everything
where it began. L toggles the platform tags.
