#!/usr/bin/env python3
"""A picture of every screen of the room, as the game draws it, stitched 3 x 2 (with the
camera screen by screen, so each is exactly one of the six).

    tools/screens.py OUTDIR [frame] [extra game args...]

Drops you on a standing spot in each screen, lets the given number of frames run (the
camera snaps; the air and the creatures settle a little), and keeps the frame. The sheet is
OUTDIR/sheet.png; each screen is OUTDIR/<letter>.png."""
import os, subprocess, sys
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SPOTS = {'A': (17, 16), 'B': (62, 13), 'C': (103, 19), 'D': (16, 37), 'E': (48, 37), 'F': (101, 32)}

def shoot(out, frame, extra):
    os.makedirs(out, exist_ok=True)
    for k, (tx, ty) in SPOTS.items():
        d = os.path.join(out, 'tmp_' + k)
        os.makedirs(d, exist_ok=True)
        cmd = [os.path.join(ROOT, 'build', 'game'), '--at', '%d,%d' % (tx, ty), '--shots', str(frame),
               '--out', d, '--scale', '1', '--mute', '--camera', 'screens'] + extra
        for attempt in range(2):
            r = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT, env=dict(os.environ, DISPLAY=os.environ.get('DISPLAY', ':99')))
            if r.returncode == 0: break
        src = os.path.join(d, 'f%04d.png' % frame)
        os.replace(src, os.path.join(out, k + '.png'))
        os.rmdir(d)
    ims = {k: Image.open(os.path.join(out, k + '.png')) for k in SPOTS}
    w, h = ims['A'].size
    sheet = Image.new('RGB', (w * 3 + 4, h * 2 + 2), (255, 0, 255))
    for i, k in enumerate('ABCDEF'):
        sheet.paste(ims[k], ((i % 3) * (w + 2), (i // 3) * (h + 2)))
    sheet.save(os.path.join(out, 'sheet.png'))
    return sheet

if __name__ == '__main__':
    out = sys.argv[1]
    frame = int(sys.argv[2]) if len(sys.argv) > 2 else 4
    shoot(out, frame, sys.argv[3:])
