"""The archive's parts, as drawing: rib, cell, stacks, vein, root, rock. Every bay of the
building is made of these and nothing else (claude/ARCHIVE.md), so the screens share one
language. A bay script composes them on a Canvas (paint.py) and writes a picture of its far
wall, a picture of its own light, and the paths of its veins.

Coordinates are px in the bay's canvas. Light is from the upper left, soft, for form only;
the room's light does the rest."""
import math
import numpy as np
from sculpt import Form, shade, TAG_WALL, TAG_THING, TAG_STONE
from paint import Canvas, h2, shade_ramp

STONE = ['deep', 'dark', 'stone', 'stoneL', 'stoneH']
LIGHT = (-0.45, -0.8, 0.6)


def lit(c, form, ramp=STONE, gain=0.95, dither=0.1, tag=TAG_THING, where=None):
    b, m = shade(form, light=LIGHT)
    if where is not None: m = m & where
    q = shade_ramp(b * gain, ramp, dither)
    for i, n in enumerate(ramp):
        c.fill_mask(m & (q == i), n, tag)
    return m


class Veins:
    """Paths of the archive's light, each running toward the heart. Drawn into the glow, and
    kept as polylines so the game can send pulses along them."""
    def __init__(self):
        self.paths = []

    def add(self, c, pts, lit_every=4, dim='coolD', node='coolM', record=True):
        pts = [(float(x), float(y)) for x, y in pts]
        n = 0
        for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
            L = max(int(math.hypot(x1 - x0, y1 - y0)), 1)
            for k in range(L):
                x, y = x0 + (x1 - x0) * k / L, y0 + (y1 - y0) * k / L
                c.put(x, y, 'deep')                                 # the channel cut for it
                c.glow(x, y, node if n % lit_every == 0 else dim)
                n += 1
        if record: self.paths.append(pts)

    def write(self, path, ox=0, oy=0):
        with open(path, 'w') as f:
            for p in self.paths:
                f.write(' '.join('%d,%d' % (round(x + ox), round(y + oy)) for x, y in p) + '\n')


# ------------------------------------------------------------------------------ cell
def arc_marks(c, cx, cy, r, a0, a1, seed, name='dark', step=1.0):
    """The catalogue, cut along an arc: marks and gaps, now and then a deeper stroke."""
    n = int(abs(a1 - a0) * r / step)
    for k in range(n):
        a = a0 + (a1 - a0) * k / n
        v = h2(k, seed)
        if v < 0.28: continue
        c.put(cx + r * math.cos(a), cy + r * math.sin(a), name)
        if v > 0.8: c.put(cx + (r - 1) * math.cos(a), cy + (r - 1) * math.sin(a), name)


