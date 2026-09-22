#!/usr/bin/env python3
"""Render every sprite in src/sprites.c to a sheet, so the art can be looked at before it is
in the game. Each sprite is drawn three ways: as authored (fully lit), under a warm band of
light, and under a dim one, snapped to the palette the way the composite does it.

    python3 tools/sprites.py [out.png] [scale]"""
import re, sys, os
from PIL import Image, ImageDraw

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PAL = {}
src = open(os.path.join(ROOT, "src/render.c")).read()
for name, r, g, b in re.findall(r"\[PL_(\w+)\]\s*=\s*\{\s*(\d+),\s*(\d+),\s*(\d+)", src):
    PAL[name] = (int(r), int(g), int(b))
LEGEND = {'0': 'VOID', '1': 'DEEP', '2': 'DARK', '3': 'STONE', '4': 'STONEL', '5': 'STONEH',
          'w': 'WARMD', 'W': 'WARM', 'a': 'AMBER', 'A': 'AMBERH', 'c': 'COOLD', 'C': 'COOLM',
          'g': 'CITY', 'G': 'CITYH', 'b': 'BONE', 'u': 'WATER', 'U': 'WATERL', 'r': 'ACCENT', 'd': 'WATERD'}

def sprites():
    s = open(os.path.join(ROOT, "src/sprites.c")).read()
    rows = {n: re.findall(r'"([^"]*)"', body) for n, body in
            re.findall(r"static const char \*const (\w+)\[\] = \{(.*?)\};", s, re.S)}
    out = []
    for name, w, h, r in re.findall(r"const Sprite (\w+) = \{ (\d+), (\d+), (\w+) \};", s):
        out.append((name, int(w), int(h), rows[r]))
    return out

def nearest(c):
    best, bd = None, 1e9
    for v in PAL.values():
        e = 0.30 * (c[0] - v[0]) ** 2 + 0.59 * (c[1] - v[1]) ** 2 + 0.11 * (c[2] - v[2]) ** 2
        if e < bd: bd, best = e, v
    return best

def lit(c, band):          # as the composite: albedo * (ambient + band * warm tint * 1.25), snapped
    amb = (0.050, 0.054, 0.090); warm = (1.0, 0.80, 0.52)
    return nearest(tuple(min(255, c[i] * (amb[i] + band * warm[i] * 1.25)) for i in range(3)))

if __name__ == "__main__":
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "shots/sprites.png")
    S = int(sys.argv[2]) if len(sys.argv) > 2 else 8
    sp = [s for s in sprites() if s[1] <= 24]     # the door is looked at in the game
    pad, maxw = 4, 150
    # flow layout: each sprite takes its own width x3, rows wrap at maxw sprite pixels
    place, x, y, rowh = [], pad, pad, 0
    for name, w, h, rows in sp:
        cw = w * 3 + pad * 3
        if x + cw > maxw: x, y, rowh = pad, y + rowh + 6, 0
        place.append((x, y)); x += cw + pad; rowh = max(rowh, h + 2)
    img = Image.new("RGB", (maxw * S, (y + rowh + 6) * S), PAL["VOID"])
    d = ImageDraw.Draw(img)
    for (name, w, h, rows), (ox, oy) in zip(sp, place):
        for k, band in enumerate((None, 0.75, 0.25)):
            bx = ox + k * (w + pad)
            d.rectangle([bx * S - 1, oy * S - 1, (bx + w) * S, (oy + h) * S], outline=(26, 25, 40))
            for yy, row in enumerate(rows):
                for xx, ch_ in enumerate(row):
                    if ch_ == '.': continue
                    c = PAL[LEGEND[ch_]]
                    if band is not None: c = lit(c, band)
                    d.rectangle([(bx + xx) * S, (oy + yy) * S, (bx + xx + 1) * S - 1, (oy + yy + 1) * S - 1], fill=c)
        d.text((ox * S, (oy + h) * S + 3), name.replace("SPR_", "").lower(), fill=(120, 116, 140))
    img.save(out); print(out, img.size, len(sp), "sprites")
