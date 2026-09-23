#!/usr/bin/env python3
"""The archive: the far wall of the whole antechamber, all six screens, as one piece of
architecture made of the building's parts (tools/art/kit.py, claude/ARCHIVE.md).

The hall is a cell, the size of the hall. Its lens is the heart: a round grate at the
centre of the six screens, green light beyond it, and the keeper sitting before it with
its hand out toward the door. Its iris is drawn all the way back into five spokes that
run out from the heart's ring to the rim, each carrying its vein in toward the heart. Its
ring is the rim, a circle through all six screens, cut into the raw rock of the mountain;
inside it the collection in its courses, the seed drawers in the outermost band, awake
toward the heart and dark toward the rim. On the rim, at the two ends of one diameter
through the heart, the door (low left, where you wake) and the window (high right, open
on more of the archive). Roots come down out of the rock, along the rim and the spokes,
to the heart.

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
# The heart's and the window's openings are the map's F_GRILLE (43, 3, 36, 36) and F_WINDOW
# (94, -1, 21, 21): circles in those squares of tiles. (centre x, y, opening radius, ring)
HEART = (488.0, 168.0, 144.0, 14.0)
WINDOW = (836.0, 76.0, 84.0, 12.0)
DOOR = (136.0, 248.0, 96.0, 16.0)       # its crown on row 17, where the hunters' plank lies
RIM = 361.0                             # the wheel's rim: through the door's centre and the window's
SPOKES = (168, 208, 318, 344, 18)       # degrees (y down): to the door, up-left, up-right, to the window, down-right
SEEDBAND = 312.0                        # inside the rim from here out: the seed drawers
FLOOR, BED = 304, 344                   # the floor and the basin's surface; the basin's floor
SILL = (800, 872, 160)                  # the window's lip: x0, x1, its top (row 20)
MURAL = (196, 312, 96)                  # the index band over the door, where hall.c carves the mural
COLOSSUS_AT = (288, 8)                  # tools/art/colossus.py: its canvas's top-left in the room
STAIR = ((108, 35), (112, 32), (108, 29), (112, 26), (108, 23))   # the map's treads (tile x, top row), 3 x 2
SPINE = 888                             # the stair's rib, between its two files of treads


def hub(r, deg):
    return HEART[0] + r * math.cos(deg * D2R), HEART[1] + r * math.sin(deg * D2R)


def grade(x, y):
    """The odds of a kept thing being awake, or at least asleep, by its place on the wheel:
    awake toward the heart, dark toward the rim."""
    d = math.hypot(x - HEART[0], y - HEART[1])
    w = min(1.0, max(0.0, 1.0 - (d - 160.0) / (RIM - 160.0)))
    a = 0.02 + 0.26 * w * w
    return a, a + 0.24 + 0.38 * w


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


def iris_of_bars(c, C, n=7, hub_r=9.0, seed=0.3, below=1e9):
    """The heart's iris: bars, not blades -- a grate you see through, curving in to a hub
    where the lens would be. A silhouette on the light."""
    cx, cy, r, ring = C
    for i in range(n):
        a0 = seed + i * TAU / n
        pts = []
        for k in range(60):
            t = k / 59
            rr = r + 1 - (r + 1 - hub_r) * t
            pts.append((cx + rr * math.cos(a0 + 1.1 * t * t), cy + rr * math.sin(a0 + 1.1 * t * t)))
        c.stroke([p for p in pts if p[1] < below + 4], 1.6, 1.1, 'deep', tag=TAG_WALL, shade=('dark', 'void'))
    for j in range(-int(hub_r) - 2, int(hub_r) + 3):
        for i in range(-int(hub_r) - 2, int(hub_r) + 3):
            dd = math.hypot(i + 0.5, j + 0.5)
            if dd < hub_r + 1.5:
                c.put(cx + i, cy + j, 'stoneL' if (i + j) < -3 else ('stone' if dd > hub_r - 1.5 else 'dark'), TAG_STONE)
            if dd < hub_r - 3.5: c.put(cx + i, cy + j, 'deep', TAG_STONE)


# ------------------------------------------------------------------------------ spokes
def spoke(c, V, deg, r0, r1, w=18.0, seed=0.0, root_side=1, band=18):
    """A rib laid along a radius of the wheel, from the heart's ring out to the rim: three
    shafts bound in bands, a root wound round it, and in a channel down its face a vein
    that runs in, to the heart, glass at every band."""
    ux, uy = math.cos(deg * D2R), math.sin(deg * D2R)
    px, py = -uy, ux
    x0, y0 = hub(r0, deg); x1, y1 = hub(r1, deg)
    m = w / 2 + 6
    v = c.view(min(x0, x1) - m, min(y0, y1) - m, max(x0, x1) + m, max(y0, y1) + m)
    if v.w <= 0 or v.h <= 0: return
    f = Form(v.w, v.h, v.ox, v.oy)
    for off in (-w / 3, 0.0, w / 3):
        f.capsule(x0 + off * px, y0 + off * py, x1 + off * px, y1 + off * py, w / 5.2,
                  depth=w / 5.2 + (2 if off == 0 else 0), base=10)
    L = r1 - r0
    for s in np.arange(10, L - 4, band):                                   # the bands
        bx, by = x0 + s * ux, y0 + s * uy
        f.capsule(bx - (w / 2 + 1) * px, by - (w / 2 + 1) * py, bx + (w / 2 + 1) * px, by + (w / 2 + 1) * py,
                  2.3, depth=5, base=14)
    for (ex, ey) in ((x0, y0), (x1, y1)):                                  # its feet, where it meets the rings
        f.capsule(ex - (w / 2 + 3) * px, ey - (w / 2 + 3) * py, ex + (w / 2 + 3) * px, ey + (w / 2 + 3) * py,
                  4.0, depth=7, base=12)
    lit(v, f)
    pts = [(x1 - s * ux + 0.5 * px, y1 - s * uy + 0.5 * py) for s in np.arange(0, L, 2)]
    V.add(c, pts, lit_every=6)
    for s in np.arange(10, L - 4, band):
        bx, by = x0 + s * ux, y0 + s * uy
        c.glow(bx, by, 'cityH'); c.glow(bx + ux, by + uy, 'city'); c.glow(bx - ux, by - uy, 'city')
    # the root, wound round it: seen where it crosses in front, its shadow where it goes behind
    for k in range(int(L * 10)):
        s = k * 0.1
        ph = s * 0.05 + seed
        sn = math.sin(ph)
        cx = x0 + s * ux + root_side * sn * (w / 2 + 1.5) * px
        cy = y0 + s * uy + root_side * sn * (w / 2 + 1.5) * py
        r = 1.6 + 1.4 * (s / L)                                            # thicker toward the rim, where it came in
        if math.cos(ph) * root_side > -0.15:
            for j in range(-3, 4):
                for i in range(-3, 4):
                    if i * i + j * j <= r * r:
                        c.put(cx + i, cy + j, 'warm' if (i + j) < -0.5 else ('warmD' if (i + j) < 1.5 else 'deep'))
            if h2(k, int(seed * 10), 5) < 0.015: c.put(cx - 1, cy - r - 0.5, 'moss')


# ------------------------------------------------------------------------------ the door
def door(c, V):
    """The largest cell you will stand before: its ring cut with the catalogue, nine blades
    closed on a lens that is awake and lights them from within. It stands on the rim with
    its foot sunk below the floor."""
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
    # its ring's vein: round the top from both sides, and out along the spoke to the heart
    rr = DR + RING - 2.5
    x_, y_ = hub(RIM - DR - RING, 168)
    for side in (1, -1):
        pts = [(DX + rr * math.cos(a), DY + rr * math.sin(a))
               for a in np.linspace(math.pi + side * 0.0, -0.1 if side > 0 else -math.pi * 0.95, 60)]
        V.add(c, pts, lit_every=6)


# ------------------------------------------------------------------------------ the vault's band
def drawers(c, m, seed, grade):
    """The seed drawers: the smallest cells, packed like comb, where m says. Each a seed's
    drawer, lit as its state."""
    ys, xs = np.nonzero(m)
    if not len(xs): return
    x0, x1, y0, y1 = xs.min() + c.ox, xs.max() + c.ox + 1, ys.min() + c.oy, ys.max() + c.oy + 1
    c.fill_mask(m, 'deep', TAG_WALL)
    row = 0
    for y in range(int(y0) - 7, int(y1), 7):
        off = 4 if row % 2 else 0
        for x in range(int(x0) - 8 + off, int(x1), 8):
            cx, cy = x + 3 - c.ox, y + 3 - c.oy
            if not (0 <= cx < c.w and 0 <= cy < c.h and m[cy, cx]): continue
            a, s = grade(x, y)
            v = h2(x, y, seed)
            seed_cell(c, x, y, 'living' if v < a * 0.5 else ('asleep' if v < s else 'dark'))
        row += 1


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
def build():
    c = Canvas(W, H)
    V = Veins()
    rock(c, np.ones((H, W), bool), seed=41)                                # the mountain, under all of it
    d_hub, a_hub = c.polar(HEART[0], HEART[1])

    # ---- the wheel's body: the collection in its courses, inside the rim
    inside = d_hub < RIM - 8
    stacks(c, 0, 2, W, BED + 2, seed=7, grade=grade, pier=48, where=inside & (d_hub < SEEDBAND - 3))
    drawers(c, inside & (d_hub >= SEEDBAND + 3), seed=5, grade=grade)
    moulded_ring(c, HEART[0], HEART[1], SEEDBAND - 3, 6, seed=0.05, stones=160, nodes=0, mark=11, gain=0.8)
    # the rim: the wheel's own ring, where the archive is cut into the rock
    moulded_ring(c, HEART[0], HEART[1], RIM - 8, 16, seed=0.02, stones=144, nodes=48,
                 awake=(0, 12, 24, 36), mark=13)
    # the rock just outside it, dressed back from the ring
    c.fill_mask((d_hub >= RIM + 8) & (d_hub < RIM + 10), 'deep', TAG_WALL)

    # the index band over the door, where the mural is carved: a panel let into the stacks,
    # its face plain and dark so the carving and the phosphor read, its frame moulded
    x0, x1, y0 = MURAL
    pv = c.view(x0 - 6, y0 - 8, x1 + 6, y0 + 38)
    f = Form(pv.w, pv.h, pv.ox, pv.oy)
    f.slab(x0 - 5, y0 - 7, x1 + 5, y0 + 37, 6, bevel=2)                   # the frame
    f.slab(x0, y0, x1, y0 + 30, 3, bevel=1, base=-2)                       # the face, set back
    lit(pv, f, tag=TAG_WALL, gain=0.8)
    face = pv.rect(x0 + 1, y0 + 1, x1 - 1, y0 + 29)
    pv.fill_mask(face, 'dark', TAG_WALL)
    pv.fill_mask(face & (noise(pv, 3, 61) > 0.62), 'deep', TAG_WALL)
    for x in (x0 - 5, x1 + 1):                                              # its ends: two cells asleep
        seed_cell(c, x, y0 + 12, 'asleep')

    # ---- the spokes: the iris drawn all the way back, each a rib from the heart to the rim
    for k, deg in enumerate(SPOKES):
        spoke(c, V, deg, HEART[2] + HEART[3] + 2, RIM - 8, w=18, seed=k * 1.3, root_side=1 if k % 2 else -1)

    # ---- the heart: the wheel's lens, a grate, green beyond; under the water, drowned
    great_cell(c, HEART, seed=0.21, teeth=11, stones=56, nodes=16, awake=tuple(range(16)), mark=31, hole_above=FLOOR)
    iris_of_bars(c, HEART, n=7, hub_r=9.0, seed=0.5, below=FLOOR)
    cx, cy, r, ring = HEART
    V.add(c, [(cx + (r + ring - 2.5) * math.cos(a), cy + (r + ring - 2.5) * math.sin(a))
              for a in np.linspace(-math.pi / 2, math.pi * 1.5, 220)], lit_every=4)

    # ---- the window: on the rim, open on more of the archive; its lip, where they sit
    great_cell(c, WINDOW, seed=-0.4, teeth=9, stones=40, nodes=12, awake=(0, 6), mark=57)
    sx0, sx1, sy = SILL
    wcx, wcy, wr, _ = WINDOW
    wv = c.view(sx0 - 4, sy - 16, sx1 + 4, sy + 14)
    dd, _ = wv.polar(wcx, wcy)
    wv.clear_mask(wv.rect(sx0 + 2, sy - 12, sx1 - 2, sy) & (dd < wr + 2)); wv.g[wv.rect(sx0 + 2, sy - 12, sx1 - 2, sy) & (dd < wr + 2)] = -1
    f = Form(wv.w, wv.h, wv.ox, wv.oy)
    f.slab(sx0, sy, sx1, sy + 10, 10, bevel=2)
    f.slab(sx0 + 3, sy + 9, sx1 - 3, sy + 13, 6, bevel=1.5)
    lit(wv, f, tag=TAG_THING)

    # ---- the door: on the rim at the far end of the diameter, its foot below the floor
    door(c, V)

    # ---- the stair: the tall ones' treads, grown by turns out of a rib standing outside the
    # rim; under each a bracket back to the rib (the treads themselves are the map's stone)
    rib(c, SPINE - 8, 150, FLOOR + 6, 16, lean=0, root_side=1, seed=7.7, veins=V,
        vein_to=[(SPINE, FLOOR - 2), (836, FLOOR - 2), hub(HEART[2] + HEART[3], 12)])
    for x0_, top in STAIR:
        tx0, tx1, ty = x0_ * 8, x0_ * 8 + 24, top * 8 + 16
        near = tx1 if tx1 <= SPINE else tx0                               # the tread's end at the rib
        far = tx0 if tx1 <= SPINE else tx1
        pts = [(near, ty), (far + (6 if far < near else -6), ty), (near, ty + 18)]
        bv = c.view(min(near, far) - 4, ty - 4, max(near, far) + 4, ty + 26)
        f = Form(bv.w, bv.h, bv.ox, bv.oy)
        f.poly(pts, 8, bevel=2.5, base=8)
        lit(bv, f)

    # ---- the living, on the building: roots down out of the rock along the rim and the
    # spokes to the heart, gripping its ring; growth where it is warm
    def arc(cx, cy, R, a0, a1, wob=1.2, seed=0):
        n = max(4, int(abs(a1 - a0) * D2R * R / 3))
        return [(cx + (R + wob * math.sin(i * 0.9 + seed)) * math.cos(a * D2R),
                 cy + (R + wob * math.sin(i * 0.9 + seed)) * math.sin(a * D2R))
                for i, a in enumerate(np.linspace(a0, a1, n))]
    hx, hy = HEART[0], HEART[1]
    roots = [(arc(hx, hy, RIM + 11, 244, 196, seed=1), 6.0, 3.0),        # over the rim, down to the door
             (arc(hx, hy, RIM - 12, 2, 24, seed=3), 3.5, 2.0),           # from the window down the rim's inside, to the water
             (arc(hx, hy, HEART[2] + HEART[3] + 3, 250, 150, seed=4), 4.0, 2.0),   # hugging the heart's ring
             (arc(hx, hy, HEART[2] + HEART[3] + 3, 290, 380, seed=5), 4.0, 2.0)]
    for k, (pts, r0, r1) in enumerate(roots):
        c.root(pts, r0, r1, seed=40 + k, moss=0.5)
        for j in range(6, len(pts) - 3, 7):                               # rootlets, hanging
            x, y = pts[j]
            s_ = 1 if (j + k) % 2 else -1
            L = 5 + h2(j, k, 9) * 10
            c.stroke([(x, y), (x + 2 * s_, y + L * 0.5), (x + 3 * s_, y + L)], 0.8, 0.4, 'warmD')
    d, _ = c.polar(hx, hy)
    c.moss((d > HEART[2] + 4) & (d < HEART[2] + HEART[3] + 3) & (noise(c, 3, 71) < 0.42), 0.55, 71)
    c.moss((np.abs(c.Y - FLOOR + 2) < 3) & (c.X > 330) & (noise(c, 4, 73) < 0.45), 0.5, 73)
    # the basin: weed standing up from its bed; the heart's ring pouring growth into it
    for k, x in enumerate(range(404, 776, 9)):
        L = 12 + h2(x, 5, 1) * 26
        c.vine([(x + 2.5 * math.sin(i * 0.2 + k), BED - i) for i in range(int(L))], seed=90 + k, leaf=0.5)
    spill(c, 356, 236, 3, seed=5, length=40)
    spill(c, 626, 236, 3, seed=6, length=44)
    # amber in the raw rock: seams of their own light, where the archive has not drunk them
    for k in range(18):
        x = h2(k, 3, 1) * W; y = h2(k, 3, 2) * H
        if math.hypot(x - hx, y - hy) < RIM + 20: continue
        pts = [(x, y)]
        for i in range(int(4 + h2(k, 3, 3) * 8)):
            x += (h2(k, i, 4) - 0.5) * 6; y += 2 + h2(k, i, 5) * 3
            pts.append((x, y))
        for (ax, ay), (bx, by) in zip(pts, pts[1:]):
            n = int(max(abs(bx - ax), abs(by - ay))) + 1
            for i in range(n):
                qx, qy = ax + (bx - ax) * i / n, ay + (by - ay) * i / n
                c.put(qx, qy, 'warmD', TAG_WALL); c.glow(qx, qy, 'warm' if i % 3 else 'amber')

    # ---- the keeper, before the heart: the colossus (tools/art/colossus.py), on a pier cut short
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
