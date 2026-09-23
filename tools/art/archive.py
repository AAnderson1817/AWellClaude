#!/usr/bin/env python3
"""The archive: the far wall of the whole antechamber, all six screens, made of the
building's parts (tools/art/kit.py, claude/ARCHIVE.md).

The antechamber is a balcony. Its far wall is open in the middle, between two great piers
leaning in toward an arch out of sight overhead, and through the opening is the rest of
the archive: a chamber miles across, its cities and temples and constructs far off in
their own light (tools/art/vista.py draws that, in layers, and src/city.c moves them). A
low parapet runs along the opening's foot; the basin is on this side of it. The keeper
sits at the edge with its back to all of it and its hand out toward the door.

The wall either side of the opening is the collection in its courses, cut into the raw
rock of the mountain at the two ends: at the left end the door, where you wake, the
hunters' planks up its ring and the mural over it; at the right end the window, a cell
open on the same chamber, and the tall ones' stair up to its lip. Veins run down the piers
to the parapet and out over it; roots hang from the roof over the opening.

    A the door's crown, the flue | B the keeper's head  | C the window
    D the door, the camp         | E the keeper's lap   | F the stair

    tools/art/archive.py [--preview DIR]    write art/archive.png, _glow.png, .veins
"""
import math, os, sys
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from sculpt import Form, PAL, TAG_WALL, TAG_THING, TAG_STONE
from paint import Canvas, h2, IDX
from kit import cell, seed_cell, stacks, rib, rock, noise, Veins, lit, arc_marks
import colossus

W, H = 960, 352
TAU = math.tau
D2R = math.pi / 180
# The opening and the window are the map's F_VISTA (36, 1, 51, 36) and F_WINDOW (94, -1, 21,
# 21): the one a rectangle, the other the circle in its square of tiles.
OPEN = (288, 8, 696, 296)               # x0, y0, x1, y1: between the piers, above the parapet
WINDOW = (836.0, 76.0, 84.0, 12.0)      # centre x, y, opening radius, ring
DOOR = (136.0, 248.0, 96.0, 16.0)       # its crown on row 17, where the hunters' plank lies
FLOOR, BED = 304, 344                   # the floor and the basin's surface; the basin's floor
SILL = (800, 872, 160)                  # the window's lip: x0, x1, its top (row 20)
MURAL = (128, 232, 96)                  # the index band over the door, where hall.c carves the mural
COLOSSUS_AT = (288, 8)                  # tools/art/colossus.py: its canvas's top-left in the room
STAIR = ((108, 35), (112, 32), (108, 29), (112, 26), (108, 23))   # the map's treads (tile x, top row), 3 x 2
SPINE = 888                             # the stair's rib, between its two files of treads


def grade(x, y):
    """The odds of a kept thing being awake, or at least asleep, by its place: awake toward
    the opening, where the chamber's light comes in; dark toward the rock."""
    d = min(abs(x - OPEN[0]), abs(x - OPEN[2]))
    w = max(0.0, 1.0 - d / 260.0)
    a = 0.02 + 0.2 * w * w
    return a, a + 0.26 + 0.34 * w


# ------------------------------------------------------------------------------ rings
def moulded_ring(c, cx, cy, r, ring, seed=0.0, stones=40, nodes=12, awake=(0,), mark=1, gain=0.95):
    """A moulded ring from r to r + ring: rounded in section with two fillets, set in its
    stones, the catalogue cut along it, glass in it lit as it is awake."""
    R = r + ring
    v = c.view(cx - R - 2, cy - R - 2, cx + R + 3, cy + R + 3)
    if v.w <= 0 or v.h <= 0: return v
    d, ang = v.polar(cx, cy)
    rim = (d >= r) & (d < R)
    t = (d - r) / ring
    f = Form(v.w, v.h, v.ox, v.oy)
    f.z = np.where(rim, 20 + 6 * np.sqrt(np.clip(1 - (2 * t - 1) ** 2, 0, 1))
                   - 3.0 * ((t > 0.30) & (t < 0.36)) - 3.0 * ((t > 0.64) & (t < 0.70)), -1e9)
    lit(v, f, where=rim, gain=gain)
    for k in range(stones):
        a = seed + k * TAU / stones
        for rr in np.arange(r, R, 0.5): v.put(cx + rr * math.cos(a), cy + rr * math.sin(a), 'deep')
        for j, fr in enumerate((0.2, 0.52, 0.84)):
            arc_marks(v, cx, cy, r + ring * fr, a + 0.4 / R, a + TAU / stones - 0.4 / R, k * 7 + mark + j)
    for k in range(nodes):
        a = -math.pi / 2 + k * TAU / nodes
        x, y = cx + (r + ring * 0.5) * math.cos(a) - 0.5, cy + (r + ring * 0.5) * math.sin(a) - 0.5
        g = 'city' if k in awake else ('coolM' if h2(k, mark, 3) < 0.45 else 'coolD')
        for dx, dy in ((0, 0), (1, 0), (0, 1), (1, 1)):
            v.put(x + dx, y + dy, 'deep'); v.glow(x + dx, y + dy, g)
        v.put(x - 1, y - 1, 'stoneH'); v.put(x + 2, y + 2, 'deep')
    return v


