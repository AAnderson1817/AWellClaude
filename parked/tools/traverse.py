#!/usr/bin/env python3
"""Traversal report for the 5x5 world.

Deliberately CONSERVATIVE: it models only moves it is sure of -- walking, falling,
jumping a whole number of tiles, and floating or sinking in brine by load. It does not
model the gaff at all. So anything it calls reachable really is reachable, and anything
it flags may still be fine via a hook. It over-reports rather than under-reports.
"""
import sys, re
from collections import deque

W = H = 2
RW, RH = 40, 22
SOLID = set('#X=')          # blocks a body
ONEWAY = set('-T')            # solid from above only
BRINE = set('~')
PASS  = set('.,~-T')          # a body can be inside these

# whole tiles of rise, floor(measured apex / 8): 4.49 3.45 2.68 1.92 1.34
JUMP = [4, 3, 2, 1, 1]
FLOATS = [True, True, False, False, False]

def parse(path):
    rooms, meta, start = {}, {}, None
    cur, lines = None, []
    for raw in open(path, encoding='utf-8'):
        line = raw.rstrip('\n')
        if line.startswith(';'): continue
        m = re.match(r'^ROOM\s+(\d),(\d)\s*(.*)$', line)
        if m:
            if cur: rooms[cur] = lines
            cur = (int(m.group(1)), int(m.group(2))); meta[cur] = m.group(3); lines = []
            continue
        if cur is None or not line.strip(): continue
        lines.append(line)
    if cur: rooms[cur] = lines
    for k, g in rooms.items():
        for r, row in enumerate(g):
            c = row.find('P')
            if c >= 0: start = (k[0], k[1], c, r)
    return rooms, meta, start

def solid(rooms, rm, c, r):
    if c < 0 or c >= RW or r < 0 or r >= RH: return True
    ch = rooms[rm][r][c]
    return ch in SOLID

def passable(rooms, rm, c, r):
    if c < 0 or c >= RW or r < 0 or r >= RH: return False
    return rooms[rm][r][c] not in SOLID

def brine(rooms, rm, c, r):
    if c < 0 or c >= RW or r < 0 or r >= RH: return False
    return rooms[rm][r][c] in BRINE

def body_fits(rooms, rm, c, r):
    """The body is ~1.5 tiles tall; require this tile and the one above to be clear."""
    return passable(rooms, rm, c, r) and passable(rooms, rm, c, r - 1)

def near_rim(rooms, rm, c, r):
    """Weight is changed wherever you are in the water -- there is no special tile for
    it any more, so any brine cell is a place you can fill or pour."""
    return brine(rooms, rm, c, r) or (r + 1 < RH and brine(rooms, rm, c, r + 1))

def moves(rooms, rm, load, c, r):
    """Every tile a body at (c,r) with this load can get to in one step. The movement
    rules live here and nowhere else, so any tool that needs the graph agrees with the
    one that walks it."""
    cand = []
    inwater = brine(rooms, rm, c, r)
    grounded = solid(rooms, rm, c, r + 1) or (rooms[rm][r + 1][c] in ONEWAY if r + 1 < RH else False)
    for dc in (-1, 1):
        cand.append((c + dc, r))
    if r + 1 < RH and rooms[rm][r + 1][c] in ONEWAY:
        cand.append((c, r + 2))                      # hold Down and drop through
    if not grounded and not inwater:
        for dc in (-1, 0, 1):
            if passable(rooms, rm, c + dc, r): cand.append((c + dc, r + 1))
    if inwater:
        cand.append((c, r + 1))
        if FLOATS[load]: cand.append((c, r - 1))
    if grounded or (inwater and FLOATS[load]):
        for up in range(1, JUMP[load] + 1):
            if not body_fits(rooms, rm, c, r - up): break
            for dc in range(-up, up + 1):
                if body_fits(rooms, rm, c + dc, r - up): cand.append((c + dc, r - up))
    return cand

def flood(rooms, rm, load, seeds):
    """Reachable (tile, load) states. Load is part of the search because a rim lets the
    player change what they weigh, which changes what they can climb and where they sink."""
    seen = set(); q = deque()
    for s in seeds:
        if body_fits(rooms, rm, *s): seen.add((s[0], s[1], load)); q.append((s[0], s[1], load))
    while q:
        c, r, load = q.popleft()
        loads = list(range(5)) if near_rim(rooms, rm, c, r) else [load]
        for n in moves(rooms, rm, load, c, r):
            if not body_fits(rooms, rm, *n): continue
            for L in loads:
                st = (n[0], n[1], L)
                if st in seen: continue
                seen.add(st); q.append(st)
        for L in loads:
            st = (c, r, L)
            if st not in seen: seen.add(st); q.append(st)
    return seen

def doors(rooms, rm):
    """Tiles just inside each doorway, tagged with the direction they lead.

    A door is only usable where a BODY fits through it, and a body is 12px tall in an
    8px grid: it always occupies two rows. So a horizontal door tile counts only if the
    boundary column is open on that row AND the row above -- otherwise the body walks up
    to the opening with its head in the rock above it and stops, which is exactly what
    room (1,0) did. Same for a vertical door and the columns either side of it."""
    x, y = rm; out = {}
    if x > 0:
        out['L'] = [(1, r) for r in range(1, RH)
                    if passable(rooms, rm, 0, r) and passable(rooms, rm, 0, r - 1)]
    if x < W-1:
        out['R'] = [(RW-2, r) for r in range(1, RH)
                    if passable(rooms, rm, RW-1, r) and passable(rooms, rm, RW-1, r - 1)]
    if y > 0:  out['U'] = [(c, 1) for c in range(RW) if passable(rooms, rm, c, 0)]
    if y < H-1:out['D'] = [(c, RH-2) for c in range(RW) if passable(rooms, rm, c, RH-1)]
    return {k: v for k, v in out.items() if v}

