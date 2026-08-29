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

## Not logged, and why
- "Load 2 can almost hold station against buoyancy." Predicted from the constants
  (SINK 0.22 vs SWIM 0.17), **not observed** — in the deep pan load 2 sank the full five
  tiles to the floor. Reasoning is not observation and it does not go in this file.
