#!/usr/bin/env python3
"""The colossus: one of the tall ones, seated in profile against the hall's back wall, facing
the undercroft, its forearm held out low over the hall with the palm open. Two screens tall:
the head is in the upper screen (B), the lap and shins on the hall floor (E), the feet in
the basin. See claude/LORE.md section 5.

The canvas starts at tile (36, 1) of the room: local px = room px - (288, 8). The surfaces
you stand on -- the back of the hand, the bands of the forearm, the elbow, the band on the
upper arm, the shoulder, the lap -- are flat on top and sit exactly on the tile rows the map carves
('X' in tools/antechamber.py); STANDS below is that list, and the map is built from it.
"""
import os, sys
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from sculpt import Form, shade, quantise, to_rgba, save, blur, TAG_WALL, TAG_THING, TAG_STONE

W, H = 320, 336
OX, OY = 36, 1                     # tile of the canvas's top-left

def T(tx, ty):                     # tile corner to local px
    return (tx - OX) * 8, (ty - OY) * 8

# (x0, x1, row): tiles you can stand on, the top of tile `row` being the surface
STANDS = [(40, 45, 26),            # the back of the hand
          (46, 49, 24), (50, 53, 22),   # the forearm's bands
          (54, 57, 20),            # the elbow
          (57, 60, 17),            # the band on the upper arm
          (60, 64, 14),            # the shoulder
          (60, 72, 25)]            # the lap