def great_cell(c, C, seed=0.0, teeth=9, stones=40, nodes=12, awake=(0,), mark=1, hole_above=1e9):
    """A cell at the building's scale, its iris drawn back into the ring so only the blades'
    tips show, a ratchet round the opening; the opening a hole in the picture (what is
    beyond shows: city.c) above `hole_above`, and below it a recess, under the water."""
    cx, cy, r, ring = C
    v = moulded_ring(c, cx, cy, r, ring, seed, stones, nodes, awake, mark)
    d, ang = v.polar(cx, cy)
    inner = d < r
    span = TAU / max(teeth, 1)
    rel = ((ang - seed) % span) / span
    depth = 2.0 + 8.0 * (1 - rel) ** 1.5 if teeth else np.full(d.shape, 1.2)
    tooth = inner & (d > r - depth)
    v.fill_mask(tooth, 'dark')
    v.fill_mask(tooth & (d < r - depth + 1.3), 'stone')                   # the blade's edge
    v.fill_mask(tooth & (rel < 0.05), 'stoneL')                            # its tip, lit
    v.fill_mask(tooth & (d > r - 1.2), 'deep')                             # where it goes into the ring
    shadow = inner & ~tooth & (d > r - depth - 1.6)
    v.fill_mask(shadow, 'deep')
    open_ = inner & ~tooth & ~shadow
    above = open_ & (v.Y < hole_above)
    v.clear_mask(above); v.g[above] = -1
    below = open_ & ~above
    v.fill_mask(below, 'deep', TAG_WALL)
    for rr in np.arange(r - 20, 8, -16):                                   # the drowned part: its courses
        v.fill_mask(below & (np.abs(d - rr) < 0.7), 'dark', TAG_WALL)
    return v


# ------------------------------------------------------------------------------ the door
def door(c, V):
    """The largest cell you will stand before: its ring cut with the catalogue, nine blades
    closed on a lens that is awake and lights them from within. Its foot is sunk below the
    floor."""
    DX, DY, DR, RING = DOOR
    cell(c, DX, DY, DR, 'living', seed=-math.pi / 2, blades=9, ring=RING)
    for k in range(36):
        a = k * TAU / 36
        for r in np.arange(DR, DR + RING, 0.5): c.put(DX + r * math.cos(a), DY + r * math.sin(a), 'deep')
        for j, fr in enumerate((0.25, 0.55, 0.85)):
            arc_marks(c, DX, DY, DR + RING * fr, a + 0.03, a + TAU / 36 - 0.03, k * 7 + 1 + j)
    for k in range(12):                                                   # glass in the ring
        a = -math.pi / 2 + k * TAU / 12
        x, y = DX + (DR + RING * 0.5) * math.cos(a), DY + (DR + RING * 0.5) * math.sin(a)
        for dx, dy in ((0, 0), (1, 0), (0, 1), (1, 1)):
            c.put(x + dx, y + dy, 'deep'); c.glow(x + dx, y + dy, 'city' if k == 0 else 'coolM')
    for i in range(9):                                                    # the blades' midribs: the door reading you in
        am = -math.pi / 2 + i * TAU / 9 + 0.6
        for r in np.arange(DR - 3, 22, -1.0):
            a = am + (DR - r) * 0.013
            if int(r) % 3 == 0: c.glow(DX + r * math.cos(a), DY + r * math.sin(a), 'coolD' if int(r) % 9 else 'coolM')
    # its ring's veins: from the crown down both sides; the right one on along the floor to
    # the pier, and down the pier's plinth to the parapet
    rr = DR + RING - 2.5
    right = [(DX + rr * math.cos(a), DY + rr * math.sin(a)) for a in np.linspace(-math.pi / 2, 0.35, 50)]
    V.add(c, right + [(DX + rr * math.cos(0.35), FLOOR - 3), (OPEN[0] - 10, FLOOR - 3)], lit_every=6)
    left = [(DX + rr * math.cos(a), DY + rr * math.sin(a)) for a in np.linspace(-math.pi / 2, -math.pi * 1.1, 50)]
    V.add(c, left, lit_every=6)


