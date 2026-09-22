#!/usr/bin/env python3
"""The door to the temple, in four designs for choosing between. Each is a picture 176 x 168
px, set in the far wall of the first screen with its foot on the floor: fifteen times your
height. The theme: a living archive -- the tall ones' making, which keeps what touches it,
grown through with living things.

    tools/art/doors.py             write art/door_<name>.png and art/door_<name>_glow.png
    tools/art/doors.py --preview   also a flat-lit preview of each in the scratchpad dir given

  iris       a closed aperture of leaf-blades, veined with light; its ring inscribed
  heartwood  a trunk's cross-section grown as the door: rings as records, roots as cables
  stacks     a portal in a cliff of archive niches, records lit and dark, vines through them
  engine     geared rings round a stone flower, pipes into the floor, flowers in the gears
"""
import math, os, sys
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from sculpt import Form, shade, TAG_WALL, TAG_THING, TAG_STONE
from paint import Canvas, h2, IDX, shade_ramp

W, H = 176, 168
CX, CY = 88.0, 84.0
STONE = ['deep', 'dark', 'stone', 'stoneL', 'stoneH']


def lit_form(c, form, ramp=STONE, light=(-0.45, -0.8, 0.6), gain=0.95, dither=0.12, tag=TAG_THING, where=None):
    b, m = shade(form, light=light)
    if where is not None: m = m & where
    q = shade_ramp(b * gain, ramp, dither)
    for i, n in enumerate(ramp):
        c.fill_mask(m & (q == i), n, tag)
    return m


def arc_marks(c, cx, cy, r, a0, a1, seed, name='dark', step=1.0):
    """A band of inscription along an arc: clusters of short ticks, never words."""
    n = int(abs(a1 - a0) * r / step)
    for k in range(n):
        a = a0 + (a1 - a0) * k / n
        v = h2(k, seed)
        if v < 0.28: continue                      # the gaps between clusters
        x, y = cx + r * math.cos(a), cy + r * math.sin(a)
        c.put(x, y, name)
        if v > 0.8: c.put(cx + (r - 1) * math.cos(a), cy + (r - 1) * math.sin(a), name)


def lens(c, x, y, r=2.5, glow='coolM', core='city', frame='stoneL'):
    for j in range(-4, 5):
        for i in range(-4, 5):
            d = math.hypot(i, j)
            if d < r: c.glow(x + i, y + j, core if d < r * 0.5 else glow); c.put(x + i, y + j, 'deep')
            elif d < r + 1.2: c.put(x + i, y + j, frame)