def cell(c, cx, cy, r, state, seed=0, blades=None, ring=None):
    """A round opening, a moulded ring, an iris, a lens. state: 'living', 'asleep', 'dark'.
    The same drawing at every size: under r 4 the iris is only a recess and a dot."""
    ring = ring if ring is not None else max(1.0, r * 0.16)
    d, ang = c.polar(cx, cy)
    outer = d < r + ring
    inner = d < r
    # the ring, rounded in section
    f = Form(c.w, c.h, c.ox, c.oy)
    rim = outer & ~inner
    f.z = np.where(rim, 20 + 6 * np.sqrt(np.clip(1 - ((d - r - ring / 2) / (ring / 2 + 0.01)) ** 2, 0, 1)), -1e9)
    if r >= 4: lit(c, f, where=rim)
    else:
        c.fill_mask(rim & (c.Y < cy), 'stoneL'); c.fill_mask(rim & (c.Y >= cy), 'stone')
    c.fill_mask(inner, 'deep')
    lens_r = max(0.8, min(r * 0.22, 6.5))
    if r >= 6:
        # the iris: blades from the rim to the lens, each lit along its leading edge
        n = blades or (6 if r < 20 else 9)
        for i in range(n):
            a0 = i * math.tau / n + seed
            lead = []
            for k in range(20):
                t = k / 19
                rr = r - (r - lens_r - 1) * t
                lead.append((cx + rr * math.cos(a0 + 1.6 * t * t), cy + rr * math.sin(a0 + 1.6 * t * t)))
            back = [(cx + r * math.cos(a0 + 1.5 * s / 8), cy + r * math.sin(a0 + 1.5 * s / 8)) for s in range(9)]
            m = c.poly(lead + back[::-1]) & inner
            bf = Form(c.w, c.h, c.ox, c.oy)
            rel = (ang - a0) % math.tau
            bf.z = np.where(m, 12 + 3 * np.sin(np.clip(d / r, 0, 1) * math.pi) - 2.0 * rel, -1e9)
            lit(c, bf, where=m, gain=0.85)
            for (x0, y0), (x1, y1) in zip(lead, lead[1:]):
                c.put(x0, y0, 'stoneL' if r < 30 else 'stoneH')
                nx, ny = -(y1 - y0), (x1 - x0); L = math.hypot(nx, ny) or 1
                c.put(x0 + 1.2 * nx / L, y0 + 1.2 * ny / L, 'dark')
    # the lens: its light is the cell's state
    core = {'living': ('city', 'cityH'), 'asleep': ('coolD', 'coolM'), 'dark': (None, None)}[state]
    for j in range(-int(lens_r) - 2, int(lens_r) + 3):
        for i in range(-int(lens_r) - 2, int(lens_r) + 3):
            dd = math.hypot(i, j)
            if dd < lens_r + 0.3:
                c.put(cx + i, cy + j, 'deep' if core[0] else 'dark')
                if core[0]: c.glow(cx + i, cy + j, core[1] if dd < lens_r * 0.45 else core[0])
            elif r >= 6 and dd < lens_r + 1.5:
                c.put(cx + i, cy + j, 'stoneL' if (i + j) < 0 else 'stone')


def seed_cell(c, x, y, state):
    """The smallest cell, drawn by hand: a round socket 7 across, its ring lit a little on the
    upper left, dark inside, and in it a lens whose light is its state."""
    rows = ['..443..',
            '.4...3.',
            '4.....2',
            '4..o..2',
            '3.....2',
            '.3...2.',
            '..222..']
    for j, row in enumerate(rows):
        for i, ch in enumerate(row):
            if ch == '.':
                if 0 < j < 6 and 0 < i < 6 and (abs(i - 3) + abs(j - 3)) < 5: c.put(x + i, y + j, 'deep', TAG_WALL)
                continue
            if ch == 'o':
                c.put(x + i, y + j, 'deep', TAG_WALL)
                if state == 'living': c.glow(x + i, y + j, 'city'); c.glow(x + i, y + j - 1, 'coolM')
                elif state == 'asleep': c.glow(x + i, y + j, 'coolD')
                continue
            c.put(x + i, y + j, {'2': 'dark', '3': 'stone', '4': 'stoneL'}[ch], TAG_WALL)


