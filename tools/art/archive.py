#!/usr/bin/env python3
"""The archive: the far wall of the whole antechamber, all six screens, as one piece of
architecture made of the building's parts (tools/art/kit.py, claude/ARCHIVE.md).

    A the intake   | B the keeper    | C the stacks window
    D seed vault   | E the garden    | F the heart

One picture, 960 x 352, in room px. A is bay A (tools/art/bay_a.py). The hall -- B, C, E
and F -- is one volume and one wall: the stacks in their courses from the roof to under the
water, the index band at the height of the walk, and on it three cells at the building's
own scale. The keeper's, which the colossus sits in, its hand out of it. The window's, its
iris drawn back, open on more of the archive. The heart's, down in the water, whose iris is
a grate of bars. Between the first two a pier stands in the basin and splits into two ribs.
D is the seed vault, cut into the rock under the intake: drawers the size of a seed, in
their thousands, asleep. Every vein in the building runs down to the heart.

    tools/art/archive.py [--preview DIR]    write art/archive.png, _glow.png, .veins
"""
import math, os, sys
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from sculpt import Form, PAL, TAG_WALL, TAG_THING, TAG_STONE
from paint import Canvas, h2, IDX
from kit import cell, seed_cell, stacks, rib, rock, noise, Veins, lit, arc_marks
import bay_a, colossus

W, H = 960, 352
TAU = math.tau
# The three great cells: centre, opening radius, ring. The window's and the heart's are the
# map's F_WINDOW (86, 1, 22, 22) and F_GRILLE (84, 26, 12, 12): circles in those squares.
KEEPER = (488.0, 172.0, 144.0, 14.0)
WINDOW = (776.0, 96.0, 88.0, 12.0)
HEART = (720.0, 256.0, 48.0, 12.0)
FLOOR, WATER, BED = 280, 288, 344     # the undercroft's floor and the paving; the basin's top and floor
COLOSSUS_AT = (288, 8)                # tools/art/colossus.py: its canvas's top-left in the room


def warmth(x, y):
    """1 at the heart, falling away from it: the building is a gradient round its heart."""
    return max(0.0, 1.0 - math.hypot(x - HEART[0], y - HEART[1]) / 520.0)


def hall_grade(x, y):
    w = warmth(x, y)
    a = 0.03 + 0.28 * w * w
    return a, a + 0.30 + 0.30 * w


def vault_grade(x, y):
    """The seed vault is the sleeping tier: almost nothing awake, almost nothing lost."""
    return 0.012, 0.74 - 0.25 * max(0.0, (140 - x) / 140.0)


# ------------------------------------------------------------------------------ the great cells
def great_cell(c, C, seed=0.0, hole=False, teeth=9, stones=40, nodes=12, awake=(0,), mark=1):
    """A cell at the building's scale: the moulded ring in its stones with the catalogue cut
    in them, glass set in it, and the iris drawn back into the ring so only the tips of its
    blades show, a ratchet round the opening. Inside, the recess -- or, if `hole`, nothing:
    the picture is cut and what is beyond shows (city.c)."""
    cx, cy, r, ring = C
    R = r + ring
    v = c.view(cx - R - 2, cy - R - 2, cx + R + 3, cy + R + 3)
    d, ang = v.polar(cx, cy)
    rim = (d >= r) & (d < R)
    t = (d - r) / ring
    f = Form(v.w, v.h, v.ox, v.oy)
    f.z = np.where(rim, 20 + 6 * np.sqrt(np.clip(1 - (2 * t - 1) ** 2, 0, 1))
                   - 3.0 * ((t > 0.30) & (t < 0.36)) - 3.0 * ((t > 0.64) & (t < 0.70)), -1e9)
    lit(v, f, where=rim)
    for k in range(stones):
        a = seed + k * TAU / stones
        for rr in np.arange(r, R, 0.5): v.put(cx + rr * math.cos(a), cy + rr * math.sin(a), 'deep')
        arc_marks(v, cx, cy, r + ring * 0.2, a + 0.02, a + TAU / stones - 0.02, k * 7 + mark)
        arc_marks(v, cx, cy, r + ring * 0.52, a + 0.02, a + TAU / stones - 0.02, k * 7 + mark + 1)
        arc_marks(v, cx, cy, r + ring * 0.84, a + 0.02, a + TAU / stones - 0.02, k * 7 + mark + 2)
    for k in range(nodes):
        a = -math.pi / 2 + k * TAU / nodes
        x, y = cx + (r + ring * 0.5) * math.cos(a) - 0.5, cy + (r + ring * 0.5) * math.sin(a) - 0.5
        g = 'city' if k in awake else ('coolM' if h2(k, mark, 3) < 0.45 else 'coolD')
        for dx, dy in ((0, 0), (1, 0), (0, 1), (1, 1)):
            v.put(x + dx, y + dy, 'deep'); v.glow(x + dx, y + dy, g)
        v.put(x - 1, y - 1, 'stoneH'); v.put(x + 2, y + 2, 'deep')
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
    if hole:
        v.clear_mask(open_); v.g[open_] = -1
    else:
        v.fill_mask(open_, 'deep', TAG_WALL)
        # the recess: shallow courses round it, the way the iris would close
        for rr in np.arange(r - 26, 12, -22):
            v.fill_mask(open_ & (np.abs(d - rr) < 0.6), 'dark', TAG_WALL)
            v.fill_mask(open_ & (np.abs(d - rr - 1.2) < 0.6), 'deep', TAG_WALL)
        for k in range(teeth * 2):
            a = seed + k * span / 2
            for rr in np.arange(18, r - 12, 0.7):
                if int(rr / 22) % 2 == k % 2:
                    v.put(cx + rr * math.cos(a + rr * 0.004), cy + rr * math.sin(a + rr * 0.004), 'dark', TAG_WALL)
    return v, d, ang