def build(throne=True):
    f = Form(W, H)
    # ---- the seat, behind everything: a block, and a high back to it unless the keeper sits
    # in its cell, where the cell is what is behind it (tools/art/archive.py)
    if throne:
        f.slab(248, 88, 320, 336, 10, bevel=4, base=0, part=9)
        f.slab(238, 214, 320, 336, 14, bevel=3, base=0, part=9)      # the seat's front
    else:
        # a drum of their stone, the keeper's seat, banded; kept under the thigh's height,
        # so the thigh stays in front
        f.slab(244, 212, 312, 338, 10, bevel=4, base=0, part=9)
        f.slab(240, 212, 316, 222, 10, bevel=2, base=1, part=9)      # its cap
        for yb in (246, 290):
            f.slab(242, yb, 314, yb + 6, 4, bevel=1.5, base=9, part=9)
    # ---- the body
    # torso: upright, robed, a little forward at the chest
    f.poly([(190, 104), (184, 150), (186, 196), (266, 214), (272, 150), (262, 104)], 30, bevel=10, base=8, part=2)
    # the hood falls from the back of the head to the shoulders
    f.poly([(188, 10), (236, 8), (262, 40), (270, 108), (244, 116), (222, 84), (206, 58)], 22, bevel=8, base=10, part=3)
    # neck, long, ringed
    f.capsule(206, 58, 212, 104, 15, 17, base=16, part=4)
    for y in range(64, 102, 6):
        f.groove([(190, y), (230, y + 3)], width=1.1, depth=2.5)
    # the head: an egg, tipped forward to look down at the fire; the face is its smooth front
    f.ellipsoid(176, 42, 54, 32, 30, base=18, part=5, rot=-0.32)
    f.ellipsoid(140, 58, 26, 16, 20, base=24, part=5, rot=-0.5)      # the fall of the face to the chin
    # shoulders
    f.ellipsoid(212, 112, 34, 15, 22, base=20, part=2)
    # the lap: thigh, knee
    f.capsule(272, 212, 202, 212, 20, 19, base=14, part=6)
    f.ellipsoid(200, 214, 22, 21, 22, base=16, part=6)
    # the shin, robed to just above the water, and the foot on the basin floor
    f.capsule(200, 226, 190, 318, 18, 15, base=12, part=7)
    f.poly([(176, 232), (222, 230), (224, 276), (168, 280)], 18, bevel=6, base=14, part=7)   # the robe's fall over the shin
    f.capsule(196, 322, 132, 328, 9, 7, base=10, part=8)
    # ---- the near arm: upper arm from the shoulder to the elbow, a band on it you can stand on
    f.capsule(204, 112, 164, 160, 15, 14, base=34, part=10)
    # the band: flat on top at the surface row
    ax0, ay = T(57, 17)
    f.slab(ax0 - 2, ay, ax0 + 34, ay + 10, 8, bevel=2, base=44, part=11)
    # the shoulder: a pauldron over it, flat on top at its surface row
    sx0, sy = T(60, 14); sx1, _ = T(65, 14)
    f.slab(sx0 + 1, sy, sx1 - 1, sy + 13, 8, bevel=3, base=46, part=11)
    f.groove([(sx0 + 3, sy + 5), (sx1 - 3, sy + 5)], width=0.8, depth=1.5)
    # the elbow
    ex0, ey = T(54, 20)
    f.ellipsoid(ex0 + 18, ey + 12, 18, 13, 16, base=36, part=10)
    # the forearm: one long tapering limb from the elbow to the wrist, and on it the bands, each
    # a drum flat on top at its surface, stepping down to the wrist like a stair
    f.capsule(ex0 + 16, ey + 14, 58, 212, 15, 11, base=32, part=10)
    for (x0, x1, row) in STANDS[1:3]:
        lx0, ly = T(x0, row); lx1, _ = T(x1 + 1, row)
        f.slab(lx0 + 1, ly, lx1 + 1, ly + 22, 12, bevel=4, base=38, part=12)
        f.groove([(lx0 + 3, ly + 4), (lx1 - 1, ly + 4)], width=0.8, depth=1.5)
        f.groove([(lx0 + 3, ly + 17), (lx1 - 1, ly + 17)], width=0.8, depth=1.5)
    # the hand: the back of it flat, the palm open toward the undercroft, six long fingers
    # spread and falling, jointed once
    hx0, hy = T(40, 26); hx1, _ = T(46, 26)
    f.slab(hx0 + 2, hy, hx1 + 4, hy + 16, 12, bevel=5, base=38, part=13)
    f.ellipsoid(hx0 + 8, hy + 12, 14, 10, 12, base=38, part=13)      # the heel of the palm
    for k in range(6):
        a0 = (hx0 + 4 + k * 2.0, hy + 4 + k * 2.4)
        a1 = (hx0 - 18 + k * 2.6, hy + 14 + k * 4.2)
        a2 = (hx0 - 26 + k * 3.8, hy + 32 + k * 3.6)
        r = 4.2 - k * 0.25
        f.capsule(a0[0], a0[1], a1[0], a1[1], r, r * 0.9, base=40 - k * 1.5, part=14)
        f.capsule(a1[0], a1[1], a2[0], a2[1], r * 0.9, r * 0.6, base=40 - k * 1.5, part=14)
    # ---- carving: robe folds, the hood's edge, the eye sockets
    for x in (204, 222, 240, 256):
        f.groove([(x, 124), (x - 4, 170), (x - 2, 200)], width=1.0, depth=2.5)
    for x in (184, 196, 208):
        f.groove([(x + 6, 236), (x, 276)], width=1.0, depth=2.0)
    f.groove([(206, 58), (222, 84), (244, 116)], width=1.2, depth=3)
    return f


def paint(f):
    b, m = shade(f, light=(-0.45, -0.8, 0.6))
    ramp = ['deep', 'dark', 'stone', 'stoneL', 'stoneH']
    q = quantise(b * 0.92, ramp, dither=0.18)
    names = [(ramp[i], q == i) for i in range(len(ramp))]
    rgba = to_rgba(names, m, TAG_THING)
    # the hand and its fingers are stone to the composite: they take a rim from whatever
    # light is near them -- the camp fire, once it is lit (LORE.md section 5)
    rgba[m & np.isin(f.part, (13, 14)), 3] = TAG_STONE
    # the standing tops: one line of the lit stone, so the climb reads in any light
    for (x0, x1, row) in STANDS:
        lx0, ly = T(x0, row); lx1, _ = T(x1 + 1, row)
        for x in range(max(0, lx0), min(W, lx1)):
            if m[ly, x]:
                rgba[ly, x, :3] = (151, 141, 196)
                rgba[ly, x, 3] = TAG_STONE
            if ly + 1 < H and m[ly + 1, x]:
                rgba[ly + 1, x, 3] = TAG_STONE
    return rgba


if __name__ == '__main__':
    # it is drawn into the room's picture by tools/art/archive.py; alone, only a look at it
    out = os.path.join(sys.argv[1], 'colossus.png')
    save(paint(build(throne=False)), out, scale=3)
    print('wrote', out)
