# SURPRISES

Every entry is an interaction observed in a running build, not reasoned about. If it was
not seen on screen or in a trace, it does not go here. Entries become rooms in Phase 4;
a room with no entry behind it was designed backwards and gets deleted.

Format: date — what happened — which two rules collided — the room it became.

---

## 2026-08-29 — Phase 2, verb A alone (the kettles)

### S1. Crust is a dwell-time sensor, not a weight sensor
Standing on the crust bridge at load 3 breaks it in 231 frames. **Walking back and forth
across the same bridge at the same load never breaks it at all** — 454 frames and still
crossing. At 1.45 px/frame a body is over an 8 px tile for about five frames, and the
counter needs twenty-four.

Collision: `StampLoad()` stamps per-tile once per frame × the crust threshold being a
per-tile counter. Weight only decides whether the clock runs; *time* decides whether the
crust fails. Nobody wrote "you can run across thin ice."

This is the L12 beat for verb A and it arrived free: the player learns "heavy breaks
crust" for an hour and then learns the rule was never about weight.

Room: TBD (Phase 4).

### S2. Hopping crosses crust, and small hops beat big ones
Load 3 on the bridge: standing = 231 frames, big hops = 264, **small hops = 306**. Airborne
frames stamp nothing, so a hop buys back its whole airtime — but a landing dimples what it
lands on, scaled by impact speed × weight, so a *high* hop costs more on arrival than it
saved in the air.

Collision: the `!onGround` early-out in `StampLoad()` × the landing-impact stress added for
the premise's "a four-tile drop dimples the crust". Two rules written minutes apart for
unrelated reasons, and between them they define an optimum hop height nobody chose.

Room: TBD.

### S3. How deep you float is the load counter
Load 0 floats with 2.4 px of a 12 px body under the line. Load 1 floats with 5.5 px. Load 2
and up do not float at all. The waterline is a legible, continuous readout of exactly how
much brine you are carrying — and it exists because buoyancy blends `GRAV_L` and `SINK_L`
by submerged fraction, not because anything displays anything.

This is what L5 is supposed to buy. There is no counter, no icon and no bar, and the number
is still on screen.

Room: TBD.

### S4. Crust saws instead of collapsing
A heavy body standing on a crust bridge does not drop the span. It punches out the one or
two tiles under its feet and leaves the rest, so a bridge crossed slowly becomes a row of
separate holes with crust still standing between them.

Collision: the stamp is per-tile and the body is 6 px wide, so only the tiles actually
underfoot ever accumulate. Watched it happen in the f0228 capture — two holes, cracks
still spreading on the tile to the right.

Room: TBD. This is the raw material for a "the floor you already crossed is not the floor
you left" room.

### S5. The water holds you at exactly the height the lip blocks
A floating body sits with its feet below the brine line. A rim flush with the surrounding
floor therefore occupies exactly the band the floating feet are in, so a light player
drifting to the edge of a pan is stopped dead by the lip they are trying to climb onto.
Pressing up frees it instantly — you push up out of the water and land on the rim.

Collision: buoyant equilibrium depth × tiles being solid over their whole 8 px. Not a bug,
but it is the difference between "swim to the edge" and "swim, then climb out", and the
game teaches it by stopping you once.

Room: TBD.

### S6. Brine's surface is a ceiling
Swim thrust cannot lift a body out of brine into air. Holding up while floating raises you
about one pixel and then equilibrates, because the thrust scales down as the submerged
fraction drops. The only way out of a pan is sideways onto something.

Collision: `SWIM_THRUST` scaling by fraction × buoyancy scaling by the same fraction — both
vanish together at the surface. The mirror image of the heavy player's problem: the light
player has a ceiling made of air, the heavy one has a floor made of crust, and neither can
be beaten by pressing harder.

Room: TBD.

---

## 2026-08-30 — Phase 3, both verbs in an empty sandbox

The Phase 3 gate is five surprises from the *pair*, found by playing before any room is
designed. These are the five, plus the measurements behind them.

### S7. Buoyancy is a spring, and the gaff is how you wind it
An empty body floats with 2.4 px under the line and, per S6, cannot swim out of the water.
Hook a ledge below the brine line and you can pull yourself **18 px below your own float
depth**. Let go and buoyancy fires you back up 34 px — 4.3 tiles, within two pixels of a
full load-0 jump on dry land, and it clears the surface by two tiles.