def ring_veins(V, c, C, bottom_to, seed=1):
    """The ring's own veins: from its top, down each side, to its foot, and on."""
    cx, cy, r, ring = C
    rr = r + ring - 2.5
    for side in (1, -1):
        pts = [(cx + rr * math.cos(a), cy + rr * math.sin(a))
               for a in np.linspace(-math.pi / 2, -math.pi / 2 + side * (math.pi - 0.04), int(rr * 0.8))]
        V.add(c, pts + bottom_to, lit_every=6)


def iris_of_bars(c, C, n=11, hub=7.0, seed=0.3):
    """The heart's iris: bars, not blades -- a grate you can see through, curving in to a
    hub where the lens would be."""
    cx, cy, r, ring = C
    for i in range(n):
        a0 = seed + i * TAU / n
        pts = []
        for k in range(40):
            t = k / 39
            rr = r + 1 - (r + 1 - hub) * t
            pts.append((cx + rr * math.cos(a0 + 1.25 * t * t), cy + rr * math.sin(a0 + 1.25 * t * t)))
        c.stroke(pts, 1.4, 1.0, 'deep', tag=TAG_WALL, shade=('dark', 'void'))      # a silhouette on the light
    for j in range(-int(hub) - 2, int(hub) + 3):
        for i in range(-int(hub) - 2, int(hub) + 3):
            dd = math.hypot(i + 0.5, j + 0.5)
            if dd < hub + 1.5:
                c.put(cx + i, cy + j, 'stoneL' if (i + j) < -2 else ('stone' if dd > hub - 1 else 'dark'), TAG_STONE)
            if dd < hub - 2.5: c.put(cx + i, cy + j, 'deep', TAG_STONE)