def stacks(c, x0, y0, x1, y1, seed=0, alive=0.08, asleep=0.5, where=None, darken=None, pier=44, grade=None):
    """The wall's fabric: the collection on its shelves. Piers every `pier` px divide the wall
    into bays of shelving; each shelf a lit edge and a shadow under it; on each shelf the
    kept things side by side -- round sockets with a lens (the cells), pairs of small ones,
    tall glass vessels, record tablets leaning, and gaps where something has gone.
    darken(x, y) -> 0..1 turns them dark toward the edges of the archive; or grade(x, y) ->
    (living, living + asleep) gives the odds outright."""
    SH = 12
    m = c.rect(x0, y0, x1, y1)
    if where is not None: m = m & where
    c.fill_mask(m, 'deep', TAG_WALL)
    def ok(x, y): return 0 <= x < c.w and 0 <= y < c.h and m[int(y), int(x)]
    def P(x, y, n):
        if ok(x, y): c.put(x, y, n, TAG_WALL)
    def G(x, y, n):
        if ok(x, y): c.glow(x, y, n)
    for gy, y in enumerate(range(y0, y1, SH)):
        sy = y + SH - 3                                                   # the shelf's top
        for x in range(x0, x1):
            P(x, sy, 'stone'); P(x, sy + 1, 'dark'); P(x, sy + 2, 'deep')
            if (x - x0) % 3 == 0: P(x, sy - 1, 'dark')                    # the back of the shelf, in shadow
        x = x0 + int(h2(gy, seed, 4) * 5)
        k = 0
        while x < x1:
            bayx = (x - x0) % pier
            if bayx > pier - 6:                                           # the pier between two bays of shelving
                for yy in range(y, y + SH): P(x, yy, 'stone'); P(x + 1, yy, 'dark')
                x += 3; continue
            v = h2(k + seed * 97, gy, 11)
            if grade: pa, ps = grade(x, y)
            else:
                dk = darken(x, y) if darken else 0.0
                pa, ps = alive * (1 - dk), (alive + asleep) * (1 - dk)
            st = h2(k, gy + seed * 13, 29)
            state = 'living' if st < pa else ('asleep' if st < ps else 'dark')
            base = sy - 1
            if v < 0.5:                                                   # a cell: a round socket, a lens
                for j, row in enumerate(['.33.', '3..2', '3..2', '.22.']):
                    for i, ch in enumerate(row):
                        if ch != '.': P(x + i, base - 5 + j, 'stone' if ch == '3' else 'dark')
                P(x + 1, base - 4, 'deep'); P(x + 2, base - 3, 'deep')
                if state == 'living': G(x + 1, base - 3, 'city'); G(x + 2, base - 4, 'coolM')
                elif state == 'asleep': G(x + 1, base - 3, 'coolD')
                x += 5
            elif v < 0.62:                                                # a pair of small ones
                for j in (0, 3):
                    P(x, base - 5 + j, 'stone'); P(x + 1, base - 5 + j, 'dark'); P(x + 1, base - 4 + j, 'dark')
                    if state == 'living' and j == 0: G(x, base - 4, 'city')
                    elif state != 'dark': G(x, base - 4 + j, 'coolD')
                x += 3
            elif v < 0.74:                                                # a tall vessel of glass
                for yy in range(base - 8, base + 1):
                    P(x, yy, 'stoneL' if yy < base - 6 else 'stone'); P(x + 1, yy, 'dark'); P(x + 2, yy, 'dark')
                P(x, base - 9, 'stone'); P(x + 1, base - 9, 'stone')
                if state == 'living':
                    for yy in range(base - 6, base - 1): G(x + 1, yy, 'coolM' if yy % 2 else 'city')
                elif state == 'asleep': G(x + 1, base - 3, 'coolD')
                x += 4
            elif v < 0.86:                                                # a record tablet, leaning
                for yy in range(7):
                    xx = x + (1 if yy < 3 else 0)
                    P(xx, base - yy, 'stone'); P(xx + 1, base - yy, 'dark'); P(xx + 2, base - yy, 'dark')
                if state != 'dark':
                    for yy in (2, 4): G(x + 1, base - yy, 'coolD')
                x += 4
            else:                                                         # a gap: whatever was here is gone
                x += 3 + int(h2(k, gy, 3) * 4)
            k += 1


