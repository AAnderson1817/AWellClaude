# Phase 4 review findings (running)

## Traversal, after modelling load changes at rims
The analyser does NOT model the gaff, so a CUT may still be crossable by hooking.
Each of these needs checking by hand or by play before it is called broken.

- (3,1) S11 — doors D/L/R not mutually reachable at ANY load. Most likely genuinely cut.
- (4,1) S3  — top door unreachable at loads 3 and 4, and no rim in the room to lighten at.
- (0,2) S6  — doors D/R not mutually reachable at any load.
- (4,2) S2  — doors D/L not mutually reachable at any load.

Clean at all five loads: (0,1) (1,1) (2,1) (1,2) (2,2) (3,2).
Deep brine: (1,1) depth 3 -> hook, (4,1) depth 3 -> rim, (0,2) depth 4 -> rim. All fine.

## Composition, from the contact sheet
Two problems visible across BOTH independently authored rows, so they are systemic and
will need a revision pass rather than a one-room fix:

1. **The top two thirds of nearly every room is empty black.** Content hugs the floor.
   The locked camera is a reading instruction (L11) — a frame that is 60% empty wastes
   it, and it makes 25 rooms read as one room. Rooms need vertical composition:
   things hanging from the ceiling, mid-air structure, something worth looking up at.
2. **Long diagonal timber staircases repeat across rooms.** Several rooms solve
   "get up there" the same way. That is the same idea more than once, which the
   composition rules forbid.
