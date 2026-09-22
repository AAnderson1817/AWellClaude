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
open(os.path.join(ROOT, 'src', 'art_data.c'), 'w', newline='\n').write('\n'.join(out))
print('embedded', ', '.join(os.path.basename(f) for f in files))
