#!/usr/bin/env python3
"""The rest of the archive, seen through the antechamber's opening: a chamber miles across,
painted as a multiplane -- five layers, each a picture on glass at its own distance, which
src/city.c slides past one another as the view moves, the far ones hardly at all.

    0  the far haze: the chamber's ceiling lost in the dark, a few of its lights; the
       horizon's glow; and the pillar of light where its heart is, with the tower it comes
       out of -- one thing, on one glass, so the two never part
    1  the constructs on the horizon, mountain-sized: domes, mesas, an aqueduct, and far
       off another keeper, seated, as ours is
    2  the city on the plain, lights to the horizon, its avenues running to the pillar
    3  the middle distance: a stepped temple, a domed one, the bridge between them, a
       lantern hung on a chain from the ceiling
    4  near: towers rising out of the dark below, roots and chains hanging from above, the
       bridge where the tall ones stand, the mist

Everything is in room px as it stands when the view is between the keeper's two screens
(the home view, camera at HOME); a layer at depth p follows the camera by p of its
movement (p 0: the wall, 1: the screen). Lights that answer (they twinkle, or go out when
a lamp comes to the edge) are not painted in: they are listed, and the game draws them.

    tools/art/vista.py [--preview DIR]    write art/vista_0..4.png, art/vista.ints
"""
import math, os, sys
import numpy as np
from PIL import Image, ImageDraw
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from sculpt import PAL, BAYER4
from paint import h2
from kit import noise as _noise

HOME = (320, 88)                         # the camera of the home view, room px
LX0, LY0, LW, LH = 240, -40, 700, 380    # every layer covers this much of the room, at home
P = (0.8, 0.72, 0.6, 0.42, 0.22)          # how far each layer follows the camera
HZ = 180                                 # the horizon
PILLAR = 590                             # the chamber's heart: a pillar of light
NAMES = list(PAL.keys())
IDX = {n: i for i, n in enumerate(NAMES)}
TWINKLE, DOUSE = 1, 2


class Layer:
    def __init__(self):
        self.a = np.full((LH, LW), -1, np.int16)
        yy, xx = np.mgrid[0:LH, 0:LW]
        self.X, self.Y = xx + LX0 + 0.5, yy + LY0 + 0.5
        self.ox, self.oy, self.w, self.h = LX0, LY0, LW, LH
        self.bay = BAYER4[yy % 4, xx % 4] - 0.5

    def put(self, x, y, name):
        x, y = int(math.floor(x)) - LX0, int(math.floor(y)) - LY0
        if 0 <= x < LW and 0 <= y < LH: self.a[y, x] = IDX[name]

    def fill(self, m, name):
        self.a[m] = IDX[name]

    def ramp(self, m, v, names, dither=1.0):
        """Fill m with v (0..1) as steps of names, ordered-dithered across each step."""
        q = np.clip(v * len(names) + self.bay * dither, 0, len(names) - 1e-6).astype(int)
        for i, n in enumerate(names):
            self.a[m & (q == i)] = IDX[n]

    def poly(self, pts):
        im = Image.new('L', (LW, LH), 0)
        ImageDraw.Draw(im).polygon([(x - LX0, y - LY0) for x, y in pts], fill=255)
        return np.array(im) > 0

    def rect(self, x0, y0, x1, y1):
        return (self.X >= x0) & (self.X < x1) & (self.Y >= y0) & (self.Y < y1)

    def disc(self, cx, cy, r):
        return np.hypot(self.X - cx, self.Y - cy) < r

    def rgba(self):
        out = np.zeros((LH, LW, 4), np.uint8)
        for n, i in IDX.items():
            m = self.a == i
            out[m, :3] = PAL[n]
        out[..., 3] = np.where(self.a >= 0, 255, 0)
        return out


def noise(L, cell, seed):
    return _noise(L, cell, seed)