# ------------------------------------------------------------------------------ the vault
def drawers(c, x0, y0, x1, y1, seed, grade, pier=56):
    """The seed vault's wall: the smallest cells, packed like comb, in cabinets between short
    piers, a course of stone every third row. Each a seed's drawer, and each lit as its
    state: most asleep."""
    m = c.rect(x0, y0, x1, y1)
    c.fill_mask(m, 'deep', TAG_WALL)
    row, y = 0, y0
    while y < y1:
        if row % 4 == 3:                                                  # a course of stone
            for x in range(x0, x1):
                c.put(x, y, 'stone', TAG_WALL); c.put(x, y + 1, 'dark', TAG_WALL)
                if (x * 7 + row) % 5 == 0: c.put(x, y + 2, 'dark', TAG_WALL)
            y += 4; row += 1
            continue
        off = 4 if row % 2 else 0
        for x in range(x0 - 8 + off, x1, 8):
            if ((x - x0) % pier) > pier - 9: continue
            a, s = grade(x, y)
            v = h2(x, y, seed)
            seed_cell(c, x, y, 'living' if v < a else ('asleep' if v < s else 'dark'))
        y += 7; row += 1
    for x in range(x0 + pier - 8, x1, pier):                               # the piers
        f = Form(12, y1 - y0 + 4, x - 2, y0 - 2)
        f.slab(x - 1, y0 - 2, x + 8, y1 + 2, 6, bevel=2)
        for yb in range(y0 + 4, y1, 14): f.slab(x - 2, yb, x + 9, yb + 3, 4, bevel=1, base=5)
        cv = c.view(x - 2, y0 - 2, x + 10, y1 + 2)
        lit(cv, f, tag=TAG_WALL)


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
    everything = np.ones((H, W), bool)
    rock(c, everything, seed=41)                                           # the mountain, under all of it

    # ---- A, the intake
    bay_a.build(c.view(0, 0, 320, 176), V)

    # ---- D, the seed vault: cut into the rock under the intake
    drawers(c, 0, 168, 320, FLOOR + 2, seed=5, grade=vault_grade)
    V.add(c, [(200, 176), (200, 214), (204, 272), (212, 277), (320, 277), (400, 277), (404, 292),
              (430, 338), (440, 340), (700, 340), (714, 318)], lit_every=6)

    # ---- the hall: one wall from the roof to under the water
    stacks(c, 320, 2, W, BED + 2, seed=7, grade=hall_grade, pier=48)
    # the index band runs on from A across the threshold, and into the keeper's cell's ring
    f = Form(56, 50, 312, 68)
    f.slab(312, 74, 368, 108, 8, bevel=2)
    f.slab(310, 70, 368, 76, 10, bevel=1.5, base=2)
    f.slab(310, 106, 368, 111, 10, bevel=1.5, base=2)
    band = c.view(312, 68, 368, 118)
    lit(band, f, tag=TAG_WALL)
    for x in range(313, 368, 6):
        band.fill_mask(band.rect(x, 111, x + 4, 115), 'stone', TAG_WALL); band.fill_mask(band.rect(x, 114, x + 4, 115), 'deep', TAG_WALL)

    # ---- the three great cells, the pier and the ribs
    great_cell(c, KEEPER, seed=0.21, teeth=11, stones=56, nodes=16, awake=(0, 4, 12), mark=31)
    ring_veins(V, c, KEEPER, [(488, 340), (520, 340), (700, 340), (714, 318)])
    great_cell(c, WINDOW, seed=-0.4, hole=True, teeth=9, stones=40, nodes=12, awake=(0, 6), mark=57)
    ring_veins(V, c, WINDOW, [(760, 197), (736, 197), (722, 196)])
    # the pier between them, standing in the water, parting into two ribs
    rib(c, 624, -40, 298, 18, lean=-30, root_side=-1, seed=4.1, veins=V, vein_to=[(633, 293), (651, 293), (668, 288)])
    rib(c, 642, -40, 298, 18, lean=30, root_side=1, seed=5.3, veins=V, vein_to=[(651, 293), (668, 288)])
    # and the rib on the stair, framing the window from the right
    rib(c, 900, -40, 154, 16, lean=-18, root_side=-1, seed=6.2, veins=V,
        vein_to=[(900, 146), (786, 258), (781, 257)])
    # the heart: its cell in the water, its iris a grate, and every vein ending at it
    great_cell(c, HEART, seed=0.9, hole=True, teeth=0, stones=28, nodes=14, awake=tuple(range(14)), mark=83)
    iris_of_bars(c, HEART, n=11, hub=7.0, seed=0.3)
    cx, cy, r, ring = HEART
    rr = r + ring - 2.5
    V.add(c, [(cx + rr * math.cos(a), cy + rr * math.sin(a)) for a in np.linspace(0.2, TAU + 0.2, 90)], lit_every=4)

    # ---- the living, on the building
    # roots: down from the roof and over the great cells, hugging their rings, into the water
    def arc(C, a0, a1, off=2.0, seed=0):
        cx, cy, r, ring = C
        R = r + ring + off
        n = max(4, int(abs(a1 - a0) * R / 3))
        return [(cx + (R + 1.2 * math.sin(i * 0.9 + seed)) * math.cos(a), cy + (R + 1.2 * math.sin(i * 0.9 + seed)) * math.sin(a))
                for i, a in enumerate(np.linspace(a0, a1, n))]
    D2R = math.pi / 180
    roots = [([(548, -2), (546, 10)] + arc(KEEPER, -72 * D2R, -250 * D2R, 2.5, 1), 5.5, 2.0),
             ([(672, -2), (684, 30)] + arc(WINDOW, -160 * D2R, -245 * D2R, 2.0, 2)
              + arc(HEART, -115 * D2R, -250 * D2R, 2.0, 3), 4.5, 1.8),
             ([(866, -2), (858, 26)] + arc(WINDOW, -32 * D2R, 72 * D2R, 2.0, 4)
              + arc(HEART, -42 * D2R, 62 * D2R, 2.0, 5), 3.8, 1.5)]
    for k, (pts, r0, r1) in enumerate(roots):
        c.root(pts, r0, r1, seed=40 + k, moss=0.5)
        for j in range(6, len(pts) - 3, 7):                                # rootlets, hanging
            x, y = pts[j]
            s_ = 1 if (j + k) % 2 else -1
            L = 6 + h2(j, k, 9) * 10
            c.stroke([(x, y), (x + 2 * s_, y + L * 0.5), (x + 3 * s_, y + L)], 0.8, 0.4, 'warmD')
    # growth pouring from the heart's ring, hanging over the water
    spill(c, 700, 198, 4, seed=3, length=24)
    spill(c, 742, 200, 3, seed=4, length=20)
    spill(c, 612, 208, 3, seed=5, length=40)
    # moss where the light is: on the heart's ring and round the basin's edge
    d, _ = c.polar(HEART[0], HEART[1])
    c.moss((d > HEART[2] + 4) & (d < HEART[2] + HEART[3] + 3) & (c.Y < HEART[1] - 20) & (noise(c, 3, 71) < 0.5), 0.55, 71)
    c.moss((np.abs(c.Y - WATER + 2) < 3) & (c.X > 320) & (noise(c, 4, 73) < 0.45), 0.5, 73)

    # E, the garden: the basin grown -- weed standing up from its bed among the drowned stacks,
    # and over its edge a cell awake with growth pouring from it
    for k, x in enumerate(range(404, 660, 9)):
        L = 16 + h2(x, 5, 1) * 34
        pts = [(x + 2.5 * math.sin(i * 0.2 + k) + (h2(x, 5, 2) - 0.5) * i * 0.2, BED - i) for i in range(int(L))]
        c.vine(pts, seed=90 + k, leaf=0.5)
    cell(c, 432, 266, 9, 'living', seed=0.5)
    spill(c, 432, 272, 4, seed=11, length=16, flower=0.12)

    # ---- the keeper, in its cell: the colossus (tools/art/colossus.py), on a pier cut short
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
            for i in range(w): c.glow(x + i, y + j, g)                     # its eye: awake, the one lens in it
    # grown over: moss on every top, strands from its arm, a bed on its lap
    kv = c.view(288, 8, 608, 344)
    overgrow(kv, cm[8:344, 288:608], seed=13, density=0.45, hang=0.05, hang_len=9)
    reeds(c, 480, 584, 200, seed=17, hmin=3, hmax=11)
    for k in range(6):                                                     # and from its fingers, to the floor
        x, y = 295 + 3.8 * k, 242 + 3.6 * k
        L = 12 + h2(k, 7, 1) * (FLOOR - y - 12)
        c.vine([(x + math.sin(i * 0.3 + k) * 0.8, y + i) for i in range(int(L))], seed=70 + k, leaf=0.4, flower=0.06)
    reeds(c, 400, 458, WATER, seed=19, hmin=4, hmax=14)                    # the basin's edge
    reeds(c, 604, 626, WATER, seed=23, hmin=4, hmax=12)
    # the terraces of the stair: each tread a bed, its growth standing up behind the nosing
    for x0, top in ((97, 34), (100, 31), (103, 28), (106, 25), (109, 22), (112, 19), (115, 16)):
        reeds(c, x0 * 8 + 1, x0 * 8 + 23, top * 8, seed=x0, hmin=3, hmax=10)
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