Collision: the pendulum integrator reads `GRAV_L` and never `SINK_L` (PREMISE.md D1 kept
that deliberately) × buoyancy being a restoring force rather than a velocity clamp. Verb B
defeats a ceiling verb A established, and it does it by *storing* verb A's own force.

Held longer than about 45 frames the launch stops growing, because the hang bottoms out
against the ledge geometry — so it is a dial with a real ceiling, not an exploit.

Room: TBD.

### S8. Two wear systems, and which one kills you first inverts partway up the dial
The same hang, feet grazing crust, at each load — frame the nub shears vs frame the crust
gives way:

| load | nub shears | crust gives |
|---|---|---|
| 0 | 257 | never |
| 1 | 129 | never |
| 2 | 89  | **69** |
| 3 | 65  | 67 |
| 4 | 57  | 64 |

Below load 2 only the hook ever fails. At load 2 the floor goes first. From load 3 up the
hook goes first again. Nobody chose that crossover — it is where two independent curves
cross, and both curves are driven by `(1 + load)` because the corner-wear counter and the
crust stamp read the same number.

Room: TBD. This is the best single thing in the build.

### S9. The failure cascade: one hang, two floors
Watched end to end at load 3. Hang heavy over crust → the salt nub you are hanging from
wears through at f=65 and shears → you drop → **the landing impact finishes the crust
below**, which was already fatigued by your grazing feet → you fall through that too, at
f=78. Two separate collapses, thirteen frames apart, from one decision to hang about.

Collision: corner wear × crust fatigue × the landing-impact dimple added in Phase 2 for an
unrelated reason.

Room: TBD.

### S10. Verb A is a level editor for verb B
Standing heavy on crust until it fails raises the room's corner count from 18 to 20,
because `corners[]` is rebuilt from `tiles[][]` and a deleted tile exposes its neighbours'
faces. Then, measured in the same run, the player hooked corner index 18 — **one of the two
its own weight had just manufactured.**

Collision: the third shared line in PREMISE.md, working exactly as written. The heavy
player breaks the ground precisely to create the hooks the light player needs.

Room: TBD.

### S11. The swing throws the same distance whatever you weigh — until it doesn't
Hooked from rest at the same angle and released at straight-down, release speed climbs 27%
from load 0 to load 3 (2.47 → 3.15 px/frame) while the distance thrown moves only 17%
(22.7 → 26.5 px). The gravity that whips you faster drops you sooner. At load 4 the fall
finally wins and distance collapses to 16.2 px.

**This corrects the Phase 0 stress test.** It predicted that gravity cancels exactly and
that terminal velocity would then make the *lightest* body throw farthest. Neither is what
the build does: air friction eats a long flight, so over loads 0-3 the two effects
near-cancel instead, and load 4 breaks the pattern downward rather than load 0 breaking it
upward. The shape of the prediction survived; its direction did not.

Room: TBD.

---

## Not logged, and why
- "Load 2 can almost hold station against buoyancy." Predicted from the constants
  (SINK 0.22 vs SWIM 0.17), **not observed** — in the deep pan load 2 sank the full five
  tiles to the floor. Reasoning is not observation and it does not go in this file.
- "Load-change at the instant of release launches under one gravity and falls under
  another." Predicted by the Phase 0 stress test and **not yet testable**: it needs a rim
  within reach of a hanging body, and no such geometry exists in either sandbox. Left open
  for Phase 4 rather than claimed.
- "Swinging carves a dotted line of holes through crust at the arc's lowest point."
  Partly observed — the feet do stamp only on grazing frames, which is the mechanism — but
  in practice the hook shears before a swinging arc completes enough passes to punch a row.
  What actually happened is S9. The prediction was right about the rule and wrong about
  which failure arrives first.

## Bugs the sandbox found before a player could
- The pendulum let the body swing *above* the corner it was biting and wedge inside the
  nub, with every recovery direction blocked — a hard softlock. Fixed by clamping the arc
  to the half below the anchor and force-releasing after 30 blocked frames.
- `player.hooked` was an index into a list that both verbs cause to be rebuilt. Breaking a
  tile renumbered the corners under an active hook. Corners are now re-identified by
  (tile, side) after a rebuild, and wear is carried across with them — otherwise a nearly
  worn-through hook healed whenever a tile broke somewhere else in the room.