# ------------------------------------------------------------------------------ the living
def spill(c, x, y, n, seed, length=30, flower=0.08):
    """Growth pouring out of an open cell and down: vines, their leaves, now and then a flower."""
    for k in range(n):
        dx = (h2(k, seed, 1) - 0.5) * 10
        L = length * (0.5 + h2(k, seed, 2) * 0.7)
        pts = []
        for i in range(int(L)):
            pts.append((x + dx + math.sin(i * 0.25 + k) * 1.5 + (h2(k, seed, 5) - 0.5) * i * 0.15, y + i))
        c.vine(pts, seed=seed * 7 + k, leaf=0.45, flower=flower)


def reeds(c, x0, x1, y, seed, hmin=5, hmax=16, every=2):
    """Growth standing up from a bed: stems, leaning a little, lit at their tips."""
    for x in range(int(x0), int(x1), every):
        if h2(x, seed, 1) < 0.25: continue
        hh = hmin + h2(x, seed, 2) * (hmax - hmin)
        lean = (h2(x, seed, 3) - 0.5) * 0.5
        for i in range(int(hh)):
            px, py = x + lean * i, y - i
            c.put(px, py, 'mossD' if i < hh * 0.5 else ('moss' if i < hh - 2 else 'mossL'))
            if i > 2 and h2(x, i, seed + 4) < 0.18: c.put(px + (1 if (x + i) % 2 else -1), py, 'moss')


def overgrow(c, m, seed, density=0.55, hang=0.0, hang_len=10):
    """Moss on the tops of whatever m covers, and, if asked, strands hanging from its undersides."""
    top = m & ~np.roll(m, 1, axis=0)
    top[0, :] = False
    c.moss(top & (noise(c, 4, seed) < density + 0.15), density + 0.3, seed)
    c.moss(np.roll(top, -1, axis=0) & (noise(c, 3, seed + 1) < density - 0.2), density, seed + 2)
    if hang > 0:
        bot = m & ~np.roll(m, -1, axis=0)
        ys, xs = np.nonzero(bot)
        for x, y in zip(xs + c.ox, ys + c.oy):
            if h2(x, y, seed + 3) < hang:
                L = 3 + int(h2(x, y, seed + 4) * hang_len)
                for i in range(1, L):
                    c.put(x + (1 if i > L * 0.6 and x % 2 else 0), y + i, 'mossD' if i < L - 1 else 'moss')


# ------------------------------------------------------------------------------ the picture
def pier(c, V, x, w, lean, seed, root_side):
    """One of the two piers either side of the opening: a rib at the building's scale, from
    a plinth on the floor up past the roof, leaning in toward the arch they close out of
    sight; its vein runs down it to the parapet."""
    rib(c, x, -80, FLOOR + 4, w, lean=lean, root_side=root_side, seed=seed, veins=V,
        vein_to=[(x + w / 2, FLOOR - 4), (x + w / 2 + (w / 2 + 6) * (1 if lean > 0 else -1), OPEN[3] - 3)])


