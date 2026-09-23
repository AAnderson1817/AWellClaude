#!/usr/bin/env python3
"""Compile the big art (art/*.png) into the game: src/art_data.c, one byte array per file.

The PNGs are the source; the game decodes them from memory once at startup (raylib reads
PNG), so nothing is streamed from disk (the anti-pattern list) and the pictures stay small.
The alpha of each pixel is its tag for the composite: 255 far wall, 254 a drawn thing,
253 stone; 0 is nothing."""
import os, glob

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
files = sorted(glob.glob(os.path.join(ROOT, 'art', '*.png')))
out = ['// art_data.c -- the big backdrop pictures, as PNG bytes. Written by tools/art/embed.py from',
       '// art/*.png, which are written by the scripts in tools/art. Do not edit by hand.',
       '#include "aw.h"', '']
for f in files:
    name = 'ART_' + os.path.splitext(os.path.basename(f))[0].upper()
    data = open(f, 'rb').read()
    out.append('const unsigned char %s[%d] = {' % (name, len(data)))
    for i in range(0, len(data), 24):
        out.append('    ' + ','.join('%d' % b for b in data[i:i + 24]) + ',')
    out.append('};')
    out.append('const int %s_LEN = %d;' % (name, len(data)))
    out.append('')
# veins: one path per line, "x,y x,y ..." in room px; as x, y pairs with -1 ending each path
for f in sorted(glob.glob(os.path.join(ROOT, 'art', '*.veins'))):
    name = 'VEINS_' + os.path.splitext(os.path.basename(f))[0].upper()
    vals = []
    for line in open(f):
        pts = [p.split(',') for p in line.split()]
        if len(pts) < 2: continue
        for x, y in pts: vals += [int(x), int(y)]
        vals.append(-1)
    out.append('const i16 %s[%d] = {' % (name, max(len(vals), 1)))
    for i in range(0, len(vals), 20):
        out.append('    ' + ','.join(str(v) for v in vals[i:i + 20]) + ',')
    out.append('};')
    out.append('const int %s_LEN = %d;' % (name, len(vals)))
    out.append('')
    files.append(f)
# tables: rows of whitespace-separated numbers, all of them in one flat array; the reader
# knows the row length
for f in sorted(glob.glob(os.path.join(ROOT, 'art', '*.ints'))):
    name = 'INTS_' + os.path.splitext(os.path.basename(f))[0].upper()
    vals = [int(v) for line in open(f) for v in line.split()]
    out.append('const i16 %s[%d] = {' % (name, max(len(vals), 1)))
    for i in range(0, len(vals), 20):
        out.append('    ' + ','.join(str(v) for v in vals[i:i + 20]) + ',')
    out.append('};')
    out.append('const int %s_LEN = %d;' % (name, len(vals)))
    out.append('')
    files.append(f)
open(os.path.join(ROOT, 'src', 'art_data.c'), 'w', newline='\n').write('\n'.join(out))
print('embedded', ', '.join(os.path.basename(f) for f in files))
