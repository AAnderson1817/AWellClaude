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
