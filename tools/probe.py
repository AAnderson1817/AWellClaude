#!/usr/bin/env python3
"""Run the game headless with a scripted plan and report what the body did.

This exists because the last build's defining bug -- every one-way shelf passable
from above -- was invisible to every tool that checked the level and obvious to the
first person who stood on one. So the checks here are the things a person does:
stand, land, drop, jump through, walk into a wall."""
import subprocess, sys, re, os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LINE = re.compile(r"f=\s*(\d+) x=\s*([-\d.]+) y=\s*([-\d.]+) vx=\s*([-\d.]+) "
                  r"vy=\s*([-\d.]+) ground=(\d+) air=(\d+) coy=(\d+) buf=(\d+)")

def run(plan, at=None, frames=None):
    cmd = [os.path.join(ROOT, "build", "game"), "--play", plan, "--trace", "--nodraw"]
    if at:     cmd += ["--at", "%d,%d" % at]
    if frames: cmd += ["--frames", str(frames)]
    result = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT)
    if result.returncode:
        raise RuntimeError("Game probe failed (exit %d): %s" %
                           (result.returncode, result.stderr.strip()))
    out = result.stdout
    rows = []
    for m in LINE.finditer(out):
        f, x, y, vx, vy, g, a, c, b = m.groups()
        rows.append(dict(f=int(f), x=float(x), y=float(y), vx=float(vx),
                         vy=float(vy), ground=int(g), air=int(a),
                         coy=int(c), buf=int(b)))
    if not rows:
        raise RuntimeError("Game probe produced no trace rows: " + result.stderr.strip())
    return rows

def last(rows):  return rows[-1] if rows else None
def peak(rows):  return min(r["y"] for r in rows)
def tile(v):     return v / 8.0

if __name__ == "__main__":
    print(run(sys.argv[1])[-1])

# --- reachability by search ------------------------------------------------
# A hop is "reachable" if SOME way of playing it lands it. My first version of this
# always jumped from the same tile and never let go of the direction, and it called
# two perfectly good hops impossible -- which is the same failure as hand-timing
# them, just automated. So it varies where you start, when you jump, how long you
# hold the jump, and when you stop pushing.
def reach(start_cols, start_row, target_row, target_cols, direction,
          pres=(0, 4, 8, 12), holds=(6, 12, 20, 30), runs=(8, 18, 40)):
    key = {-1: "L", 1: "R", 0: ""}[direction]
    for sc in start_cols:
        for pre in pres:
            for hold in holds:
                for after in runs:
                    plan = ""
                    if pre and key: plan += "%s:%d," % (key, pre)
                    plan += "%sJ:%d," % (key, hold)
                    if key: plan += "%s:%d," % (key, after)
                    plan += "-:50"
                    for r in run(plan, at=(sc, start_row)):
                        if not r["ground"]: continue
                        if round((r["y"] + 11) / 8.0) != target_row: continue
                        if int(r["x"] // 8) in target_cols or int((r["x"] + 5) // 8) in target_cols:
                            return "from col %d: %s" % (sc, plan)
    return None
