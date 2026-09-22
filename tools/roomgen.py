#!/usr/bin/env python3
"""The room's authoring tool: draw the antechamber with shapes, write it out as text.

The game reads its level as text compiled into the binary (src/antechamber.c): the tile
map, the props grid, the city's zones and the backdrop pieces. At 120 x 44 that is too big
to keep straight by typing rows, so the room is drawn here -- rectangles of stone, runs of
shelf, stamped pieces for the hand-shaped parts -- and this writes the C. The C is still
the only thing the game reads, and it stays readable as a picture of the room.

    tools/roomgen.py            write src/antechamber.c and print the map by screens
    tools/roomgen.py --png F    also a picture of it, a pixel or four per tile
"""
import os, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SW, SH = 40, 22


class Room:
    def __init__(self, w, h):
        self.w, self.h = w, h
        self.t = [['.'] * w for _ in range(h)]       # tiles
        self.p = [['.'] * w for _ in range(h)]       # props
        self.zones = []                              # city stone, as tile rects
        self.feats = []                              # backdrop pieces: (kind, x, y, w, h, a)
        self.drafts = []                             # (x, y, vx, vy, radius) in tiles, pushed every step

    # ---- tiles
    def fill(self, x0, y0, x1, y1, c='#'):
        for y in range(max(0, y0), min(self.h - 1, y1) + 1):
            for x in range(max(0, x0), min(self.w - 1, x1) + 1):
                self.t[y][x] = c

    def clear(self, x0, y0, x1, y1):
        self.fill(x0, y0, x1, y1, '.')

    def shelf(self, x0, x1, y):
        self.fill(x0, y, x1, y, '-')

    def at(self, x, y, c):
        if 0 <= x < self.w and 0 <= y < self.h:
            self.t[y][x] = c

    def get(self, x, y):
        return self.t[y][x] if 0 <= x < self.w and 0 <= y < self.h else '#'

    def stamp(self, x0, y0, art, layer='t'):
        """Hand-shaped piece: ' ' leaves what is there, anything else is written."""
        g = self.t if layer == 't' else self.p
        for j, row in enumerate(art.strip('\n').split('\n')):
            for i, c in enumerate(row):
                if c != ' ' and 0 <= x0 + i < self.w and 0 <= y0 + j < self.h:
                    g[y0 + j][x0 + i] = c

    # ---- the other layers
    def prop(self, x, y, c):
        self.p[y][x] = c

    def city(self, x0, y0, x1, y1):
        self.zones.append((x0, y0, x1, y1))

    def feat(self, kind, x, y, w=1, h=1, a=0):
        self.feats.append((kind, x, y, w, h, a))

    def draft(self, x, y, vx, vy, r):
        self.drafts.append((x, y, vx, vy, r))

    # ---- checks
    def check(self):
        errs = []
        starts = sum(r.count('P') for r in self.t)
        if starts != 1: errs.append("%d start tiles" % starts)
        for y in range(self.h):
            for x in range(self.w):
                if self.t[y][x] == '*':
                    open_ = any(self.get(x + dx, y + dy) not in '#*' for dx in (-1, 0, 1) for dy in (-1, 0, 1))
                    if not open_: errs.append("seam at %d,%d is walled in" % (x, y))
        for x in range(self.w):
            if self.t[0][x] not in '#*' or self.t[self.h - 1][x] not in '#*':
                errs.append("the room is open at column %d" % x); break
        for y in range(self.h):
            if self.t[y][0] not in '#*' or self.t[y][self.w - 1] not in '#*':
                errs.append("the room is open at row %d" % y); break
        return errs

    # ---- out
    def c_source(self, header):
        q = lambda row: '    "%s",' % ''.join(row)
        out = [header, '#include "aw.h"', '']
        out.append('const char *const ROOM_MAP[RH] = {')
        for y, row in enumerate(self.t):
            out.append(q(row) + ('   // %d' % y if y % SH == 0 else ''))
        out.append('};')
        out.append('')
        out.append('const char *const ROOM_PROPS[RH] = {')
        for y, row in enumerate(self.p):
            out.append(q(row) + ('   // %d' % y if y % SH == 0 else ''))
        out.append('};')
        out.append('')
        out.append('const ZRect ROOM_CITY[] = {')
        for z in self.zones:
            out.append('    { %d, %d, %d, %d },' % z)
        out.append('    { -1, 0, 0, 0 },')
        out.append('};')
        out.append('')
        out.append('const Feature ROOM_FEATURES[] = {')
        for k, x, y, w, h, a in self.feats:
            out.append('    { %s, %d, %d, %d, %d, %d },' % (k, x, y, w, h, a))
        out.append('    { F_NONE, 0, 0, 0, 0, 0 },')
        out.append('};')
        out.append('')
        out.append('// The room\'s own slow drafts, in tiles and px per frame.')
        out.append('void RoomDrafts(void) {')
        for x, y, vx, vy, r in self.drafts:
            out.append('    AirPush(%.1ff * TS, %.1ff * TS, %.3ff, %.3ff, %.1ff);' % (x, y, vx, vy, r))
        out.append('}')
        out.append('')
        return '\n'.join(out)

    def preview(self):
        lines = []
        for y in range(self.h):
            if y % SH == 0:
                lines.append('    ' + ''.join(('|' if x % SW == 0 else ' ') if x % 10 else str(x // 10 % 10) for x in range(self.w)))
            row = ''
            for x in range(self.w):
                c = self.t[y][x]
                if c == '.' and self.p[y][x] != '.': c = self.p[y][x].lower() if self.p[y][x].isupper() else self.p[y][x]
                row += c
            lines.append('%3d %s' % (y, row))
        return '\n'.join(lines)

    def png(self, path, scale=4):
        from PIL import Image
        col = {'#': (70, 62, 110), '-': (170, 130, 80), '~': (40, 90, 170), '*': (250, 150, 50), ',': (60, 120, 60),
               'b': (50, 140, 60), 'o': (220, 80, 130), 'P': (255, 255, 255), '.': (14, 12, 26)}
        im = Image.new('RGB', (self.w, self.h))
        zone = [[0] * self.w for _ in range(self.h)]
        for x0, y0, x1, y1 in self.zones:
            for y in range(y0, y1 + 1):
                for x in range(x0, x1 + 1):
                    if 0 <= x < self.w and 0 <= y < self.h: zone[y][x] = 1
        for y in range(self.h):
            for x in range(self.w):
                c = col.get(self.t[y][x], (200, 200, 200))
                if self.t[y][x] == '.':
                    if zone[y][x]: c = (20, 34, 36)
                    if self.p[y][x] != '.': c = (150, 60, 150)
                elif self.t[y][x] == '#' and zone[y][x]: c = (80, 100, 110)
                im.putpixel((x, y), c)
        im = im.resize((self.w * scale, self.h * scale), Image.NEAREST)
        from PIL import ImageDraw
        d = ImageDraw.Draw(im)
        for k, x, y, w, h, a in self.feats:
            d.rectangle([x * scale, y * scale, (x + w) * scale - 1, (y + h) * scale - 1], outline=(90, 220, 170))
        for sx in range(1, self.w // SW):
            d.line([(sx * SW * scale, 0), (sx * SW * scale, self.h * scale)], fill=(255, 0, 255))
        for sy in range(1, self.h // SH):
            d.line([(0, sy * SH * scale), (self.w * scale, sy * SH * scale)], fill=(255, 0, 255))
        im.save(path)


def write(room, header):
    errs = room.check()
    for e in errs: print("ERROR:", e)
    src = room.c_source(header)
    open(os.path.join(ROOT, 'src', 'antechamber.c'), 'w', newline='\n').write(src)
    print(room.preview())
    if '--png' in sys.argv:
        room.png(sys.argv[sys.argv.index('--png') + 1])
    return not errs
