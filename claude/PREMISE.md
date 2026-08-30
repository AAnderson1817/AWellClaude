# FROZEN PREMISE — Brine and Ballast

Chosen at the Phase 0 gate. This file is the authority for every later phase and for
every subagent authoring rooms in Phase 4. If an implementation detail conflicts with
this file, this file wins or the change gets recorded here with a reason.

## Setting (one sentence)
A drained salt-works of terraced evaporation pans, roofed by collapsing timber scaffold,
where blind filter-feeders the size of barges plough slow circuits through the deep brine
and everything the sea left behind is still crystallising.

## Verb A — The Kettles
A shoulder yoke carrying two sealed iron kettles, filled with brine at a pan rim or
emptied over it. **Categorically: scalar magnitudes along the gravity axis only.**

- **First reading.** Your own weight is a dial you turn. Empty you moon-jump — high, slow,
  drifting, unable to shove anything. Full you plummet, and a four-tile drop lands with a
  thud that dimples the crust.
- **Second reading.** Weight does not scale your movement, it swaps which floors exist.
  Salt crust holds a light body and shatters under a heavy one, so the light player has a
  walkable plane the heavy player has never seen. The kettles are sealed air vessels:
  empty you float and cannot descend, full you sink and walk the pan floor. Every room is
  two rooms and the seam is your own body.
- **Cannot:** convert vertical motion to horizontal, hold you against gravity, attach you
  to anything, or move an object that is not underfoot.

## Verb B — The Gaff
A short iron salt-hauling hook on a haft, swung by hand onto a corner within arm's reach.
**Categorically: a positional constraint that redirects momentum you already had.**

- **First reading.** You bite a corner and become a pendulum. No aiming, no throwing, no
  return — it catches an outside corner within arm's reach or it clacks off. Up/down
  slides your grip: pull in and you whip faster, pay out and you lengthen. Releasing at
  different points to see where the launch angle comes out is the whole toy.
- **Second reading.** The pivot is a two-body constraint, not a fixed point, discovered by
  hooking something that is not a wall — a drifting raft, a filter-feeder's dorsal plate.
  And corners are bodies too: heavy swings shear salt corners off mid-arc. The world's
  furniture has a weight class and you are near the bottom of it.
- **Cannot:** add energy, make you fall slower, raise your apex above the corner you
  caught, crack crust, or alter any property of your body.

## The coupling (the whole thesis)
Three shared lines, none written to be clever:
1. `GRAV[load]` is read by both the jump integrator and the pendulum's angular integrator.
2. `(1 + load)` is read by both the crust stress stamp and the corner-wear counter.
3. `corners[]` is derived from the tile grid, and **both verbs delete tiles from it.**

Protect these three. They are where the emergence lives.

## DECISIONS TAKEN AT THE GATE

**D1 — The gaff keeps its ratchet; the deep pan loses its corners.**
The stress test found that the pivot integrator reads gravity and never buoyancy, so
hooking a submerged corner lets a light player pull themselves *down* against buoyancy and
ratchet back up — dissolving the contradiction the diving bell depends on ("heavy enough to
sink, too heavy to climb out, no rim down there").
Resolved in favour of level design over a physics exception: the ratchet **stays**, because
it is a real discovered technique and L7 says unintended routes that don't break the slice
stay in. The bell's gate is instead built on geometry — the deep pan in the bell's room is
smooth-walled below the brine line, so it has no convex corners to catch. A player who
learned the ratchet elsewhere will try it there and find nothing to bite, which is an
honest affordance answer (L6), not a rule that switched itself off.
Consequence for Phase 4: **no convex submerged corner may be authored in the bell's room
below the brine line.** Everywhere else they are welcome.

**D1a — amended at the end of Phase 4.** The constraint was originally handed to (4,4),
the deepest pan. That was wrong, and the contradiction is instructive: a room a heavy
body can only leave via the bell is, by definition, a gate — and L7 requires Phase 4 to
build the world fully open with no gates at all. The two cannot both be true in Phase 4.
So the bell's room is deferred to Phase 5, where it is authored deliberately as the one
place the world is not open, and (4,4) stays an ordinary open deep pan with rims on its
floor and a door at the waterline. The no-corner rule still holds — it just applies to a
room that does not exist yet.

**D2 — The two-body mass ratio is cut from Phase 2/3 scope.**
The stress test found it degenerates to a plain pendulum against static corners, so it is
dead code in most rooms and only fires against hand-tuned entities. Findings 1–7 all
survive without it. It returns only if a filter-feeder or a drifting raft earns it in
Phase 4 — as content, budgeted as content.

**D3 — Crust threshold off-by-one fixed.**
`loadMap` stamps `1 + load` so that presence is distinguishable from absence; the crust
stress threshold is therefore `>= 3`, i.e. load >= 2. This matches the stated rule that
crust holds a body of load 0–1.

**D4 — Weight touches the gravity axis only.**
Run speed, ground/air acceleration, friction and turnaround are identical at every load.
This is the L4 discipline: the moment weight starts changing horizontal handling it begins
doing the gaff's job.

## Layer 2 hook
A cast-iron diving bell, mouth-down on the crust at the lip of the first room's pan, a
rusted lifting eye on its crown, a drift of shrimp shells banked against its rim. In
Layer 1 it cannot be pushed, entered, climbed or tipped, and no room is built around it.
It becomes usable only after failing at the deep pan: hanging off the eye at full load puts
a combined weight on the crust beneath it, the crust gives, and the bell goes down with you
clinging to it, carrying its trapped air pocket. Inside the pocket there is a rim. You go
down heavy and come back light. Nothing says any of this.

## Indifference (L9)
The filter-feeders neither pursue nor avoid. Each ploughs a fixed circuit, mouth open,
straining brine; standing in one means being pushed under the crust by a body that never
changed course. Riding one is worse, not better: it carries you exactly where it was
already going and scrapes you off under a low arch it has been passing for years, at the
same speed, with the same rhythm. No attack animation, because there is no attack.

## Predicted surprises to verify by playing (Phase 3 gate needs 5)
From the stress test. These are hypotheses, not entries — an entry in SURPRISES.md requires
observing it in a running build.
1. Gravity cancels out of the swing's range equation, so a full swing throws the same
   distance at every load; terminal velocity then breaks the tie so the *lightest* body
   throws farthest. Inverts everything the jump table teaches.
2. Swinging carves its own next anchor: the load stamp re-applies every frame from the
   tiles under your feet with no pivot exemption, so the arc's lowest point punches a row
   of discrete holes in crust, and every hole is a fresh convex corner.
3. Filling destroys its own preconditions: buoyancy is negative when light and positive
   when heavy, so filling at a rim over deep brine drops you out of the rim tile mid-fill
   at the instant load crosses over. Escaping needs the gaff on the rim's own corner.
4. Load-change at release: the integrator has no mass term, so changing load in the instant
   before letting go launches under one gravity and falls under another.
5. Corner wear throttles the pumping exploit for free, because both read `(1 + load)` — and
   the failure drops you mid-arc at whatever angle the counter expired on.
6. Heavy play manufactures the hooks light play needs, since standing heavy on crust
   deletes tiles and every deletion makes new convex corners on its neighbours.
