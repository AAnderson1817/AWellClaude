#!/usr/bin/env python3
"""Crust is destructible and only heals when you LEAVE the room. So if breaking a crust
tile drops a body somewhere it cannot get back out of, the heal can never fire and the
body is stuck for good. The static traversal treats crust as permanently solid and would
never see this.

Worst case: assume every crust tile in the room is already gone. Then ask, of everywhere
a body can reach, whether it can still get back to a door. Uses the shared move rules and
a reverse BFS, so it is linear rather than a flood per tile.
"""
from collections import deque
import sys
src = open('tools/traverse.py').read().split("def main()")[0]
g = {}; exec(compile(src, 'traverse', 'exec'), g)
rooms, meta, start = g['parse']('rooms/world.txt')
RW, RH = 40, 22
moves, body_fits, doorsf, flood = g['moves'], g['body_fits'], g['doors'], g['flood']

bad = 0
for y in range(5):
    for x in range(5):
        rm = (x, y)
        grid = rooms[rm]
        ncrust = sum(row.count('=') for row in grid)
        if not ncrust: continue
        gone = dict(rooms); gone[rm] = [row.replace('=', '.') for row in grid]
        ds = doorsf(gone, rm)
        if not ds: continue
        doortiles = set(t for k in ds for t in ds[k])

        for load in (0, 4):
            # Work over (tile, load) states, exactly as the forward flood does. Holding
            # load fixed in the reverse pass reports false traps: a heavy body that falls
            # into brine can tip out at a rim, float up, and climb back.
            fwd = set()
            for k in ds:
                fwd |= flood(gone, rm, load, ds[k])
            radj = {}
            for (c, r, L) in fwd:
                nxt = [(n[0], n[1], L) for n in moves(gone, rm, L, c, r)]
                if g['near_rim'](gone, rm, c, r):
                    nxt += [(c, r, L2) for L2 in range(5)]
                for st in nxt:
                    if st in fwd: radj.setdefault(st, []).append((c, r, L))
            seen = set(st for st in fwd if (st[0], st[1]) in doortiles)
            q = deque(seen)
            while q:
                cur = q.popleft()
                for p in radj.get(cur, []):
                    if p not in seen: seen.add(p); q.append(p)
            # Tiles on the frame are doorways out of the room, not places to be stuck.
            stranded = sorted(set((c, r) for c, r, _ in fwd - seen
                                  if 0 < c < RW - 1 and 0 < r < RH - 1))
            if stranded:
                bad += 1
                print(f"({x},{y}) entering at load {load}: {ncrust} crust tiles; if they all "
                      f"break, {len(stranded)} tiles can no longer get back to a door")
                print(f"    e.g. {stranded[:8]}")

print()
print("no room can strand a body by breaking its own crust" if not bad
      else f"{bad} room/load combinations can strand a body")
