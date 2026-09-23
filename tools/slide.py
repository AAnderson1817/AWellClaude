#!/usr/bin/env python3
"""A camera slide as an animated picture: what the multiplane does, which a still cannot show.

    tools/slide.py OUT.gif TX,TY PLAN FIRST LAST [STEP] [SCALE]

Drops you at tile (TX, TY), plays PLAN (the --play syntax), and keeps every STEP-th frame
from FIRST to LAST, as the game draws them; the first and last are held a moment."""
import os, subprocess, sys, tempfile
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def frames(at, plan, first, last, step):
    want = list(range(first, last + 1, step))
    got = {}
    with tempfile.TemporaryDirectory() as d:
        for i in range(0, len(want), 16):                  # the game keeps at most 16 shots a run
            batch = want[i:i + 16]
            subprocess.run([os.path.join(ROOT, 'build', 'game'), '--at', at, '--play', plan,
                            '--shots', ','.join(map(str, batch)), '--out', d, '--scale', '1', '--mute'],
                           capture_output=True, cwd=ROOT, env=dict(os.environ, DISPLAY=os.environ.get('DISPLAY', ':99')))
            for f in batch:
                p = os.path.join(d, 'f%04d.png' % f)
                if os.path.exists(p): got[f] = Image.open(p).convert('RGB').copy()
    return [got[f] for f in want if f in got]


if __name__ == '__main__':
    out, at, plan, first, last = sys.argv[1], sys.argv[2], sys.argv[3], int(sys.argv[4]), int(sys.argv[5])
    step = int(sys.argv[6]) if len(sys.argv) > 6 else 2
    scale = int(sys.argv[7]) if len(sys.argv) > 7 else 2
    fr = [im.resize((im.width * scale, im.height * scale), Image.NEAREST) for im in frames(at, plan, first, last, step)]
    dur = [900] + [step * 1000 // 60] * (len(fr) - 2) + [1200]
    fr[0].save(out, save_all=True, append_images=fr[1:], duration=dur, loop=0)
    print(out, len(fr), 'frames')