def build():
    c = Canvas(W, H)
    V = Veins()
    x0o, y0o, x1o, y1o = OPEN
    rock(c, np.ones((H, W), bool), seed=41)                                # the mountain, under all of it

    # ---- the wall either side of the opening: the collection in its courses, cut into the
    # rock at the two ends of the room; under the parapet, the basin's drowned back wall
    edgeL = 96 + 14 * noise(c, 20, 3) + 0.08 * (H - c.Y)
    edgeR = 936 - 12 * noise(c, 20, 4)
    cut = (c.X > edgeL) & (c.X < edgeR)
    stacks(c, 0, 2, W, BED + 2, seed=7, grade=grade, pier=48, where=cut)
    for e, side in ((edgeL, 1), (edgeR, -1)):                              # the cut: a shadow along it
        c.fill_mask(cut & (np.abs(c.X - e) < 1.5), 'deep', TAG_WALL)
        c.fill_mask(cut & (np.abs(c.X - e - 2 * side) < 0.6), 'stone', TAG_WALL)

    # the index band over the door, where the mural is carved: a panel let into the stacks,
    # its face plain and dark so the carving and the phosphor read, its frame moulded
    mx0, mx1, my0 = MURAL
    pv = c.view(mx0 - 6, my0 - 8, mx1 + 6, my0 + 38)
    f = Form(pv.w, pv.h, pv.ox, pv.oy)
    f.slab(mx0 - 5, my0 - 7, mx1 + 5, my0 + 37, 6, bevel=2)                # the frame
    f.slab(mx0, my0, mx1, my0 + 30, 3, bevel=1, base=-2)                   # the face, set back
    lit(pv, f, tag=TAG_WALL, gain=0.8)
    face = pv.rect(mx0 + 1, my0 + 1, mx1 - 1, my0 + 29)
    pv.fill_mask(face, 'dark', TAG_WALL)
    pv.fill_mask(face & (noise(pv, 5, 61) > 0.78), 'deep', TAG_WALL)
    for x in (mx0 - 5, mx1 + 1):                                            # its ends: two cells asleep
        seed_cell(c, x, my0 + 12, 'asleep')

    # ---- the opening: nothing, where the rest of the archive shows (src/city.c)
    hole = c.rect(x0o, y0o - 8, x1o, y1o)
    c.clear_mask(hole); c.g[hole] = -1
    # the parapet along its foot, the basin's far edge: coping stones, their joints
    pv = c.view(x0o - 4, y1o - 4, x1o + 4, FLOOR + 2)
    f = Form(pv.w, pv.h, pv.ox, pv.oy)
    f.slab(x0o - 4, y1o - 4, x1o + 4, y1o + 3, 9, bevel=2, base=4)          # the coping
    f.slab(x0o - 2, y1o + 3, x1o + 2, FLOOR + 2, 6, bevel=1.5)              # its face
    lit(pv, f, tag=TAG_THING)
    for x in range(x0o + 5, x1o, 23):
        for y in range(y1o - 3, FLOOR + 2): c.put(x, y, 'deep')
    # the piers, leaning in: the opening's two sides, and what the arch over it stands on
    pier(c, V, x0o - 34, 34, 22, seed=2.1, root_side=-1)
    pier(c, V, x1o, 34, -22, seed=3.4, root_side=1)

    # ---- the window: a cell open on the same chamber; its lip, where they sit
    great_cell(c, WINDOW, seed=-0.4, teeth=9, stones=40, nodes=12, awake=(0, 6), mark=57)
    sx0, sx1, sy = SILL
    wcx, wcy, wr, _ = WINDOW
    wv = c.view(sx0 - 4, sy - 16, sx1 + 4, sy + 14)
    dd, _ = wv.polar(wcx, wcy)
    lipm = wv.rect(sx0 + 2, sy - 12, sx1 - 2, sy) & (dd < wr + 2)
    wv.clear_mask(lipm); wv.g[lipm] = -1
    f = Form(wv.w, wv.h, wv.ox, wv.oy)
    f.slab(sx0, sy, sx1, sy + 10, 10, bevel=2)
    f.slab(sx0 + 3, sy + 9, sx1 - 3, sy + 13, 6, bevel=1.5)
    lit(wv, f, tag=TAG_THING)
    rr = WINDOW[2] + WINDOW[3] - 2.5
    for side in (1, -1):                                                    # its ring's veins, down to the floor and in to the parapet
        pts = [(wcx + rr * math.cos(a), wcy + rr * math.sin(a))
               for a in np.linspace(-math.pi / 2, -math.pi / 2 + side * (math.pi - 0.05), 70)]
        if side < 0: V.add(c, pts + [(wcx - 4, FLOOR - 3), (x1o + 40, FLOOR - 3)], lit_every=6)
        else: V.add(c, pts, lit_every=6)

    # ---- the door, at the left end: its foot below the floor
    door(c, V)

    # ---- the stair: the tall ones' treads, grown by turns out of a rib at the right end;
    # under each a bracket back to the rib (the treads themselves are the map's stone)
    rib(c, SPINE - 8, 150, FLOOR + 6, 16, lean=0, root_side=1, seed=7.7, veins=V,
        vein_to=[(SPINE, FLOOR - 3), (x1o + 40, FLOOR - 3)])
    for x0_, top in STAIR:
        tx0, tx1, ty = x0_ * 8, x0_ * 8 + 24, top * 8 + 16
        near = tx1 if tx1 <= SPINE else tx0                               # the tread's end at the rib
        far = tx0 if tx1 <= SPINE else tx1
        pts = [(near, ty), (far + (6 if far < near else -6), ty), (near, ty + 18)]
        bv = c.view(min(near, far) - 4, ty - 4, max(near, far) + 4, ty + 26)
        f = Form(bv.w, bv.h, bv.ox, bv.oy)
        f.poly(pts, 8, bevel=2.5, base=8)
        lit(bv, f)

    # ---- the living: roots down out of the rock over the roof's edge, hanging into the
    # opening and gripping the piers; growth where the light comes in
    roots = [(Canvas.bez((x0o - 40, -4), (x0o - 30, 60), (x0o + 6, 90), (x0o + 2, 190), 50), 6.0, 2.5),
             (Canvas.bez((x0o + 30, -4), (x0o + 24, 30), (x0o + 44, 50), (x0o + 36, 96), 30), 3.6, 1.2),
             (Canvas.bez((x0o + 120, -4), (x0o + 118, 14), (x0o + 130, 22), (x0o + 124, 44), 18), 2.6, 1.0),
             (Canvas.bez((x1o - 70, -4), (x1o - 74, 26), (x1o - 58, 40), (x1o - 64, 78), 26), 3.2, 1.0),
             (Canvas.bez((x1o + 50, -4), (x1o + 36, 70), (x1o - 8, 110), (x1o - 2, 230), 56), 6.0, 2.5),
             (Canvas.bez((x1o - 150, -4), (x1o - 146, 10), (x1o - 158, 18), (x1o - 152, 34), 14), 2.2, 0.8)]
    for k, (pts, r0, r1) in enumerate(roots):
        c.root(pts, r0, r1, seed=40 + k, moss=0.5)
        for j in range(5, len(pts) - 2, 6):                               # rootlets, hanging
            x, y = pts[j]
            s_ = 1 if (j + k) % 2 else -1
            L = 5 + h2(j, k, 9) * 12
            c.stroke([(x, y), (x + 2 * s_, y + L * 0.5), (x + 3 * s_, y + L)], 0.8, 0.4, 'warmD')
    for k, x in enumerate(range(x0o + 10, x1o - 10, 17)):                  # vines hanging over the roof's edge
        if h2(k, 5, 1) < 0.45: continue
        L = 8 + h2(k, 5, 2) * 36
        c.vine([(x + math.sin(i * 0.25 + k) * 1.2, i) for i in range(int(L))], seed=120 + k, leaf=0.45, flower=0.04)
    c.moss(c.rect(x0o - 4, y1o - 5, x1o + 4, y1o - 3) & (noise(c, 3, 73) < 0.5), 0.6, 73)   # moss along the parapet
    for k, x in enumerate(range(x0o + 2, x1o, 5)):                         # and growth standing on it
        if h2(k, 9, 1) < 0.6: continue
        reeds(c, x, x + 3, y1o - 4, seed=200 + k, hmin=2, hmax=7)
    # the basin: weed standing up from its bed
    for k, x in enumerate(range(404, 776, 9)):
        L = 12 + h2(x, 5, 1) * 26
        c.vine([(x + 2.5 * math.sin(i * 0.2 + k), BED - i) for i in range(int(L))], seed=90 + k, leaf=0.5)
    # amber in the raw rock: seams of their own light, where the archive has not drunk them
    for k in range(22):
        x = h2(k, 3, 1) * W; y = h2(k, 3, 2) * H
        if (edgeL[min(H - 1, int(y)), min(W - 1, int(x))] < x < edgeR[min(H - 1, int(y)), min(W - 1, int(x))]): continue
        pts = [(x, y)]
        for i in range(int(4 + h2(k, 3, 3) * 8)):
            x += (h2(k, i, 4) - 0.5) * 6; y += 2 + h2(k, i, 5) * 3
            pts.append((x, y))
        for (ax, ay), (bx, by) in zip(pts, pts[1:]):
            n = int(max(abs(bx - ax), abs(by - ay))) + 1
            for i in range(n):
                qx, qy = ax + (bx - ax) * i / n, ay + (by - ay) * i / n
                c.put(qx, qy, 'warmD', TAG_WALL); c.glow(qx, qy, 'warm' if i % 3 else 'amber')

    # ---- the keeper, at the edge: the colossus (tools/art/colossus.py), on its seat
    col = colossus.paint(colossus.build(throne=False))
    rev = {tuple(int(u) for u in v): IDX[k] for k, v in PAL.items()}
    ox, oy = COLOSSUS_AT
    cm = np.zeros((H, W), bool)
    ys, xs = np.nonzero(col[..., 3])
    for y, x in zip(ys, xs):
        if 0 <= ox + x < W and 0 <= oy + y < H:
            p = col[y, x]
            c.a[oy + y, ox + x] = rev[(int(p[0]), int(p[1]), int(p[2]))]; c.t[oy + y, ox + x] = p[3]
            c.g[oy + y, ox + x] = -1
            cm[oy + y, ox + x] = True
    for (x, y, w, h, g) in ((438, 54, 5, 2, 'city'), (439, 53, 3, 1, 'city'), (439, 54, 2, 1, 'cityH'), (429, 57, 2, 1, 'coolM')):
        for j in range(h):
            for i in range(w): c.glow(x + i, y + j, g)                     # its eye: awake
    kv = c.view(288, 8, 608, 344)
    overgrow(kv, cm[8:344, 288:608], seed=13, density=0.45, hang=0.05, hang_len=9)
    reeds(c, 480, 584, 200, seed=17, hmin=3, hmax=11)                     # a bed on its lap
    for k in range(6):                                                     # and from its fingers, down toward the fire
        x, y = 295 + 3.8 * k, 242 + 3.6 * k
        L = 10 + h2(k, 7, 1) * 22
        c.vine([(x + math.sin(i * 0.3 + k) * 0.8, y + i) for i in range(int(L))], seed=70 + k, leaf=0.4, flower=0.06)
    # the terraces of the stair: each tread a bed, its growth standing up behind the nosing
    for x0_, top in STAIR:
        reeds(c, x0_ * 8 + 1, x0_ * 8 + 23, top * 8, seed=x0_ * 3 + top, hmin=3, hmax=10)
    return c, V


