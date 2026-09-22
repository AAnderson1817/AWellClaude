"""Painting on a palette canvas, for the big backdrop pieces that are drawn rather than
sculpted: rings, discs, strokes, roots, vines, moss, marks. Every pixel is a palette name,
so what is painted is what the composite snaps to.

A Canvas holds three layers: the albedo (lit by the room), its tag for the composite, and
the glow (what gives its own light: drawn in the emissive layer, the city's colours)."""
import math
import numpy as np
from PIL import Image
from sculpt import PAL, TAG_WALL, TAG_THING, TAG_STONE, BAYER4

NAMES = list(PAL.keys())
IDX = {n: i for i, n in enumerate(NAMES)}


def h2(x, y, s=0):
    """A hash to 0..1, like the game's Hash2 in spirit."""
    v = (x * 374761393 + y * 668265263 + s * 2246822519) & 0xFFFFFFFF
    v = (v ^ (v >> 13)) * 1274126177 & 0xFFFFFFFF
    return ((v ^ (v >> 16)) & 0xFFFF) / 65535.0


class Canvas:
    def __init__(self, w, h):
        self.w, self.h = w, h
        self.a = np.full((h, w), -1, np.int16)      # albedo, palette index; -1 nothing
        self.t = np.zeros((h, w), np.uint8)          # tag
        self.g = np.full((h, w), -1, np.int16)      # glow
        yy, xx = np.mgrid[0:h, 0:w]
        self.X, self.Y = xx + 0.5, yy + 0.5

    # ---- raw
    def put(self, x, y, name, tag=TAG_THING):
        x, y = int(x), int(y)
        if 0 <= x < self.w and 0 <= y < self.h:
            self.a[y, x] = IDX[name]; self.t[y, x] = tag

    def glow(self, x, y, name):
        x, y = int(x), int(y)
        if 0 <= x < self.w and 0 <= y < self.h:
            self.g[y, x] = IDX[name]

    def fill_mask(self, m, name, tag=TAG_THING):
        self.a[m] = IDX[name]; self.t[m] = tag

    def glow_mask(self, m, name):
        self.g[m] = IDX[name]

    def clear_mask(self, m):
        self.a[m] = -1; self.t[m] = 0

    # ---- shapes, as masks
    def polar(self, cx, cy):
        dx, dy = self.X - cx, self.Y - cy
        return np.hypot(dx, dy), np.arctan2(dy, dx)

    def disc(self, cx, cy, r):
        return np.hypot(self.X - cx, self.Y - cy) < r

    def ring(self, cx, cy, r0, r1):
        d = np.hypot(self.X - cx, self.Y - cy)
        return (d >= r0) & (d < r1)

    def rect(self, x0, y0, x1, y1):
        return (self.X >= x0) & (self.X < x1) & (self.Y >= y0) & (self.Y < y1)

    def poly(self, pts):
        from PIL import ImageDraw
        im = Image.new('L', (self.w, self.h), 0)
        ImageDraw.Draw(im).polygon([(float(x), float(y)) for x, y in pts], fill=255)
        return np.array(im) > 0

    # ---- strokes
    def stroke(self, pts, r0, r1=None, name='warmD', tag=TAG_THING, shade=None, glow=None):
        """A tapering stroke along a polyline: roots, vines, pipes. shade=(lit, dark) colours
        its upper-left and lower-right edges so it reads round."""
        r1 = r0 if r1 is None else r1
        L = [0.0]
        for (x0, y0), (x1, y1) in zip(pts, pts[1:]): L.append(L[-1] + math.hypot(x1 - x0, y1 - y0))
        tot = max(L[-1], 1e-6)
        for i, ((x0, y0), (x1, y1)) in enumerate(zip(pts, pts[1:])):
            n = max(int(math.hypot(x1 - x0, y1 - y0) * 2), 1)
            for k in range(n + 1):
                t = k / n
                x, y = x0 + (x1 - x0) * t, y0 + (y1 - y0) * t
                f = (L[i] + (L[i + 1] - L[i]) * t) / tot
                r = r0 + (r1 - r0) * f
                ri = int(math.ceil(r)) + 1
                for yy in range(int(y) - ri, int(y) + ri + 1):
                    for xx in range(int(x) - ri, int(x) + ri + 1):
                        dx, dy = xx + 0.5 - x, yy + 0.5 - y
                        d = math.hypot(dx, dy)
                        if d > r: continue
                        c = name
                        if shade and r >= 1.2:
                            if dx + dy < -r * 0.55: c = shade[0]
                            elif dx + dy > r * 0.6: c = shade[1]
                        if glow: self.glow(xx, yy, glow)
                        else: self.put(xx, yy, c, tag)

    @staticmethod
    def bez(p0, p1, p2, p3, n=24):
        pts = []
        for i in range(n + 1):
            t = i / n; u = 1 - t
            pts.append((u * u * u * p0[0] + 3 * u * u * t * p1[0] + 3 * u * t * t * p2[0] + t * t * t * p3[0],
                        u * u * u * p0[1] + 3 * u * u * t * p1[1] + 3 * u * t * t * p2[1] + t * t * t * p3[1]))
        return pts

    def root(self, pts, r0, r1, seed=0, moss=0.0):
        """A root: dark core, lit upper edge, a knot now and then, moss on its top."""
        self.stroke(pts, r0, r1, 'warmD', shade=('warm', 'deep'))
        if moss > 0:
            for i, (x, y) in enumerate(pts):
                if h2(i, seed, 3) < moss:
                    r = r0 + (r1 - r0) * i / max(len(pts) - 1, 1)
                    self.put(x, y - r - 0.5, 'moss' if h2(i, seed, 5) < 0.6 else 'mossL')

    def vine(self, pts, seed=0, leaf=0.35, flower=0.0):
        """A thin vine with leaves on alternating sides, and a flower now and then."""
        self.stroke(pts, 0.6, 0.5, 'mossD')
        for i, (x, y) in enumerate(pts):
            if h2(i, seed) < leaf:
                s = 1 if i % 2 else -1
                self.put(x + s, y, 'moss'); self.put(x + 2 * s, y + 1, 'mossL' if h2(i, seed, 2) < 0.4 else 'moss')
            if flower and h2(i, seed, 9) < flower:
                self.put(x, y + 1, 'roseL'); self.put(x + 1, y + 1, 'accent')

    def moss(self, m, density=0.5, seed=0):
        """Moss over whatever is under the mask, thicker where the mask is thick."""
        ys, xs = np.nonzero(m)
        for x, y in zip(xs, ys):
            v = h2(x, y, seed)
            if v < density * 0.55: self.put(x, y, 'mossD')
            elif v < density * 0.85: self.put(x, y, 'moss')
            elif v < density: self.put(x, y, 'mossL')

    # ---- out
    def rgba(self, layer='a'):
        src = self.a if layer == 'a' else self.g
        out = np.zeros((self.h, self.w, 4), np.uint8)
        for n, i in IDX.items():
            m = src == i
            out[m, :3] = PAL[n]
        if layer == 'a':
            out[..., 3] = np.where(src >= 0, self.t, 0)
        else:
            out[..., 3] = np.where(src >= 0, 255, 0)
        return out

    def save(self, stem):
        Image.fromarray(self.rgba('a'), 'RGBA').save(stem + '.png')
        Image.fromarray(self.rgba('g'), 'RGBA').save(stem + '_glow.png')

    def preview(self, path, scale=3, light=0.9):
        """Albedo under a flat light, glow over it: a look at the art itself, not the game."""
        a = self.rgba('a').astype(float); g = self.rgba('g')
        rgb = np.full((self.h, self.w, 3), (14, 12, 26), float)
        m = a[..., 3] > 0
        rgb[m] = a[m, :3] * light
        mg = g[..., 3] > 0
        rgb[mg] = g[mg, :3]
        im = Image.fromarray(rgb.clip(0, 255).astype(np.uint8), 'RGB')
        im.resize((self.w * scale, self.h * scale), Image.NEAREST).save(path)


def shade_ramp(b, ramp, dither=0.0, seed=0):
    """Brightness 0..1 to indices into ramp, ordered-dithered across the steps if asked."""
    h, w = b.shape
    th = BAYER4[np.arange(h)[:, None] % 4, np.arange(w)[None, :] % 4] - 0.5
    return np.clip(b * len(ramp) + th * dither, 0, len(ramp) - 1e-6).astype(int)