def brine_pools(rooms, rm):
    """Connected brine regions, with their depth and whether a load-4 body can leave."""
    seen = set(); pools = []
    for r in range(RH):
        for c in range(RW):
            if not brine(rooms, rm, c, r) or (c, r) in seen: continue
            q = deque([(c, r)]); cells = set([(c, r)]); seen.add((c, r))
            while q:
                cc, rr = q.popleft()
                for dc, dr in ((1,0),(-1,0),(0,1),(0,-1)):
                    n = (cc+dc, rr+dr)
                    if n in seen or not brine(rooms, rm, *n): continue
                    seen.add(n); cells.add(n); q.append(n)
            top = min(r for _, r in cells); bot = max(r for _, r in cells)
            pools.append((cells, bot - top + 1))
    return pools

def main():
    rooms, meta, start = parse(sys.argv[1] if len(sys.argv) > 1 else 'rooms/world.txt')
    if not start: print("FAIL: no start"); return 1
    print(f"start room ({start[0]},{start[1]}) tile ({start[2]},{start[3]})\n")

    issues = []

    # --- 1. per-room: are all doors mutually reachable, at each load?
    print("ROOM        doors   load0 load1 load2 load3 load4   teaches")
    for y in range(H):
        for x in range(W):
            rm = (x, y); ds = doors(rooms, rm)
            cols = []
            for load in range(5):
                ok = True
                keys = list(ds)
                if len(keys) > 1:
                    base = flood(rooms, rm, load, ds[keys[0]])
                    reach = set((c, r) for c, r, _ in base)
                    for k in keys[1:]:
                        if not any(t in reach for t in ds[k]): ok = False
                cols.append('ok ' if ok else 'CUT')
                if not ok: issues.append(f"({x},{y}) load {load}: doors not mutually reachable")
            t = re.search(r'teaches=(\S+)', meta.get(rm, '') or '')
            print(f"({x},{y})       {''.join(sorted(ds)):5}  " + " ".join(f"{c:5}" for c in cols)
                  + f"   {t.group(1) if t else '-'}")

    # --- 2. deep brine: can a load-4 body get out?
    print("\ndeep brine (a load-4 body sinks and cannot swim up):")
    any_pool = False
    for y in range(H):
        for x in range(W):
            rm = (x, y)
            for cells, depth in brine_pools(rooms, rm):
                if depth <= 2: continue
                any_pool = True
                bot = max(r for _, r in cells)
                floor_cells = [(c, r) for c, r in cells if r == bot]
                rim = any(rooms[rm][r + 1][c] == 'R' for c, r in floor_cells if r + 1 < RH)
                rim = rim or any(rooms[rm][r][c] == 'R' for c, r in cells)
                # a convex corner below the waterline: solid tile with air above and to a side
                top = min(r for _, r in cells)
                corner = False
                for c, r in cells:
                    for cc, rr in ((c-1,r),(c+1,r),(c,r+1)):
                        if 0 <= cc < RW and 0 <= rr < RH and rooms[rm][rr][cc] in SOLID and rr > top:
                            above = rooms[rm][rr-1][cc] if rr > 0 else '#'
                            if above in PASS:
                                sideL = rooms[rm][rr][cc-1] if cc > 0 else '#'
                                sideR = rooms[rm][rr][cc+1] if cc < RW-1 else '#'
                                if sideL in PASS or sideR in PASS: corner = True
                verdict = "rim" if rim else ("hook" if corner else "*** NO WAY OUT ***")
                if not rim and not corner:
                    issues.append(f"({x},{y}) brine pool depth {depth}: no rim and no submerged corner")
                print(f"  ({x},{y}) depth {depth:2d} tiles -> {verdict}")
    if not any_pool: print("  none deeper than 2 tiles")

    # --- 3. the world graph, from the start room
    adj = {}
    for y in range(H):
        for x in range(W):
            ds = doors(rooms, (x, y)); n = []
            if 'L' in ds: n.append((x-1, y))
            if 'R' in ds: n.append((x+1, y))
            if 'U' in ds: n.append((x, y-1))
            if 'D' in ds: n.append((x, y+1))
            adj[(x, y)] = n
    seen = {(start[0], start[1])}; q = deque([(start[0], start[1])])
    while q:
        cur = q.popleft()
        for n in adj[cur]:
            if n not in seen: seen.add(n); q.append(n)
    print(f"\nrooms reachable from the start, by doors alone: {len(seen)}/{W*H}")
    if len(seen) < W * H:
        miss = sorted(set((x, y) for y in range(H) for x in range(W)) - seen)
        print("  unreachable:", miss); issues.append(f"unreachable rooms: {miss}")

    print("\n" + ("-" * 60))
    if issues:
        print(f"{len(issues)} ISSUES")
        for i in issues: print("  ! " + i)
        return 1
    print("no issues")
    return 0

sys.exit(main())
