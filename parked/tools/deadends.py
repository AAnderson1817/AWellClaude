#!/usr/bin/env python3
"""Start wandering bots in every room and see where they can get to.

A room you can enter but not leave is a dead end, and L7 says gates get added only
where testing shows people wander into one. This is that test, run empirically against
the real physics rather than against a model.
"""
import subprocess, os, sys, re
# Place each bot at a tile that actually exists in its room. Using the world's single
# start tile for every room drops most bots inside solid rock, where they cannot move
# and every room looks like a dead end.
_src = open('tools/traverse.py').read().split("def main()")[0]
_g = {}; exec(compile(_src, 'traverse', 'exec'), _g)
_rooms, _meta, _start = _g['parse']('rooms/world.txt')
def start_tile(x, y):
    ds = _g['doors'](_rooms, (x, y))
    for k in ('L', 'R', 'D', 'U'):
        if k in ds:
            for (c, r) in ds[k]:
                if _g['body_fits'](_rooms, (x, y), c, r): return c, r
    for r in range(20, 0, -1):
        for c in range(1, 39):
            if _g['body_fits'](_rooms, (x, y), c, r): return c, r
    return 2, 17
BOTS = int(sys.argv[1]) if len(sys.argv) > 1 else 8
FRAMES = int(sys.argv[2]) if len(sys.argv) > 2 else 90000
env = dict(os.environ, LIBGL_ALWAYS_SOFTWARE="1", GALLIUM_DRIVER="llvmpipe")
START = (2, 2)

reach = {}
for y in range(5):
    for x in range(5):
        u = [0] * 25
        for s in range(1, BOTS + 1):
            try:
                stx, sty = start_tile(x, y)
                out = subprocess.run(["./build/game", "--room", f"{x},{y}",
                                      "--at", f"{stx},{sty}",
                                      "--wander", str(s), "--frames", str(FRAMES)],
                                     capture_output=True, text=True, timeout=90, env=env).stdout
            except subprocess.TimeoutExpired:
                continue
            rows = [l for l in out.split("\n") if "##" in l or ".." in l]
            for ry, line in enumerate(rows[:5]):
                for rx, c in enumerate(re.findall(r"##|\.\.", line)[:5]):
                    if c == "##": u[ry * 5 + rx] = 1
        reach[(x, y)] = u

print(f"{BOTS} bots x {FRAMES} frames from each of 25 rooms\n")
print("from      reaches   can get back to the first room?")
stuck = []
for y in range(5):
    for x in range(5):
        u = reach[(x, y)]
        n = sum(u)
        back = u[START[1] * 5 + START[0]] == 1
        if not back and (x, y) != START: stuck.append((x, y))
        print(f"({x},{y})      {n:2}/25      {'yes' if back or (x,y)==START else 'NO'}")
print()
if stuck:
    print(f"{len(stuck)} rooms a wandering bot could not get out of:")
    for r in stuck: print("  ", r)
    print("(a bot has no plan -- some of these will be climbs a player would simply make)")
else:
    print("every room can be left")
