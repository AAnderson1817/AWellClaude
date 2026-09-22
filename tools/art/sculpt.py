"""A small sculptor: carve 2.5D forms out of primitives, shade them as stone, snap to the
palette. The big backdrop pieces are made with it -- they are too large to draw a pixel at a
time, and a height field gives them form the way carving does.

A form is a height field over the canvas: each primitive (a slab, a capsule, an ellipsoid)
raises the surface where it covers, and the surface is the highest of them. Grooves cut into
it. Shading is from the surface's normal against a light from above and in front -- soft,
not a sun, so it reads as form in any light the room throws on it -- and a cavity term that
darkens creases. The result is quantised to a few of the palette's stones."""
import numpy as np
from PIL import Image

# the palette of the new look (render.c PAL), by name
PAL = {
    'void': (7, 6, 15), 'deep': (18, 15, 36), 'dark': (33, 27, 61), 'stone': (58, 49, 96),
    'stoneL': (94, 83, 144), 'stoneH': (151, 141, 196), 'violet': (107, 63, 160),
    'warmD': (74, 30, 34), 'warm': (154, 63, 36), 'amber': (240, 138, 44), 'peach': (255, 176, 122),
    'amberH': (255, 212, 106), 'coolD': (12, 48, 64), 'coolM': (22, 112, 106), 'city': (63, 214, 166),
    'cityH': (189, 251, 224), 'cyan': (127, 232, 255), 'waterD': (11, 33, 72), 'water': (26, 74, 138),
    'waterL': (78, 168, 216), 'mossD': (29, 59, 36), 'moss': (63, 122, 58), 'mossL': (140, 200, 90),
    'roseD': (106, 31, 74), 'accent': (224, 72, 126), 'roseL': (255, 158, 192), 'bone': (244, 236, 214),
}
TAG_WALL, TAG_THING, TAG_STONE = 255, 254, 253
BAYER4 = np.array([[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]) / 16.0 + 1 / 32.0


class Form:
    def __init__(self, w, h):
        self.w, self.h = w, h
        self.z = np.full((h, w), -1e9)         # height; -inf where nothing is
        self.part = np.zeros((h, w), np.int16)  # which primitive won, for per-part colour
        yy, xx = np.mgrid[0:h, 0:w]
        self.X, self.Y = xx + 0.5, yy + 0.5
        self.n = 0

    def _put(self, zz, mask, part):
        zz = np.where(mask, zz, -1e9)
        win = zz > self.z
        self.z = np.where(win, zz, self.z)
        self.part = np.where(win, part, self.part)

    def ellipsoid(self, cx, cy, rx, ry, depth, base=0.0, part=1, rot=0.0):
        c, s = np.cos(rot), np.sin(rot)
        dx, dy = self.X - cx, self.Y - cy
        u, v = (dx * c + dy * s) / rx, (-dx * s + dy * c) / ry
        r2 = u * u + v * v
        self._put(base + depth * np.sqrt(np.clip(1 - r2, 0, 1)), r2 < 1, part)

    def capsule(self, x0, y0, x1, y1, r0, r1=None, depth=None, base=0.0, part=1):
        r1 = r0 if r1 is None else r1
        vx, vy = x1 - x0, y1 - y0
        L2 = vx * vx + vy * vy
        t = np.clip(((self.X - x0) * vx + (self.Y - y0) * vy) / L2, 0, 1)
        px, py = x0 + vx * t, y0 + vy * t
        d = np.hypot(self.X - px, self.Y - py)
        r = r0 + (r1 - r0) * t
        depth = r if depth is None else depth
        self._put(base + depth * np.sqrt(np.clip(1 - (d / r) ** 2, 0, 1)), d < r, part)

    def slab(self, x0, y0, x1, y1, depth, bevel=3.0, base=0.0, part=1):
        """A block with rounded-off edges."""
        m = (self.X >= x0) & (self.X < x1) & (self.Y >= y0) & (self.Y < y1)
        e = np.minimum.reduce([self.X - x0, x1 - self.X, self.Y - y0, y1 - self.Y])
        self._put(base + depth * np.clip(e / bevel, 0, 1) ** 0.5, m, part)

    def poly(self, pts, depth, bevel=3.0, base=0.0, part=1):
        """A flat polygon with a bevel toward its edge."""
        from PIL import ImageDraw
        im = Image.new('L', (self.w, self.h), 0)
        ImageDraw.Draw(im).polygon(pts, fill=255)
        m = np.array(im) > 0
        d = _dist_inside(m)
        self._put(base + depth * np.clip(d / bevel, 0, 1) ** 0.5, m, part)

    def groove(self, pts, width=1.0, depth=2.0):
        """Cut a line into whatever is there."""
        for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
            vx, vy = x1 - x0, y1 - y0
            L2 = max(vx * vx + vy * vy, 1e-6)
            t = np.clip(((self.X - x0) * vx + (self.Y - y0) * vy) / L2, 0, 1)
            d = np.hypot(self.X - (x0 + vx * t), self.Y - (y0 + vy * t))
            cut = np.clip(1 - d / width, 0, 1) * depth
            self.z = np.where(self.z > -1e8, self.z - cut, self.z)

    def mask(self):
        return self.z > -1e8


def _dist_inside(m, it=24):
    """Approximate distance to the outside, in px, by erosion."""
    d = np.zeros(m.shape)
    cur = m.copy()
    for i in range(it):
        d += cur
        n = cur.copy()
        n[1:, :] &= cur[:-1, :]; n[:-1, :] &= cur[1:, :]
        n[:, 1:] &= cur[:, :-1]; n[:, :-1] &= cur[:, 1:]
        cur = n
    return d


def blur(a, r):
    k = 2 * r + 1
    p = np.pad(a, r, mode='edge')
    c = np.cumsum(np.cumsum(p, 0), 1)
    c = np.pad(c, ((1, 0), (1, 0)))
    return (c[k:, k:] - c[:-k, k:] - c[k:, :-k] + c[:-k, :-k]) / (k * k)


def shade(form, light=(-0.35, -0.75, 0.55), cavity=0.9):
    """0..1 brightness per pixel: the normal against a soft light, less in the creases."""
    m = form.mask()
    z = np.where(m, form.z, 0.0)
    # outside the form, let the height fall away so edges read as turning away
    zz = np.where(m, z, blur(z, 2) - 6)
    gx = np.zeros_like(zz); gy = np.zeros_like(zz)
    gx[:, 1:-1] = (zz[:, 2:] - zz[:, :-2]) * 0.5
    gy[1:-1, :] = (zz[2:, :] - zz[:-2, :]) * 0.5
    n = np.stack([-gx, -gy, np.ones_like(zz)], -1)
    n /= np.linalg.norm(n, axis=-1, keepdims=True)
    L = np.array(light, float); L /= np.linalg.norm(L)
    lam = np.clip((n * L).sum(-1), 0, 1)
    cav = np.clip((z - blur(z, 3)) * 0.25, -1, 0.5)
    b = 0.25 + 0.75 * lam + cavity * cav * 0.3
    return np.clip(b, 0, 1), m


def quantise(b, ramp, dither=0.0):
    """Brightness to a ramp of palette names, with an optional ordered dither across steps."""
    h, w = b.shape
    th = BAYER4[np.arange(h)[:, None] % 4, np.arange(w)[None, :] % 4] - 0.5
    q = np.clip(b * len(ramp) + th * dither, 0, len(ramp) - 1e-6).astype(int)
    return q


def to_rgba(idx_names, m, tag):
    h, w = m.shape
    out = np.zeros((h, w, 4), np.uint8)
    for (name, sel) in idx_names:
        out[sel & m, :3] = PAL[name]
    out[m, 3] = tag
    return out


def save(rgba, path, scale=1):
    im = Image.fromarray(rgba, 'RGBA')
    im.save(path)
    if scale > 1:
        big = im.resize((im.width * scale, im.height * scale), Image.NEAREST)
        bg = Image.new('RGBA', big.size, (14, 12, 26, 255))
        bg.alpha_composite(Image.fromarray(np.where(np.array(big)[..., 3:4] > 0, np.concatenate([np.array(big)[..., :3], np.full(np.array(big).shape[:2] + (1,), 255, np.uint8)], -1), 0).astype(np.uint8)))
        bg.save(path.replace('.png', '_x%d.png' % scale))