# ------------------------------------------------------------------------------ 0: the haze
def layer0(lights):
    L = Layer()
    X, Y = L.X, L.Y
    up = Y < HZ
    t = np.clip((Y - LY0) / (HZ - LY0), 0, 1)                             # 0 at the top, 1 at the horizon
    # where the chamber's light is strongest: round the pillar, and a broad glow behind the
    # middle of the view, where the city is
    gp = np.exp(-((X - PILLAR) / 90.0) ** 2)
    gc = np.exp(-((X - 470) / 230.0) ** 2)
    # the air: from the ceiling lost in the dark to the horizon's glow, blue haze, teal near
    # the light
    gk = np.exp(-((X - 362) / 34.0) ** 2 - ((Y - 150) / 40.0) ** 2)      # a little light behind the far keeper
    v = t ** 2.6 * (0.62 + 0.22 * gc + 0.3 * gp) + 0.1 * gk
    L.ramp(up, np.clip(v, 0, 0.999), ['void', 'deep', 'deep', 'waterD', 'dark', 'waterD', 'water', 'coolM'], dither=1.0)
    # shafts of light down through it from openings far up in the ceiling, slanting
    for sx, w, sl, st in ((360, 16, 0.55, 0.9), (455, 10, 0.5, 0.7), (700, 22, 0.62, 1.0), (790, 12, 0.6, 0.7)):
        d = np.abs((X - sx) - (Y - LY0) * sl)
        m = up & (d < w) & (Y < HZ - 20)
        k = np.clip(1 - d / w, 0, 1) * np.clip(1 - (Y - LY0) / (HZ - LY0 - 10), 0, 1) * st
        L.ramp(m & (k > 0.2), np.clip(v + 0.2 * k, 0, 0.999), ['void', 'deep', 'deep', 'waterD', 'dark', 'waterD', 'water', 'coolM'], dither=1.0)
    # below the horizon, the far plain going dark toward you
    below = ~up
    tb = np.clip((Y - HZ) / 60.0, 0, 1)
    L.ramp(below, np.clip((1 - tb) ** 1.5 * (0.62 + 0.3 * gc + 0.2 * gp), 0, 0.999), ['void', 'deep', 'waterD', 'dark', 'water'], dither=1.0)
    # the ceiling's courses, miles up: faint lights along lines that bend down away from you
    for k in range(8):
        for x in range(LX0, LX0 + LW, 2):
            y = 70 + k * 7 + 0.00009 * (x - 520) ** 2 * (1 + k * 0.3)
            if y > HZ - 45: continue
            hv = h2(x, k, 3)
            if hv < 0.62: continue
            L.put(x, y, 'waterD' if hv < 0.9 else ('coolD' if hv < 0.985 else 'coolM'))
            if hv > 0.993: lights.append((0, x, int(y), 'city', TWINKLE))
    # far falls of water, from the ceiling down into the haze
    for fx, fw in ((318, 4), (738, 3), (812, 5)):
        m = L.rect(fx, LY0, fx + fw, HZ - 6)
        L.ramp(m, np.clip(v + 0.12 + 0.08 * noise(L, 3, fx), 0, 0.999), ['void', 'deep', 'deep', 'waterD', 'dark', 'waterD', 'water', 'coolM'], dither=1.0)
    # the pillar of light: the chamber's heart, from the tower on the horizon up out of sight,
    # and where it meets the ceiling, spreading
    d = np.abs(X - PILLAR)
    halo = up & (d < 16)
    k = np.clip(1 - d / 16.0, 0, 1) ** 2.2 * (0.6 + 0.4 * t)
    L.ramp(halo & (k > 0.04), np.clip(v + k * 0.42, 0, 0.999), ['void', 'deep', 'deep', 'waterD', 'dark', 'waterD', 'coolD', 'coolM', 'city'], dither=1.0)
    L.fill(up & (d < 2.2) & (Y < 132), 'city')
    L.fill(up & (d < 1.0) & (Y < 132), 'cityH')
    top = np.hypot((X - PILLAR) / 60.0, (Y - 50) / 22.0)
    L.ramp((top < 1) & up, np.clip(v + (1 - top) * 0.3, 0, 0.999), ['void', 'deep', 'deep', 'waterD', 'dark', 'waterD', 'coolD', 'coolM'], dither=1.0)
    # the tower the light comes out of: tall, tapering, crowned. On this glass and not the
    # horizon's, though it stands among those: a beam and its tower on two glasses slide
    # apart as the view moves, and they are one thing.
    tower = L.poly([(PILLAR - 13, HZ + 2), (PILLAR - 5, 142), (PILLAR - 3, 130), (PILLAR + 3, 130), (PILLAR + 5, 142), (PILLAR + 13, HZ + 2)])
    tower |= L.rect(PILLAR - 8, 140, PILLAR + 8, 143)
    L.ramp(tower, np.clip(0.18 + np.clip((HZ - Y) / 150.0, 0, 1) * 0.55, 0, 0.999), ['deep', 'waterD', 'dark', 'waterD', 'water'], dither=1.0)
    for y in range(134, HZ, 5):                                            # its windows, up it
        L.put(PILLAR - 2 + (y // 5) % 4, y, 'coolM')
    return L


# ------------------------------------------------------------------------------ 1: the horizon's constructs
def layer1(lights):
    L = Layer()
    X, Y = L.X, L.Y
    sil = np.zeros((LH, LW), bool)
    # domes, their finials
    for cx, r in ((352, 20), (470, 34), (536, 15), (700, 20), (846, 30)):
        sil |= L.disc(cx, HZ + 2, r) & (Y < HZ + 2)
        sil |= L.rect(cx - 1, HZ - r - 9, cx + 1, HZ - r)
    # mesas, stepped
    for x0, x1, h in ((396, 444, 16), (404, 436, 26), (412, 428, 34), (760, 824, 12), (770, 812, 20)):
        sil |= L.rect(x0, HZ - h, x1, HZ + 2)
    # an aqueduct on arches, going away along the horizon
    sil |= L.rect(614, HZ - 22, 776, HZ - 17)
    for x in range(614, 776, 11):
        sil |= L.rect(x, HZ - 17, x + 3, HZ + 2)
    # another keeper, far off, seated in profile as ours is, its hand out: the same shape,
    # a mile away
    KX, KB, KS = 344, HZ + 2, 1.3
    def kp(pts): return [(KX + x * KS, KB + y * KS) for x, y in pts]
    sil |= L.poly(kp([(0, 0), (0, -12), (4, -22), (6, -34), (2, -40), (8, -46), (16, -44), (18, -36),
                      (20, -22), (28, -18), (30, -10), (24, -8), (24, 0)]))
    sil |= L.poly(kp([(4, -26), (-10, -16), (-12, -14), (-8, -14), (6, -22)]))
    # their air: the higher, the more haze between -- the tops go into it
    hz = np.clip((HZ - Y) / 150.0, 0, 1)
    L.ramp(sil & (Y >= 0), np.clip(0.18 + hz * 0.55, 0, 0.999), ['deep', 'waterD', 'dark', 'waterD', 'water'], dither=1.0)

    # lights in them: few, small
    for i in range(220):
        x, y = LX0 + h2(i, 11, 1) * LW, LY0 + h2(i, 11, 2) * (HZ - LY0)
        xi, yi = int(x) - LX0, int(y) - LY0
        if 0 <= xi < LW and 0 <= yi < LH and sil[yi, xi]:
            hv = h2(i, 11, 3)
            if hv < 0.12: lights.append((1, int(x), int(y), 'roseL' if hv < 0.03 else 'coolM', TWINKLE))
            else: L.put(x, y, 'coolM' if hv > 0.75 else 'coolD')
    L.put(KX + 14 * KS, KB - 40 * KS, 'city')                               # the far keeper's eye, awake
    return L


# ------------------------------------------------------------------------------ 2: the city on the plain
def layer2(lights):
    L = Layer()
    X, Y = L.X, L.Y
    # the skyline right at the horizon: blocks of every size, dark on the glow
    sil = np.zeros((LH, LW), bool)
    for i in range(170):
        x = LX0 + h2(i, 21, 1) * LW
        if 330 < x < 390: continue                                         # the far keeper shows through
        w = 2 + int(h2(i, 21, 2) * 6)
        hgt = 2 + int(h2(i, 21, 3) ** 2.2 * 20)
        sil |= L.rect(x, HZ - hgt + 3, x + w, HZ + 5)
    L.fill(sil, 'deep')
    L.fill(sil & ~np.roll(sil, 1, axis=0), 'waterD')
    for i in range(420):
        x = LX0 + h2(i, 22, 1) * LW; y = HZ + 4 - h2(i, 22, 2) * 20
        xi, yi = int(x) - LX0, int(y) - LY0
        if 0 <= xi < LW and 0 <= yi < LH and sil[yi, xi] and h2(i, 22, 3) < 0.45:
            L.put(x, y, 'coolD' if h2(i, 22, 4) < 0.6 else 'coolM')
    # the plain: its lights in perspective, the rows closer together toward the horizon,
    # districts lit and dark, the light gathering toward the pillar's foot and the middle
    dist = noise(L, 22, 5) * 0.7 + noise(L, 9, 6) * 0.3
    for k in range(40):
        y = HZ + 5 + k * 1.1 + k * k * 0.08
        if y > HZ + 125: break
        yi = int(y) - LY0
        if not (0 <= yi < LH): continue
        for x in range(LX0, LX0 + LW):
            xi = x - LX0
            near = math.exp(-((x - PILLAR) / 110.0) ** 2) * 0.5 + math.exp(-((x - 470) / 200.0) ** 2) * 0.3
            dense = (dist[yi, xi] - 0.45) * 2.2 + near
            hv = h2(x, k, 23)
            if hv < 0.26 * max(0.0, dense) * (1.0 - k / 70.0):
                name = 'coolD' if hv < 0.26 * max(0.0, dense) * 0.55 else 'coolM'
                if h2(x, k, 25) < 0.06: name = 'city'
                if h2(x, k, 29) < 0.012: name = 'amber'
                if h2(x, k, 31) < 0.006: name = 'roseL'
                L.put(x, y, name)
                if h2(x, k, 33) < 0.01: lights.append((2, x, int(y), 'cityH', TWINKLE))
    for a in range(-8, 9):                                                 # the avenues, running to the pillar's foot
        if a == 0: continue
        for s in np.arange(3, 140, 0.6):
            y = HZ + 4 + s * 0.85
            x = PILLAR + a * s * 1.7
            if y > HZ + 125 or not (LX0 <= x < LX0 + LW): continue
            if h2(int(x), int(y), 41) < 0.5: L.put(x, y, 'city' if (int(s) % 11) == 0 else 'coolD')
    # the haze lying on the plain, thicker toward the horizon
    mist = (Y > HZ + 2) & (Y < HZ + 30) & (L.a < 0)
    return L


# ------------------------------------------------------------------------------ 3: the middle distance
def layer3(lights):
    L = Layer()
    X, Y = L.X, L.Y
    sil = np.zeros((LH, LW), bool)
    GROUND = 320
    # a stepped temple: five tiers, each tier's edge a line of lights, a shrine at its top
    tiers = [(314, 440, GROUND, 262), (326, 428, 262, 236), (338, 416, 236, 214), (350, 404, 214, 198), (362, 392, 198, 186)]
    for x0, x1, yb, yt in tiers:
        sil |= L.poly([(x0, yb), (x0 + 5, yt), (x1 - 5, yt), (x1, yb)])
        for x in range(x0 + 6, x1 - 5, 2):
            lights.append((3, x, yt + 1, 'coolM' if (x // 2) % 4 else 'city', DOUSE))
        for x in range(x0 + 9, x1 - 8, 7):                                 # doors in each tier, lit within
            for y in range(yt + 4, min(yb, yt + 10)):
                lights.append((3, x, y, 'coolD', DOUSE))
    sil |= L.rect(371, 172, 383, 186)                                      # the shrine
    for (x, y) in ((375, 178), (376, 178), (377, 178), (375, 179), (376, 179), (377, 179)):
        lights.append((3, x, y, 'roseL', DOUSE))
    lights.append((3, 376, 177, 'accent', DOUSE))
    # a domed temple: a drum of columns, a dome, its oculus lit amber
    sil |= L.rect(618, 214, 682, GROUND)
    sil |= L.disc(650, 214, 32) & (Y < 214)
    sil |= L.rect(648, 170, 652, 184)
    for x in range(622, 680, 6):                                           # between its columns, their light
        for y in range(222, 270, 2):
            lights.append((3, x, y, 'coolD' if y % 4 else 'coolM', DOUSE))
    for (x, y) in ((649, 176), (650, 176), (649, 177), (650, 177)):
        lights.append((3, x, y, 'amber', DOUSE))
    # the bridge between them, on arches, lanterns along it (and a procession, drawn live)
    sil |= L.rect(430, 232, 620, 237)
    for x in range(436, 620, 26):
        sil |= L.rect(x, 237, x + 6, GROUND)
        sil |= L.rect(x + 2, 229, x + 4, 232)
        lights.append((3, x + 3, 228, 'amber', DOUSE))
    # spires, each with a light at its tip
    for x, top in ((452, 190), (474, 206), (520, 172), (560, 214), (598, 196), (704, 186), (744, 210), (786, 200)):
        sil |= L.poly([(x - 5, GROUND), (x - 1, top), (x + 1, top), (x + 5, GROUND)])
        lights.append((3, x, top - 1, 'city', DOUSE))
        for y in range(top + 12, GROUND, 9):
            if h2(x, y, 7) < 0.4: L.put(x, y, 'coolD')
    # a lantern the size of a house, hung on a chain from the ceiling
    LX, LYt = 352, 96
    for y in range(LY0, LYt, 2):
        L.put(LX + 10, y, 'deep'); L.put(LX + 11, y + 1, 'deep')
    cage = L.poly([(LX, LYt), (LX + 22, LYt), (LX + 26, LYt + 16), (LX + 18, LYt + 30), (LX + 4, LYt + 30), (LX - 4, LYt + 16)])
    sil |= cage
    for y in range(LYt + 6, LYt + 28, 2):
        for x in range(LX + 2, LX + 21, 3):
            lights.append((3, x + (y // 2) % 2, y, 'city' if (x + y) % 7 else 'cityH', DOUSE))
    # dark against the haze, the tops of things caught by the chamber's light
    L.fill(sil, 'deep')
    L.fill(sil & ~np.roll(sil, 1, axis=0), 'dark')
    L.fill(sil & ~np.roll(sil, -1, axis=1) & (Y > 180), 'waterD')        # an edge toward the pillar
    # mist in the depths between them
    mist = (Y > 250) & ~sil
    L.ramp(mist & (noise(L, 11, 51) > 0.4), np.clip((Y - 250) / 80.0, 0, 0.99) * 0.8, ['deep', 'waterD', 'dark'], dither=1.0)
    return L


# ------------------------------------------------------------------------------ 4: near
def layer4(lights):
    L = Layer()
    X, Y = L.X, L.Y
    sil = np.zeros((LH, LW), bool)
    # towers rising out of the dark below, their tops just in view; windows lit here and there
    for x0, x1, top in ((256, 298, 222), (298, 318, 246), (650, 688, 214), (688, 714, 238), (740, 782, 252), (812, 840, 230)):
        sil |= L.rect(x0, top, x1, LY0 + LH)
        sil |= L.rect(x0 - 2, top, x1 + 2, top + 4)
        sil |= L.rect(x0 + 3, top - 6, x0 + 5, top)
        for x in range(x0 + 4, x1 - 3, 5):
            for y in range(top + 10, LY0 + LH, 8):
                if h2(x, y, 9) < 0.2: lights.append((4, x, y, 'city' if h2(x, y, 10) < 0.3 else 'coolM', DOUSE))
    # the near bridge, where the tall ones stand
    sil |= L.rect(318, 262, 470, 268)
    for x in range(326, 470, 30):
        sil |= L.rect(x, 268, x + 5, LY0 + LH)
    for x in range(322, 470, 12):                                          # its lamps, low
        lights.append((4, x, 260, 'amber' if (x // 12) % 3 == 0 else 'coolM', DOUSE))
    # roots and chains hanging from the ceiling, dark on the haze
    for x, bottom, w in ((330, 58, 5), (346, 30, 3), (612, 70, 6), (632, 40, 3), (760, 50, 4)):
        pts = [(x + math.sin(i * 0.08 + x) * 3, LY0 + i) for i in range(0, bottom - LY0, 2)]
        for i, (px, py) in enumerate(pts):
            r = w * (1 - i / len(pts) * 0.7)
            sil |= L.disc(px, py, r)
    L.fill(sil, 'void')
    L.fill(sil & ~np.roll(sil, 1, axis=0) & (Y > 200), 'deep')
    # mist, low, drifting between the towers
    mist = (Y > 272) & ~sil
    L.ramp(mist & (noise(L, 13, 61) > 0.45), np.clip((Y - 272) / 60.0, 0, 0.99), ['deep', 'waterD', 'dark'], dither=1.0)
    return L


def build():
    lights = []
    layers = [layer0(lights), layer1(lights), layer2(lights), layer3(lights), layer4(lights)]
    return layers, lights


# ------------------------------------------------------------------------------ out
def write(layers, lights, root):
    for i, L in enumerate(layers):
        Image.fromarray(L.rgba(), 'RGBA').save(os.path.join(root, 'vista_%d.png' % i))
    # one table for the game: the layers (depth x 1000, their origin), then the lights
    # (layer, x, y, colour, what they answer to); five numbers a row
    rows = [(len(layers), len(lights), HOME[0], HOME[1], 0)]
    rows += [(int(round(P[i] * 1000)), LX0, LY0, LW, LH) for i in range(len(layers))]
    rows += [(l, x, y, IDX[n], f) for (l, x, y, n, f) in lights]
    with open(os.path.join(root, 'vista.ints'), 'w') as f:
        for r in rows: f.write(' '.join(str(v) for v in r) + '\n')


def preview(layers, lights, path, archive_png, glow_png, scale=2):
    """The six screens as the game would compose them: the layers at their depths for each
    view, and the wall over them under a flat light."""
    wall = np.array(Image.open(archive_png).convert('RGBA')).astype(float)
    glow = np.array(Image.open(glow_png).convert('RGBA'))
    views = [(0, 0), (320, 0), (640, 0), (0, 176), (320, 176), (640, 176)]
    rgba = [L.rgba() for L in layers]
    sheet = Image.new('RGB', (320 * 3 + 4, 176 * 2 + 2), (255, 0, 255))
    for vi, (cx, cy) in enumerate(views):
        out = np.zeros((176, 320, 3), float)
        for i, im in enumerate(rgba):
            dx, dy = (cx - HOME[0]) * P[i], (cy - HOME[1]) * P[i]
            # layer px (lx, ly) shows at room (lx + dx, ly + dy), screen minus (cx, cy)
            sx0 = int(round(LX0 + dx - cx)); sy0 = int(round(LY0 + dy - cy))
            for (l, x, y, n, f) in lights:
                if l == i: pass
            ys, xs = np.nonzero(im[..., 3])
            X, Y = xs + sx0, ys + sy0
            ok = (X >= 0) & (X < 320) & (Y >= 0) & (Y < 176)
            out[Y[ok], X[ok]] = im[ys[ok], xs[ok], :3]
            for (l, x, y, n, f) in lights:
                if l != i: continue
                X_, Y_ = int(round(x + dx - cx)), int(round(y + dy - cy))
                if 0 <= X_ < 320 and 0 <= Y_ < 176: out[Y_, X_] = PAL[n]
        w = wall[cy:cy + 176, cx:cx + 320]
        m = w[..., 3] > 0
        out[m] = w[m, :3] * 0.62
        g = glow[cy:cy + 176, cx:cx + 320]
        mg = g[..., 3] > 0
        out[mg] = g[mg, :3]
        im = Image.fromarray(out.clip(0, 255).astype(np.uint8))
        sheet.paste(im, ((vi % 3) * 322, (vi // 3) * 178))
        im.resize((960, 528), Image.NEAREST).save(path.replace('.png', '_%s.png' % 'ABCDEF'[vi]))
    sheet.resize((sheet.width * scale, sheet.height * scale), Image.NEAREST).save(path)


if __name__ == '__main__':
    root = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'art')
    layers, lights = build()
    write(layers, lights, root)
    if '--preview' in sys.argv:
        d = sys.argv[sys.argv.index('--preview') + 1]
        preview(layers, lights, os.path.join(d, 'vista.png'), os.path.join(root, 'archive.png'), os.path.join(root, 'archive_glow.png'))
    print('vista: %d layers, %d lights' % (len(layers), len(lights)))