# ------------------------------------------------------------------------------ iris
def iris():
    c = Canvas(W, H)
    R0, R1 = 70.0, 83.5
    # the ring: rounded in section, lit from above
    f = Form(W, H)
    d, ang = c.polar(CX, CY)
    rm, hw = (R0 + R1) / 2, (R1 - R0) / 2
    m = (d > R0) & (d < R1)
    f.z = np.where(m, 20 + 10 * np.sqrt(np.clip(1 - ((d - rm) / hw) ** 2, 0, 1)), -1e9)
    lit_form(c, f)
    # its joints and its inscription: 24 stones, each with two bands of marks
    for k in range(24):
        a = k * math.tau / 24
        for r in np.arange(R0, R1, 0.5):
            c.put(CX + r * math.cos(a), CY + r * math.sin(a), 'deep')
        arc_marks(c, CX, CY, 74.5, a + 0.03, a + math.tau / 24 - 0.03, k * 7 + 1)
        arc_marks(c, CX, CY, 79.5, a + 0.03, a + math.tau / 24 - 0.03, k * 7 + 2)
    # the blades: nine leaves overlapping, each a curved sheet from the rim to the eye, a
    # midrib and veins of light in it. Painted in order, so each lies over the one before.
    N = 9
    under = d < R0 + 1                                  # what shows between the blades: the dark behind them
    c.fill_mask(under, 'dark')
    for k in range(40):
        a = k * math.tau / 40
        for r in np.arange(15, R0, 1.0):
            if h2(k, int(r), 3) < 0.3: c.put(CX + r * math.cos(a), CY + r * math.sin(a), 'deep')
    for i in range(N):
        a0 = i * math.tau / N - math.pi / 2
        lead = []
        for k in range(28):
            t = k / 27
            r = R0 + 1 - (R0 - 7) * t
            a = a0 + 1.75 * t * t + 0.2 * t
            lead.append((CX + r * math.cos(a), CY + r * math.sin(a)))
        back = [(CX + (R0 + 1) * math.cos(a0 + 1.45 * s / 12 + 0.02), CY + (R0 + 1) * math.sin(a0 + 1.45 * s / 12 + 0.02)) for s in range(12, -1, -1)]
        pts = lead + [(CX + 7 * math.cos(a0 + 1.95 + 0.3), CY + 7 * math.sin(a0 + 1.95 + 0.3))] + back[::-1][::-1]
        bm = c.poly(pts) & (d < R0 + 1)
        # shade: lighter toward its leading edge (it lies on top there), darker under the next
        bf = Form(W, H)
        rr = np.hypot(c.X - CX, c.Y - CY)
        rel = ((np.arctan2(c.Y - CY, c.X - CX) - a0) % math.tau)
        bf.z = np.where(bm, 12 + 4 * np.sin(np.clip(rr / R0, 0, 1) * math.pi) - 2.2 * rel, -1e9)
        lit_form(c, bf, where=bm, gain=0.9)
        for (x0, y0), (x1, y1) in zip(lead, lead[1:]):                 # the lit edge, and its shadow over the blade below
            c.put(x0, y0, 'stoneH')
            nx, ny = -(y1 - y0), (x1 - x0)
            L = math.hypot(nx, ny) or 1
            c.put(x0 + 1.4 * nx / L, y0 + 1.4 * ny / L, 'dark')
        # growth lines across the blade, like a shell's or a leaf's, and a midrib of light
        for k, (x, y) in enumerate(lead[3:-4]):
            if k % 3: continue
            for j in range(3, 14):
                ax_ = math.atan2(y - CY, x - CX) + 0.25
                gx, gy = x - j * math.cos(ax_) * 0.8, y - j * math.sin(ax_) * 0.8
                if bm[int(gy) % H, int(gx) % W] and h2(k, j, i) < 0.8: c.put(gx, gy, 'stone')
        am = a0 + 0.55
        rib = [(CX + r * math.cos(am + (R0 - r) * 0.012), CY + r * math.sin(am + (R0 - r) * 0.012)) for r in np.arange(R0 - 3, 14, -1.0)]
        for k, (x, y) in enumerate(rib):
            c.put(x, y, 'stoneL')
            if k % 4 == 1: c.glow(x, y, 'coolM' if k % 12 else 'city')
    # the eye: a hub the blades close on, a lens in it, and a halo of beads of glass
    hf = Form(W, H)
    hf.ellipsoid(CX, CY, 14, 14, 8, base=24)
    lit_form(c, hf, where=c.disc(CX, CY, 14))
    c.fill_mask(c.ring(CX, CY, 13.2, 14.2), 'deep')
    for k in range(12):
        a = k * math.tau / 12
        c.glow(CX + 11 * math.cos(a), CY + 11 * math.sin(a), 'coolM' if k % 2 else 'coolD')
    for j in range(-8, 9):
        for i in range(-8, 9):
            dd = math.hypot(i, j)
            if dd < 6.5: c.glow(CX + i, CY + j, 'cityH' if dd < 2.5 else ('city' if dd < 5 else 'coolM')); c.put(CX + i, CY + j, 'deep')
            elif dd < 8: c.put(CX + i, CY + j, 'stoneH' if (i + j) < -2 else 'dark')
    # glass in the ring: eight lenses, the crown one brighter
    for k in range(8):
        a = -math.pi / 2 + k * math.tau / 8 + math.tau / 16
        lens(c, CX + 76.5 * math.cos(a), CY + 76.5 * math.sin(a))
    lens(c, CX, CY - 76.5, 3.2, 'city', 'cityH', 'stoneH')
    # roots over the top of the ring from the rock, gripping it, mossed
    c.root(Canvas.bez((20, 0), (26, 18), (8, 40), (6, 72)), 4.0, 1.2, seed=1, moss=0.5)
    c.root(Canvas.bez((42, 0), (46, 12), (64, 6), (74, 16)), 3.0, 1.0, seed=2, moss=0.4)
    c.root(Canvas.bez((150, 0), (140, 16), (168, 34), (166, 70)), 3.6, 1.0, seed=3, moss=0.5)
    c.root(Canvas.bez((126, 0), (124, 10), (108, 8), (100, 16)), 2.4, 0.8, seed=4, moss=0.3)
    c.vine(Canvas.bez((74, 16), (70, 40), (84, 52), (80, 78), 40), seed=5, leaf=0.3, flower=0.05)
    c.vine(Canvas.bez((166, 70), (160, 90), (170, 110), (162, 128), 30), seed=6)
    # moss hanging in strands from the upper ring
    for k in range(14):
        a = -math.pi + 0.35 + k * (math.pi - 0.7) / 13
        x, y = CX + (R1 - 1) * math.cos(a), CY + (R1 - 1) * math.sin(a)
        L = 4 + int(10 * h2(k, 88))
        for j in range(L): c.put(x + (1 if j > L // 2 and k % 2 else 0), y + j, 'mossD' if j > L - 3 else 'moss')
    # moss on the lower ring and at its foot; ferns at the base
    low = (d > R0 - 1) & (d < R1 + 2) & (c.Y > CY + 40)
    c.moss(low & (np.array([[h2(x // 3, y // 3, 9) for x in range(W)] for y in range(H)]) < 0.5), 0.6, 11)
    for fx in (18, 30, 146, 160):
        for k in range(5):
            a = -math.pi / 2 + (k - 2) * 0.45
            c.stroke([(fx, 167), (fx + 9 * math.cos(a), 167 + 9 * math.sin(a))], 0.7, 0.4, 'moss')
    return c


# ------------------------------------------------------------------------------ heartwood
def heartwood():
    c = Canvas(W, H)
    OY = CY + 2
    d, ang = c.polar(CX, OY)
    # the rings: each its own slightly wandering circle, as wood grows; the face a little
    # proud at each ring and sunk between, so it reads cut and weathered
    wob = lambda a, k: 1 + 0.03 * math.sin(a * 3 + k * 0.7) + 0.018 * math.sin(a * 7 + k * 1.3)
    rr = d * (1 + 0.03 * np.sin(ang * 3 + 1.0) + 0.018 * np.sin(ang * 7 + 2.0))
    face = rr < 71
    f = Form(W, H)
    f.z = np.where(face, 20 + 2.5 * np.cos(rr * 1.15) - rr * 0.05, -1e9)
    lit_form(c, f, ramp=['warmD', 'warmD', 'warm', 'warm', 'peach'], gain=0.8, dither=0.1)
    # the dark between the rings: every ring a line, the late wood of each year
    ring_r = list(np.arange(9, 71, 5.4))
    for k, r in enumerate(ring_r):
        c.fill_mask(face & (np.abs(rr - r) < 0.6), 'deep' if k % 3 == 0 else 'warmD')
    # the records: marks cut along some of the rings, and a few of them in use -- an arc of
    # light along the ring, brighter at the point where it is being read
    for k, r in enumerate(ring_r[1:], 1):
        if k % 2: arc_marks(c, CX, OY, r - 2.2, 0, math.tau, 40 + k, 'warmD', 1.4)
    for k, (r, a0, a1) in enumerate([(ring_r[2] - 2.6, 0.3, 2.3), (ring_r[4] - 2.6, 3.3, 5.3), (ring_r[6] - 2.6, 1.0, 2.6),
                                     (ring_r[8] - 2.6, 4.0, 6.1), (ring_r[10] - 2.6, 0.5, 1.7)]):
        n = int((a1 - a0) * r)
        for i in range(n):
            a = a0 + (a1 - a0) * i / n
            ra = r * (1 + 0.03 * math.sin(a * 3 + 1.0) + 0.018 * math.sin(a * 7 + 2.0))
            c.glow(CX + ra * math.cos(a), OY + ra * math.sin(a), 'coolM' if i % 4 else 'city')
        ra = r * (1 + 0.03 * math.sin(a1 * 3 + 1.0) + 0.018 * math.sin(a1 * 7 + 2.0))
        lens(c, CX + ra * math.cos(a1), OY + ra * math.sin(a1), 1.6, 'city', 'cityH', 'stoneH')
    # checks: cracks out from the pith
    for k, a in enumerate((0.5, 1.9, 2.9, 4.4, 5.6)):
        for r in np.arange(6, 26 + 30 * h2(k, 1), 0.5):
            c.put(CX + r * math.cos(a + 0.004 * r * r), OY + r * math.sin(a + 0.004 * r * r), 'deep')
    # the parting, down the middle; three iron clasps across it, riveted
    for y in range(int(OY - 71), int(OY + 71)):
        if face[y, int(CX)]: c.put(CX, y, 'deep'); c.put(CX + 1, y, 'warm')
    for cy_ in (OY - 46, OY + 46):
        cf = Form(W, H)
        cf.slab(CX - 12, cy_ - 4, CX + 14, cy_ + 4, 6, bevel=2)
        lit_form(c, cf, where=face)
        for bx in (CX - 8, CX + 10): c.put(bx, cy_, 'stoneH'); c.put(bx + 1, cy_ + 1, 'deep')
    # the pith: a seed of their glass, the door's heart
    for j in range(-8, 9):
        for i in range(-8, 9):
            dd = math.hypot(i, j)
            if dd < 4.5: c.glow(CX + i, OY + j, 'cityH' if dd < 2 else 'city'); c.put(CX + i, OY + j, 'deep')
            elif dd < 7: c.put(CX + i, OY + j, 'stoneH' if (i + j) < -2 else 'stoneL')
    # the bark: thick, in long plates split by deep fissures, mossed on its upper side
    bark = (rr >= 71) & (d < 83 + 2.0 * np.sin(ang * 9) + 1.2 * np.sin(ang * 21))
    bf = Form(W, H)
    plate = np.abs(np.sin(ang * 26 + np.sin(d * 0.35) * 0.6))
    bf.z = np.where(bark, 14 + 6 * np.clip(plate * 1.6, 0, 1) + 3 * np.cos((d - 77) * 0.4), -1e9)
    lit_form(c, bf, ramp=['deep', 'warmD', 'warmD', 'warm', 'warm'], gain=0.85, dither=0.0)
    c.fill_mask(bark & (plate < 0.12), 'deep')
    c.moss(bark & (c.Y < OY - 10) & (np.array([[h2(x // 4, y // 3, 3) for x in range(W)] for y in range(H)]) < 0.5), 0.7, 7)
    # roots out of the bark's foot, spreading along the floor and up the walls: thick,
    # rounded, banded in iron here and there, beaded with glass -- the cables it grew
    roots = [(Canvas.bez((46, 146), (30, 160), (14, 164), (-4, 166)), 6.0, 2.5),
             (Canvas.bez((66, 158), (56, 166), (40, 168), (24, 170)), 4.5, 2.0),
             (Canvas.bez((130, 146), (146, 160), (162, 164), (180, 166)), 6.0, 2.5),
             (Canvas.bez((110, 158), (120, 166), (136, 168), (152, 170)), 4.5, 2.0),
             (Canvas.bez((14, 110), (4, 122), (2, 140), (-2, 158)), 4.0, 2.0),
             (Canvas.bez((162, 110), (172, 122), (174, 140), (178, 158)), 4.0, 2.0)]
    for k, (pts, r0, r1) in enumerate(roots):
        c.root(pts, r0, r1, seed=10 + k, moss=0.25)
        for j in (7, 15):
            x, y = pts[j]
            r = r0 + (r1 - r0) * j / 24
            for t in np.arange(-r - 1, r + 1.1, 0.5): c.put(x - 1, y + t, 'stoneL'); c.put(x, y + t, 'stone')
            c.glow(x + 2, y - r + 1, 'coolM')
    # branches up into the roof, leafed
    for k, pts in enumerate([Canvas.bez((62, 14), (54, 4), (38, 8), (26, -2)),
                             Canvas.bez((114, 14), (122, 4), (138, 8), (150, -2)),
                             Canvas.bez((86, 12), (82, 4), (92, 2), (94, -2))]):
        c.root(pts, 4.5, 2.0, seed=20 + k, moss=0.6)
        c.vine(pts[::2], seed=30 + k, leaf=0.8)
    return c


# ------------------------------------------------------------------------------ stacks
def stacks():
    c = Canvas(W, H)
    CW, CH = 11, 13
    PX0, PX1, PT = 50, 126, 44            # the portal's opening; its head a half circle over PT
    PR = (PX1 - PX0) / 2
    PCX = (PX0 + PX1) / 2
    # the cliff of niches: every cell a recess with a shelf, and in most a record
    for gy in range(0, H // CH + 1):
        for gx in range(-1, W // CW + 2):
            x0, y0 = gx * CW - (gy % 2) * 5, gy * CH
            c.fill_mask(c.rect(x0, y0, x0 + CW, y0 + CH), 'stone')
            c.fill_mask(c.rect(x0 + 2, y0 + 2, x0 + CW - 1, y0 + CH - 2), 'deep')
            c.fill_mask(c.rect(x0, y0, x0 + CW, y0 + 1), 'stoneL')
            c.fill_mask(c.rect(x0 + 1, y0 + CH - 2, x0 + CW, y0 + CH - 1), 'dark')
            v = h2(gx, gy, 71)
            if v < 0.24:                                                # a record, lit: their glass
                c.fill_mask(c.rect(x0 + 4, y0 + 4, x0 + 7, y0 + CH - 2), 'stoneL')
                c.glow_mask(c.rect(x0 + 4, y0 + 5, x0 + 7, y0 + CH - 3), 'coolM' if v < 0.14 else 'city')
                c.glow(x0 + 5, y0 + 5, 'cityH')
            elif v < 0.62:                                              # a record, dark
                c.fill_mask(c.rect(x0 + 4, y0 + 4, x0 + 7, y0 + CH - 2), 'stone')
                c.fill_mask(c.rect(x0 + 4, y0 + 4, x0 + 5, y0 + CH - 2), 'stoneL')
                if v > 0.5: c.glow(x0 + 5, y0 + 6, 'coolD')             # the last of a light in it
            elif v < 0.72:                                              # two thin ones
                c.fill_mask(c.rect(x0 + 3, y0 + 6, x0 + 5, y0 + CH - 2), 'stoneL')
                c.fill_mask(c.rect(x0 + 6, y0 + 5, x0 + 8, y0 + CH - 2), 'stone')
            elif v < 0.86:                                              # grown over: leaves spill out of it
                for k in range(9):
                    lx, ly = x0 + 2 + int(7 * h2(k, gx, gy)), y0 + 3 + int(9 * h2(gy, k, gx))
                    c.put(lx, ly, 'moss' if k % 3 else 'mossL')
                c.put(x0 + 4, y0 + CH - 1, 'moss'); c.put(x0 + 5, y0 + CH, 'mossD')
    # the portal: jambs and a round head of three orders, the leaves, the seal
    d, ang = c.polar(PCX, PT)
    opening = (c.X >= PX0) & (c.X < PX1) & ((c.Y >= PT) | (d < PR))
    frame = (c.X >= PX0 - 12) & (c.X < PX1 + 12) & ((c.Y >= PT) | (d < PR + 12)) & ~opening
    c.clear_mask(frame | opening)
    c.g[frame | opening] = -1
    ff = Form(W, H)
    t = np.where(c.Y >= PT, np.minimum(c.X - (PX0 - 12), (PX1 + 12) - c.X), (PR + 12) - d)
    ff.z = np.where(frame, 14 + 3 * np.cos(t * 0.8) + (t > 4) * 3 + (t > 8) * 3, -1e9)
    lit_form(c, ff)
    for r in (PR + 4, PR + 8):
        c.fill_mask(c.ring(PCX, PT, r - 0.5, r + 0.5) & (c.Y < PT), 'dark')
    for x in (PX0 - 4, PX0 - 8, PX1 + 3, PX1 + 7):
        c.fill_mask(c.rect(x, PT, x + 1, H), 'dark')
    lf = Form(W, H)
    lf.z = np.where(opening, 10 - 1.5 * (np.abs(c.X - PCX) < 1), -1e9)
    for x in range(PX0 + 5, PX1 - 3, 7):                               # the leaves' ribs
        lf.groove([(x, PT - PR + 6), (x, H - 4)], width=1.0, depth=2)
    for y in range(PT + 8, H - 6, 24):
        lf.groove([(PX0 + 2, y), (PX1 - 2, y)], width=1.0, depth=2)
    lit_form(c, lf, gain=0.85, where=opening)
    for y in range(int(PT - PR), H):                                    # the parting
        if opening[y, int(PCX)]: c.put(PCX, y, 'deep')
    for x in range(PX0 + 5, PX1 - 3, 7):                               # a light running down some ribs: the door is written too
        if h2(x, 5) < 0.55: continue
        for y in range(PT + 2 + int(10 * h2(x, 7)), H - 6, 5):
            if opening[y, x] and h2(x, y) < 0.7: c.glow(x, y, 'coolD' if h2(y, x) < 0.7 else 'coolM')
    # the jambs' carving: a column of small cells, each with a mark of light
    for side in (0, 1):
        x0 = PX0 - 10 if side == 0 else PX1 + 4
        for y in range(PT + 2, H - 4, 10):
            c.fill_mask(c.rect(x0 + 1, y, x0 + 6, y + 6), 'deep')
            c.glow(x0 + 3, y + 3, 'city' if h2(side, y) < 0.4 else 'coolM')
    # the seal across the parting: a round lock, grooved, a slit of light
    SX, SY = PCX, 112.0
    sf = Form(W, H)
    sf.ellipsoid(SX, SY, 20, 20, 8, base=16)
    lit_form(c, sf)
    for r in (7.5, 12.5, 17.5):
        c.fill_mask(c.ring(SX, SY, r - 0.5, r + 0.5), 'dark')
    for y in range(int(SY - 16), int(SY + 17)):
        c.glow(SX, y, 'cityH' if abs(y - SY) < 5 else 'city')
    # the lamp in the head of the arch, their colour
    lens(c, PCX, PT - PR - 6, 3.4, 'city', 'cityH', 'stoneH')
    # roots and vines through the stacks, from the roof and out of the grown-over cells
    for k, (pts, r0) in enumerate([(Canvas.bez((12, -2), (22, 34), (2, 70), (14, 128)), 4.0),
                                   (Canvas.bez((164, -2), (152, 40), (174, 76), (160, 140)), 4.0),
                                   (Canvas.bez((34, -2), (42, 14), (30, 22), (40, 40)), 2.6),
                                   (Canvas.bez((142, -2), (134, 18), (146, 30), (136, 48)), 2.6)]):
        c.root(pts, r0, 1.2, seed=k, moss=0.5)
    for k, pts in enumerate([Canvas.bez((0, 30), (30, 36), (20, 60), (44, 70), 44),
                             Canvas.bez((176, 50), (150, 58), (160, 80), (134, 96), 44),
                             Canvas.bez((22, 100), (32, 124), (10, 140), (26, 168), 44),
                             Canvas.bez((150, 108), (140, 130), (162, 146), (150, 168), 44),
                             Canvas.bez((PX0 - 12, PT - 20), (PX0 - 2, PT - 40), (PCX - 10, PT - PR - 10), (PCX, PT - PR - 12), 40)]):
        c.vine(pts, seed=40 + k, leaf=0.5, flower=0.07)
    c.moss(c.rect(0, 152, W, H) & ~opening & (np.array([[h2(x // 2, y // 2, 5) for x in range(W)] for y in range(H)]) < 0.5), 0.55, 3)
    return c


# ------------------------------------------------------------------------------ engine
def engine():
    c = Canvas(W, H)
    d, ang = c.polar(CX, CY)
    f = Form(W, H)
    # the outer wheel: a cog, its teeth outward
    teeth = (np.sin(ang * 28) > 0.2) & (d < 84) & (d >= 78)
    body = (d >= 64) & (d < 78)
    f.z = np.where(body | teeth, 20 + 6 * np.sqrt(np.clip(1 - ((d - 71) / 8.5) ** 2, 0, 1)), -1e9)
    # the inner wheel: teeth inward, meshing
    iteeth = (np.sin(ang * 22 + 0.4) > 0.2) & (d > 44) & (d <= 50)
    ibody = (d > 50) & (d <= 58)
    f.z = np.where(ibody | iteeth, 16 + 5 * np.sqrt(np.clip(1 - ((d - 54) / 5.0) ** 2, 0, 1)), f.z)
    # spokes, and the flower in the middle: six petals round a glass
    for k in range(6):
        a = k * math.tau / 6 + 0.26
        f.capsule(CX + 18 * math.cos(a), CY + 18 * math.sin(a), CX + 47 * math.cos(a), CY + 47 * math.sin(a), 3.2, 2.6, base=14)
        f.ellipsoid(CX + 11 * math.cos(a + 0.52), CY + 11 * math.sin(a + 0.52), 9, 4.5, 6, base=18, rot=a + 0.52)
    lit_form(c, f)
    # bolts round the outer wheel, grooves round both
    for k in range(36):
        a = k * math.tau / 36
        c.put(CX + 71 * math.cos(a), CY + 71 * math.sin(a), 'stoneH')
        c.put(CX + 71 * math.cos(a) + 1, CY + 71 * math.sin(a) + 1, 'deep')
    for r in (66, 76, 52, 57):
        c.fill_mask(c.ring(CX, CY, r - 0.5, r + 0.5) & (body | ibody), 'dark')
    # the wheels are written on: bands of marks round the outer, and on the inner wheel a few
    # beads of glass riding it, each at a mark -- an orrery of something, or an index
    for k in range(48):
        a0_ = k * math.tau / 48
        arc_marks(c, CX, CY, 68.5, a0_ + 0.02, a0_ + math.tau / 48 - 0.02, 300 + k)
        arc_marks(c, CX, CY, 73.5, a0_ + 0.02, a0_ + math.tau / 48 - 0.02, 400 + k)
    for k, a in enumerate((0.4, 1.5, 2.2, 3.7, 4.6, 5.5)):
        lens(c, CX + 54 * math.cos(a), CY + 54 * math.sin(a), 2.0 + (k % 2), 'coolM', 'city', 'stoneH')
    # channels of light along the spokes, and round the inner wheel
    for k in range(6):
        a = k * math.tau / 6 + 0.26
        for r in np.arange(20, 46, 1.0):
            if int(r) % 4: c.glow(CX + r * math.cos(a), CY + r * math.sin(a), 'coolM' if int(r) % 9 else 'city')
    for i in range(160):
        a = i * math.tau / 160
        if i % 5: c.glow(CX + 54.5 * math.cos(a), CY + 54.5 * math.sin(a), 'coolD')
    # the glass at the heart
    for j in range(-7, 8):
        for i in range(-7, 8):
            dd = math.hypot(i, j)
            if dd < 4.5: c.glow(CX + i, CY + j, 'cityH' if dd < 2 else 'city')
            elif dd < 6.5: c.put(CX + i, CY + j, 'stoneL')
    # the dark between the wheels, grown through: vines round it, flowering
    for k in range(3):
        pts = [(CX + (60.5 + 1.8 * math.sin(i * 0.9 + k)) * math.cos(a), CY + (60.5 + 1.8 * math.sin(i * 0.9 + k)) * math.sin(a))
               for i, a in enumerate(np.linspace(k * 2.1, k * 2.1 + 2.6, 60))]
        c.vine(pts, seed=60 + k, leaf=0.5, flower=0.08)
    # pipes from its sides down into the floor, with a valve on each
    for s in (-1, 1):
        x0 = CX + s * 80
        pts = Canvas.bez((x0, CY + 20), (x0 + s * 8, CY + 50), (x0 - s * 2, CY + 70), (x0 + s * 4, H))
        c.stroke(pts, 4.5, 4.5, 'stone', shade=('stoneL', 'deep'))
        for j in (6, 14, 20):
            x, y = pts[j]
            c.stroke([(x - 5, y), (x + 5, y)], 1.5, 1.5, 'stoneL')
        vx, vy = pts[10]
        for i in range(24):
            a = i * math.tau / 24
            c.put(vx + 5 * math.cos(a), vy + 5 * math.sin(a), 'stoneH')
        c.put(vx, vy, 'bone')
        c.vine(Canvas.bez((x0 + s * 6, H), (x0 + s * 2, CY + 60), (x0 - s * 10, CY + 40), (CX + s * 60, CY - 20), 40), seed=70 + s, leaf=0.5, flower=0.05)
    # roots from the roof, down over the top of the wheel
    c.root(Canvas.bez((30, 0), (40, 10), (56, 8), (66, 18)), 3.2, 1.0, seed=81, moss=0.4)
    c.root(Canvas.bez((140, 0), (136, 12), (120, 8), (112, 18)), 3.0, 1.0, seed=82, moss=0.4)
    c.moss(((d > 62) & (d < 86) & (c.Y > CY + 44)) & (np.array([[h2(x // 3, y // 3, 4) for x in range(W)] for y in range(H)]) < 0.5), 0.6, 9)
    return c


DOORS = [('iris', iris), ('heartwood', heartwood), ('stacks', stacks), ('engine', engine)]

if __name__ == '__main__':
    root = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'art')
    prev = sys.argv[sys.argv.index('--preview') + 1] if '--preview' in sys.argv else None
    only = [a for a in sys.argv[1:] if a in dict(DOORS)]
    for name, fn in DOORS:
        if only and name not in only: continue
        c = fn()
        c.save(os.path.join(root, 'door_' + name))
        if prev: c.preview(os.path.join(prev, 'door_' + name + '.png'))
        print('door', name)
