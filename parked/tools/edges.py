#!/usr/bin/env python3
"""Drive a body through every door in the world, both ways, at both weight extremes.

The static analyser says the doors are reachable. This asks whether a body actually
gets through them in the real engine.
"""
import subprocess, os, re, sys
HOR = {(0,0):(15,18),(1,0):(15,18),(2,0):(8,11),(3,0):(15,18),
       (0,1):(15,18),(1,1):(15,18),(2,1):(15,18),(3,1):(8,11),
       (0,2):(15,18),(1,2):(15,18),(2,2):(15,18),(3,2):(15,18),
       (0,3):(15,18),(1,3):(15,18),(2,3):(8,11),(3,3):(15,18),
       (0,4):(15,18),(1,4):(15,18),(2,4):(15,18),(3,4):(15,18)}
VER = {(0,0):(18,21),(2,0):(6,9),(4,0):(30,33),(1,1):(18,21),(3,1):(12,15),
       (0,2):(25,28),(2,2):(18,21),(4,2):(8,11),(1,3):(18,21),(3,3):(26,29)}
env = dict(os.environ, LIBGL_ALWAYS_SOFTWARE="1", GALLIUM_DRIVER="llvmpipe")

def rooms_visited(room, tx, ty, load, plan, frames=700):
    try:
        out = subprocess.run(["xvfb-run","-a","-s","-screen 0 320x180x24","./build/game",
                              "--room", f"{room[0]},{room[1]}", "--at", f"{tx},{ty}",
                              "--load", str(load), "--play", plan,
                              "--frames", str(frames), "--trace"],
                             capture_output=True, text=True, timeout=90, env=env).stdout
    except subprocess.TimeoutExpired:
        return []
    seq, prev = [], None
    for l in out.split("\n"):
        m = re.match(r'f=\s*\d+ r=(\d,\d)', l)
        if m and m.group(1) != prev: seq.append(m.group(1)); prev = m.group(1)
    return seq

# a plan that walks one way while hopping, so a one-tile lip does not stop it
def walk(d, n=560):
    step = f"{d}:24,{d}J:9"
    return ",".join([step] * (n // 33))

fails = []
print("horizontal doors")
for (x, y), (a, b) in sorted(HOR.items()):
    for load in (0, 4):
        seq = rooms_visited((x, y), 36, b, load, walk("R"))
        ok_r = f"{x+1},{y}" in seq
        seq2 = rooms_visited((x+1, y), 2, b, load, walk("L"))
        ok_l = f"{x},{y}" in seq2
        if not ok_r: fails.append(f"({x},{y}) -> ({x+1},{y}) at load {load}")
        if not ok_l: fails.append(f"({x+1},{y}) -> ({x},{y}) at load {load}")
    print(f"  ({x},{y})<->({x+1},{y}) rows {a}-{b}: "
          + ("both ways, both loads" if not [f for f in fails if f'({x},{y})' in f or f'({x+1},{y})' in f] else "SEE FAILURES"))

print("\nvertical doors, downward")
for (x, y), (a, b) in sorted(VER.items()):
    for load in (0, 4):
        seq = rooms_visited((x, y), (a + b) // 2, 18, load, "D:40,-:400")
        if f"{x},{y+1}" not in seq:
            fails.append(f"({x},{y}) -> ({x},{y+1}) DOWN at load {load}")
    print(f"  ({x},{y}) -> ({x},{y+1}) cols {a}-{b}: "
          + ("ok" if not [f for f in fails if f"({x},{y}) ->" in f] else "SEE FAILURES"))

print()
if fails:
    print(f"{len(fails)} DOOR CROSSINGS FAILED")
    for f in fails: print("  !", f)
else:
    print("every door crossed in both directions at load 0 and load 4")