def preview_with_tiles(c, path, scale=2):
    """The picture under a flat light, the map's solid tiles over it half-dark: what shows."""
    import re
    src = open(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'src', 'antechamber.c')).read()
    rows = re.findall(r'^\s+"([^"]{120})",', src.split('ROOM_MAP')[1].split('};')[0], re.M)
    from PIL import Image
    a = c.rgba('a').astype(float); g = c.rgba('g')
    rgb = np.full((c.h, c.w, 3), (0, 0, 0), float)
    m = a[..., 3] > 0
    rgb[m] = a[m, :3] * 0.9
    mg = g[..., 3] > 0
    rgb[mg] = g[mg, :3]
    for ty, row in enumerate(rows):
        for tx, ch in enumerate(row):
            if ch in '#*kX':
                rgb[ty * 8:ty * 8 + 8, tx * 8:tx * 8 + 8] = rgb[ty * 8:ty * 8 + 8, tx * 8:tx * 8 + 8] * 0.25 + np.array((40, 30, 20)) * 0.75
            elif ch == '~':
                rgb[ty * 8:ty * 8 + 8, tx * 8:tx * 8 + 8] = rgb[ty * 8:ty * 8 + 8, tx * 8:tx * 8 + 8] * 0.7 + np.array((20, 60, 140)) * 0.3
            elif ch in '-x':
                rgb[ty * 8:ty * 8 + 3, tx * 8:tx * 8 + 8] = (200, 180, 120)
    for x in (320, 640): rgb[:, x] = (255, 0, 255)
    rgb[176, :] = (255, 0, 255)
    Image.fromarray(rgb.clip(0, 255).astype(np.uint8)).resize((c.w * scale, c.h * scale), Image.NEAREST).save(path)


if __name__ == '__main__':
    root = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'art')
    c, V = build()
    c.save(os.path.join(root, 'archive'))
    V.write(os.path.join(root, 'archive.veins'))
    if '--preview' in sys.argv:
        d = sys.argv[sys.argv.index('--preview') + 1]
        c.preview(os.path.join(d, 'archive.png'), scale=2)
        preview_with_tiles(c, os.path.join(d, 'archive_tiles.png'))
    print('archive:', len(V.paths), 'veins')