# ------------------------------------------------------------------------------ rib
def rib(c, x, y_top, y_foot, w, lean=0.0, root_side=1, seed=0, veins=None, vein_to=None):
    """A pier leaning in toward the top by `lean` px, into an arch out of sight: a shaft of
    three rounded columns bound by bands of stone, a root spiralling up it, a vein in a
    channel down its middle with a node of glass at each band. The vein runs down."""
    BAND = 18
    cx_at = lambda y: x + w / 2 + lean * max(0.0, (y_foot - y) / (y_foot - y_top)) ** 2.2
    f = Form(c.w, c.h, c.ox, c.oy)
    ys = list(range(y_foot, y_top - 2, -2))
    for k, off in enumerate((-w / 3, 0.0, w / 3)):                        # three shafts
        for y0, y1 in zip(ys, ys[1:]):
            f.capsule(cx_at(y0) + off, y0, cx_at(y1) + off, y1, w / 5.2, depth=w / 5.2 + (2 if off == 0 else 0), base=10)
    for yb in range(y_foot - 12, y_top, -BAND):                            # the bands
        cx = cx_at(yb)
        f.slab(cx - w / 2 - 1, yb - 2, cx + w / 2 + 1, yb + 2, 6, bevel=1.5, base=14)
    f.slab(x - 3, y_foot - 10, x + w + 3, y_foot + 2, 10, bevel=3, base=12)   # the plinth
    m = lit(c, f)
    pts = [(cx_at(y) + 0.5, y) for y in range(max(y_top, 0), y_foot - 10, 2)]
    if veins is not None:
        veins.add(c, pts + (vein_to or []), lit_every=6)
        for yb in range(y_foot - 12, y_top, -BAND):
            cx = cx_at(yb) + 0.5
            if 0 <= yb < c.h: c.glow(cx, yb - 1, 'city'); c.glow(cx, yb, 'cityH'); c.glow(cx, yb + 1, 'city')
    # the root, spiralling up: seen where it crosses in front, its shadow where it goes behind
    for k in range(0, 3000):
        y = y_foot - 6 - k * 0.1
        if y < y_top: break
        ph = y * 0.05 + seed
        s_ = math.sin(ph)
        cx = cx_at(y) + root_side * s_ * (w / 2 + 1.5)
        r = 3.0 - 1.6 * (y_foot - y) / max(1, (y_foot - y_top))
        if math.cos(ph) * root_side > -0.15:
            for j in range(-3, 4):
                for i in range(-3, 4):
                    dd = i * i + j * j
                    if dd <= r * r:
                        c.put(cx + i, y + j, 'warm' if (i + j) < -0.5 else ('warmD' if (i + j) < 1.5 else 'deep'))
            if h2(k, seed, 5) < 0.02: c.put(cx - 1, y - r - 0.5, 'moss')
        elif abs(s_) > 0.85 and 0 <= y < c.h:
            c.put(cx, y, 'deep')


# ------------------------------------------------------------------------------ rock and root
def noise(c, cell, seed):
    rng = np.random.default_rng(seed)
    gw, gh = (c.ox + c.w) // cell + 2, (c.oy + c.h) // cell + 2       # the grid spans the picture up to here
    g = rng.random((gh, gw))
    fx, fy = c.X / cell, c.Y / cell
    i0, j0 = fx.astype(int), fy.astype(int)
    sx, sy = fx - i0, fy - j0
    sx, sy = sx * sx * (3 - 2 * sx), sy * sy * (3 - 2 * sy)
    a, b = g[j0, i0], g[j0, i0 + 1]
    d_, e = g[j0 + 1, i0], g[j0 + 1, i0 + 1]
    return (a + (b - a) * sx) * (1 - sy) + (d_ + (e - d_) * sx) * sy


def rock(c, m, seed=0):
    """Raw rock, the mountain where it has not been cut or has pushed back in: strata,
    cracks, a lip of light on each stratum."""
    wob = noise(c, 24, seed) * 10
    s = (c.Y + c.X * 0.18 + wob) / 9.0
    band = s - np.floor(s)
    n = noise(c, 7, seed + 1) * 0.55 + noise(c, 3, seed + 2) * 0.45
    crack = np.abs(noise(c, 16, seed + 3) - 0.5)
    pl = np.where(n > 0.66, 2, 1)
    pl = np.where(band < 0.12, 0, pl)
    pl = np.where((band >= 0.12) & (band < 0.22) & (n > 0.45), 2, pl)
    pl = np.where(crack < 0.02, 0, pl)
    for i, name in enumerate(['deep', 'dark', 'stone']):
        c.fill_mask(m & (pl == i), name, TAG_WALL)
