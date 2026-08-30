#!/usr/bin/env python3
"""Run many wandering bots from different seeds and union what they reach.

The static analyser proves what a model can do. This proves what the actual physics
does, with the actual verbs, driven by something with no plan.
"""
import subprocess, sys, os, re
N = int(sys.argv[1]) if len(sys.argv) > 1 else 60
FRAMES = int(sys.argv[2]) if len(sys.argv) > 2 else 120000
env = dict(os.environ, LIBGL_ALWAYS_SOFTWARE="1", GALLIUM_DRIVER="llvmpipe")
union = [0] * 25
per = []
for s in range(1, N + 1):
    try:
        out = subprocess.run(["xvfb-run", "-a", "-s", "-screen 0 320x180x24",
                              "./build/game", "--wander", str(s), "--frames", str(FRAMES)],
                             capture_output=True, text=True, timeout=90, env=env).stdout
    except subprocess.TimeoutExpired:
        continue
    rows = [l for l in out.split("\n") if "##" in l or ".." in l]
    got = 0
    for y, line in enumerate(rows[:5]):
        cells = re.findall(r"##|\.\.", line)
        for x, c in enumerate(cells[:5]):
            if c == "##": union[y * 5 + x] = 1; got += 1
    per.append(got)
tot = sum(union)
print(f"{N} bots x {FRAMES} frames")
print(f"best single bot reached {max(per) if per else 0}/25, median {sorted(per)[len(per)//2] if per else 0}")
print(f"union reached {tot}/25")
for y in range(5):
    print("  " + " ".join("##" if union[y * 5 + x] else ".." for x in range(5)))
miss = [(x, y) for y in range(5) for x in range(5) if not union[y * 5 + x]]
if miss: print("  never reached:", miss)
