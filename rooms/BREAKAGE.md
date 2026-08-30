# Adversarial pass — Phase 4

The subagent assigned this was killed before producing anything, so I ran it myself.
Everything below was reproduced in a running build; nothing here is reasoned-only.

## SOFTLOCKS

### 1. (3,0) — sealed chamber under the crust lid. FIXED.
A heavy body standing on the crust lid at row 16 breaks through into a chamber at
rows 17-20 with no door, no rim and no exit. **Crust only heals when you LEAVE a room**,
so the heal can never fire — you are stuck permanently.

Reproduce (before the fix):
```
./build/game --room 3,0 --at 18,15 --load 4 --play "-:400" --frames 420 --trace
```
0 of 8 wandering bots escaped in 1.2 million frames combined. Unescapable at *every*
load, not just heavy: climbing back out needs 5 tiles and a load-0 jump clears 4.49.

Fixed as a route rather than a wall — the chamber floor is raised and a corridor runs
east to the room's own door, so breaking the lid is now a way *down* to the lower exit.
6 of 6 bots escape.

### 2. (2,3) — reported, then withdrawn.
`tools/crusttrap.py` flagged 115 stranded tiles. False positive: its reverse pass held
load fixed and so could not see a heavy body tip out at a rim and float back up. Tool
fixed to work over (tile, load) states like the forward pass. No room can now strand a
body.

## BROKEN DOORS — a whole row of the world was cut off. FIXED.

### 3. All five rooms of row 0 were unreachable through their horizontal doors.
Found by driving (1,0) for real: the body walks to the right door and **stops**, at
x=306 of a possible 320.
```
./build/game --room 1,0 --at 1,15 --load 0 --play "R:26,RJ:9,..." --frames 440 --trace
```
The cause is the same one that has bitten this project twice already. **The body is 12px
tall and a tile is 8px, so it always occupies two rows.** The floor beside those doors
sits at row 16, which puts the body at rows 14-15 — and row 14 at the boundary column is
border rock. The body arrives at the opening with its head in the wall above it.

My own analyser could not see this: it checked door tiles one at a time. A door is only
usable where the boundary column is open on a row AND the row above it. Fixed in
`tools/doors()`, which immediately turned all five row-0 rooms red.

Fixed by dropping the body one row at each of five door columns — (0,0)R, (1,0)L,
(1,0)R, (2,0)L, (4,0)L — so it stands at rows 15-16, both inside the opening.
(3,0)R and (2,0)R already worked and were left alone. A first, over-eager patch that
"fixed" all eight columns re-sealed the corridor from finding 1 and had to be reverted:
rows 17-18 is an equally valid body position and needed no help.

### 4. (3,0) — the crust lid also sealed the room's east exit. FIXED.
With the lid intact the only path to the eastern corridor was through it, and breaking
crust requires weight, which requires a rim. That is a gate, and L7 forbids gates in
Phase 4. The shelf now stops three tiles short of the wall so the corridor is simply
walkable.

## UNINTENDED ROUTES
None found. Specifically tried and could not do it:
- Hooking a corner beside a room edge to swing across the boundary — the arc clamp keeps
  the body below the corner and no transition fires early. `--room 1,0 --at 36,14`.
- Hooking while submerged at load 4 in the rim-only pans (4,3) and (4,4) to beat the
  "you must tip out" rule — no corner is bitable in either; the body simply sinks.
- Breaking crust under a doorway to seal a room — `crusttrap.py` assumes every crust tile
  in a room is already gone and finds nothing stranded.

This is a mildly disappointing result: L7 says unintended routes should be kept, and I
would rather the world had some. It probably reflects that the gaff needs a corner within
24px and the rooms were authored before anyone was hunting for skips.

## WHAT THE TOOLS GOT WRONG
Worth recording, because in this phase the tools were wrong more often than the rooms:
- door tiles checked singly instead of as a body-sized opening (missed a whole row)
- reverse reachability with load held fixed (false trap in (2,3))
- bots started at the world's single start tile in every room, usually inside rock
- one-way platforms not droppable-through (four false cuts, fixed earlier)
- load treated as fixed at rims (three false cuts, fixed earlier)
