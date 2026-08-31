# Phase 4 review findings (running)

## Traversal — RESOLVED, and the tool was the thing that was wrong
Four rooms reported CUT at every load. All four were the same false positive: the
authors capped their bottom doors with one-way tiles, which is exactly right — a hatch
you drop through by holding Down — and the analyser had no rule for dropping through a
one-way platform. With that rule added, **all ten authored rooms are clean at all five
loads.** An earlier round of false cuts came from treating load as fixed when a rim lets
the player change it.

Both are worth remembering: this analyser's failures have so far been its own blind
spots, not the rooms'. Check the model before blaming a room.

Deep brine: (1,1) depth 3 -> hook, (4,1) depth 3 -> rim, (0,2) depth 4 -> rim. All fine.

## Composition, from the contact sheet
Two problems visible across BOTH independently authored rows, so they are systemic and
will need a revision pass rather than a one-room fix:

1. **The top two thirds of nearly every room is empty black.** Content hugs the floor.
   The locked camera is a reading instruction (L11) — a frame that is 60% empty wastes
   it, and it makes 25 rooms read as one room. Rooms need vertical composition:
   things hanging from the ceiling, mid-air structure, something worth looking up at.
2. **Long diagonal timber staircases repeat across rooms.** `tools/compose.py` now
   detects them: (1,2) has an ELEVEN step plank stair, (3,2) has seven, (4,2) has six.
   Three of the four row-2 rooms solve "get up there" with the same object. Seen at full
   size, (3,2)'s staircase dominates the frame and drowns the two beams the room is
   actually about — so this is not just repetition, it is the subject being buried.

## Done
- (2,2), the first room, rebuilt. It was the sparsest room in the world at 12.9% fill
  with an empty top band. Now 19.5% with the roof fallen in at both ends, two beams at
  different heights, a stone mass hanging mid-frame, and the bell enlarged to a shaped
  3x2 object that reads as cast iron rather than a crate.


## What the two traversal tools each prove, and what they do not

**tools/traverse.py (static).** Models walking, falling with air control, whole-tile
jumps, dropping through one-way platforms, load-signed buoyancy, and load changes at
rims. Does NOT model the gaff. It proves that every door in a room is reachable from
every other door at all five loads. Combined with the room graph being connected, that
is the traversability claim.

**tools/sweep.py + --wander (empirical).** Seeded bots with no plan, driven through the
real physics with the real verbs. It proves what actually happens rather than what a
model says. It found the two engine bugs that a model could never have found: bodies
could not cross a room edge leftward or upward, and rims were unusable when pressed
flat against them.

**The bot's honest limit.** It reaches 12-13 of 25. It never gets into rows 0 and 1,
because the only ways up are top doors reached by ladders of eight landings on
alternating sides, two rows apart. A player does that without thinking; a bot pressing
a random direction essentially never does, even in 400,000 frames, even with a
climb-when-stuck heuristic and a sideways sweep.

I checked whether that was the room's fault by dumping (1,2) with its reachable set
marked: the U door is reachable, the ladder is eight 2-tile hops, and a load-0 body
clears four tiles. The room is fine. **So the bot's ceiling is a statement about the
bot, and the traversability claim rests on the static analysis.** Do not report the
sweep number as a coverage figure without that caveat.
