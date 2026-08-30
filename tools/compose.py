#!/usr/bin/env python3
"""Measure how much of each room's frame is actually used.

The camera is locked, so the whole 22-row image is the composition. A room whose
content all sits in the bottom third wastes the instruction the locked frame gives,
and 25 such rooms read as one room.
"""
import re, sys
RW, RH = 40, 22
rooms, meta = {}, {}
cur, lines = None, []
for raw in open(sys.argv[1] if len(sys.argv) > 1 else 'rooms/world.txt'):
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

print("room    top(1-7)  mid(8-14)  bot(15-20)   fill%  gap  verdict")
worst = []
for y in range(5):
    for x in range(5):
        g = rooms[(x, y)]
        def band(a, b):
            return sum(1 for r in range(a, b + 1) for c in range(1, RW - 1) if g[r][c] != '.')
        t, m_, b = band(1, 7), band(8, 14), band(15, 20)
        tot = t + m_ + b
        fill = 100.0 * tot / (20 * 38)
        # longest run of interior rows that are completely empty
        gap = run = 0
        for r in range(1, 21):
            if all(g[r][c] == '.' for c in range(1, RW - 1)): run += 1; gap = max(gap, run)
            else: run = 0
        # A diagonal plank staircase: 5+ consecutive rows each holding one short
        # one-way segment whose centre marches steadily sideways. It is the default
        # solution to "get up there" and it has been used in room after room.
        centres = []
        for r in range(1, 21):
            segs, run0 = [], None
            for c in range(1, RW):
                oneway = c < RW - 1 and g[r][c] in '-T'
                if oneway and run0 is None: run0 = c
                elif not oneway and run0 is not None: segs.append((run0, c - 1)); run0 = None
            centres.append(sum((a + b) / 2 for a, b in segs) / len(segs) if len(segs) == 1 and segs[0][1]-segs[0][0] <= 12 else None)
        stair = best = 0
        for i in range(len(centres)):
            if centres[i] is None: stair = 0; continue
            if i and centres[i-1] is not None and 0 < abs(centres[i] - centres[i-1]) <= 6:
                stair += 1; best = max(best, stair + 1)
            else: stair = 0
        tag = ""
        if best >= 5: tag += f"PLANK-STAIR({best}) "
        if t < 20: tag += "TOP-EMPTY "
        if gap >= 6: tag += f"DEAD-BAND({gap}) "
        if fill < 18: tag += "SPARSE "
        if tag: worst.append((x, y, tag.strip()))
        print(f"({x},{y})   {t:5}     {m_:5}      {b:5}     {fill:5.1f}  {gap:3}  {tag}")

# --- does any pair of rooms read as the same picture?
# Compare coarse silhouettes: the frame downsampled to 10x11 blocks, occupied or not.
# Two rooms that agree on almost every block are the same still image twice, whatever
# their tiles say, and that is the rule about not repeating an idea.
def silhouette(g):
    sig = []
    for by in range(11):
        for bx in range(10):
            n = sum(1 for r in range(by * 2, by * 2 + 2) for c in range(bx * 4, bx * 4 + 4)
                    if g[r][c] != '.')
            sig.append(1 if n >= 3 else 0)
    return sig

authored = [k for k in rooms if 'PENDING' not in meta.get(k, '')]
sigs = {k: silhouette(rooms[k]) for k in authored}
pairs = []
keys = sorted(authored)
for i in range(len(keys)):
    for j in range(i + 1, len(keys)):
        a, b = sigs[keys[i]], sigs[keys[j]]
        inter = sum(1 for x, y in zip(a, b) if x and y)
        union = sum(1 for x, y in zip(a, b) if x or y)
        if union < 12: continue
        jac = inter / union
        if jac >= 0.72: pairs.append((jac, keys[i], keys[j]))
pairs.sort(reverse=True)
if pairs:
    print("\nrooms that read as the same picture:")
    for jac, a, b in pairs[:12]:
        print(f"  {jac:.2f}  {a} ~ {b}")
else:
    print("\nno two rooms read as the same picture")

print()
if worst:
    print(f"{len(worst)} rooms need a composition pass:")
    for x, y, t in worst: print(f"  ({x},{y})  {t}")
else:
    print("every room uses its frame")
