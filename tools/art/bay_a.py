#!/usr/bin/env python3
"""Bay A, the intake: the far wall of the first screen, made of the archive's parts
(tools/art/kit.py, claude/ARCHIVE.md).

The door is the largest cell: a ring inscribed with the catalogue, nine blades, a lens that
is awake. Two ribs frame it and lean in over it toward an arch out of sight above. The wall
behind is seed drawers in their courses, mostly asleep or dark -- this is the archive's
edge. To the right the mountain has pushed back in: raw rock over the stacks below the
index band, where the chasm drops toward the heart and the fallen rock climbs to the
passage; a third rib frames the way into the hall. Every vein runs down, into the chasm.

It is drawn into the whole room's picture by tools/art/archive.py; run alone it only shows
itself.

    tools/art/bay_a.py DIR    a look at bay A by itself, DIR/bay_a.png
"""
import math, os, sys
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from sculpt import TAG_WALL, TAG_THING, TAG_STONE
from paint import Canvas, h2
from kit import cell, stacks, rib, rock, noise, Veins, lit, arc_marks
from sculpt import Form

W, H = 320, 176                  # the whole first screen; its top-left is room px (0, 0)
DX, DY, DR, RING = 104.0, 88.0, 70.0, 10.0      # the door
FLOOR = 168                      # the floor's top (row 21)


def build(c=None, V=None):
    """Draw the bay into c (a canvas, or a view of the whole room's) and its veins into V."""
    c = c if c is not None else Canvas(W, H)
    V = V if V is not None else Veins()
    # ---- the fabric: seed drawers everywhere, the mountain over them on the right
    stacks(c, 0, 2, W, FLOOR, seed=1, alive=0.06, asleep=0.42,
           darken=lambda x, y: min(1.0, max(0.0, (x - 150) / 200.0)))
    n = noise(c, 18, 5)
    edge = 206 + 20 * n + 0.10 * (c.Y - 112)
    mountain = (c.X > edge) & (c.Y > 108 + 10 * noise(c, 11, 9))       # below the index band
    rock(c, mountain, seed=3)
    # the index: a frieze over the rock, moulded above and below, dentils under it; hall.c
    # carves the mural into its face
    band = (c.X > 200) & (c.Y >= 74) & (c.Y < 108)
    f = Form(c.w, c.h, c.ox, c.oy)
    f.slab(200, 74, W + 8, 108, 8, bevel=2)
    f.slab(198, 70, W + 8, 76, 10, bevel=1.5, base=2)
    f.slab(198, 106, W + 8, 111, 10, bevel=1.5, base=2)
    lit(c, f, tag=TAG_WALL)
    for x in range(201, W, 6):
        c.fill_mask(c.rect(x, 111, x + 4, 115), 'stone', TAG_WALL); c.fill_mask(c.rect(x, 114, x + 4, 115), 'deep', TAG_WALL)
    # ---- the door: the largest cell, awake
    cell(c, DX, DY, DR, 'living', seed=-math.pi / 2, blades=9, ring=RING)
    d, ang = c.polar(DX, DY)
    for k in range(32):                                                   # the ring's stones, and the catalogue cut in them
        a = k * math.tau / 32
        for r in np.arange(DR, DR + RING, 0.5): c.put(DX + r * math.cos(a), DY + r * math.sin(a), 'deep')
        arc_marks(c, DX, DY, DR + 3.5, a + 0.03, a + math.tau / 32 - 0.03, k * 7 + 1)
        arc_marks(c, DX, DY, DR + 6.5, a + 0.03, a + math.tau / 32 - 0.03, k * 7 + 2)
    for k in range(12):                                                   # glass in the ring
        a = -math.pi / 2 + k * math.tau / 12
        x, y = DX + (DR + 5) * math.cos(a), DY + (DR + 5) * math.sin(a)
        c.put(x, y, 'deep'); c.glow(x, y, 'city' if k == 0 else 'coolM')
    # the blades' midribs: lines of light from the rim to the lens, the door reading you in
    for i in range(9):
        am = -math.pi / 2 + i * math.tau / 9 + 0.6
        for r in np.arange(DR - 3, 20, -1.0):
            a = am + (DR - r) * 0.013
            if int(r) % 3 == 0: c.glow(DX + r * math.cos(a), DY + r * math.sin(a), 'coolD' if int(r) % 9 else 'coolM')
    # ---- the ribs: two framing the door, leaning in toward the arch over it; a third at the
    # way into the hall. Their veins run down to the floor and along it to the chasm.
    floor_run = [(x, 165.5) for x in range(12, 196, 2)] + [(196, 170), (200, 176)]
    rib(c, 3, -20, FLOOR, 18, lean=16, root_side=1, seed=0.3, veins=V, vein_to=[(12, 165.5)] + floor_run[1:])
    rib(c, 186, -20, FLOOR, 18, lean=-16, root_side=-1, seed=1.7, veins=V, vein_to=[(195, 165.5), (196, 170), (200, 176)])
    rib(c, 298, -20, 112, 16, lean=-12, root_side=-1, seed=2.9)
    # the ring's own vein, round it and down its foot into the floor's
    ring_pts = [(DX + (DR + RING - 1.5) * math.cos(a), DY + (DR + RING - 1.5) * math.sin(a))
                for a in np.linspace(-math.pi / 2, math.pi / 2 - 0.05, 60)]
    V.add(c, ring_pts + [(DX + 2, 165.5), (150, 165.5), (196, 165.5), (196, 170), (200, 176)], lit_every=6)
    ring_l = [(DX + (DR + RING - 1.5) * math.cos(a), DY + (DR + RING - 1.5) * math.sin(a))
              for a in np.linspace(-math.pi / 2, -math.pi * 1.5 + 0.05, 60)]
    V.add(c, ring_l + [(DX - 2, 165.5), (DX + 2, 165.5)], lit_every=6)
    # ---- the living: roots down from the roof over the stacks and the rock, moss where light is
    roots = [(Canvas.bez((240, 22), (226, 52), (262, 64), (236, 118), 40), 4.5),
             (Canvas.bez((236, 60), (218, 76), (222, 96), (206, 120), 30), 2.4),
             (Canvas.bez((276, 22), (284, 44), (262, 58), (274, 100), 36), 3.2),
             (Canvas.bez((212, 22), (204, 40), (222, 52), (212, 70), 24), 2.2),
             (Canvas.bez((250, 90), (266, 104), (290, 104), (306, 124), 30), 2.0)]
    for k, (pts, r0) in enumerate(roots):
        c.root(pts, r0, 0.9, seed=k, moss=0.5)
        for j in range(4, len(pts) - 2, 7):                                # rootlets
            x, y = pts[j]
            c.stroke([(x, y), (x + (6 if j % 2 else -6), y + 5), (x + (8 if j % 2 else -9), y + 11)], 0.7, 0.4, 'warmD')
    low = (d > DR - 2) & (d < DR + RING + 2) & (c.Y > DY + 44)
    c.moss(low & (noise(c, 3, 21) < 0.5), 0.6, 11)
    for px in (12, 195):                                                  # on the plinths
        c.moss((abs(c.X - px) < 11) & (abs(c.Y - 160) < 3) & (noise(c, 2, px) < 0.55), 0.7, px)
    return c, V


if __name__ == '__main__':
    c, V = build()
    c.preview(os.path.join(sys.argv[1], 'bay_a.png'))
    print('bay a:', len(V.paths), 'veins')
