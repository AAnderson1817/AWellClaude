#!/usr/bin/env python3
"""From every standable surface in the room, can a body get back to the forecourt, where it began?

The route checks say the designed climb works. This asks the opposite question of every
surface the map has: drop the wander bot there -- it does not know the route -- and leave
it to push and jump at random until it stands on the start tile again or the frames run
out. A surface no seed gets home from is reported and the command fails.

It is a heuristic in one direction only: a bot getting home is a fact, a bot failing is
a place to go and look. It is also the audit behind the rule that you are never
soft-locked without a way out -- the geometry is checked here; the way out (hold R) is
checked in route.py."""
import os, re, subprocess, sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GAME = os.path.join(ROOT, "build", "game")
SURF = re.compile(r"R(\d) (\S+) (shelf|stone) row\s+(\d+) cols\s+(\d+)-\s*(\d+)")
FRAMES = 600000         # nearly three hours of play per attempt; the sim is cheap, the bot is not clever
SEEDS = tuple(range(1, 11))  # a long way home from the basin floor: the bot is lucky, not clever

def game(*args):
    r = subprocess.run([GAME, *args, "--mute"], capture_output=True, text=True, cwd=ROOT)
    if r.returncode < 0:  # display contention, as in probe.py: once more
        r = subprocess.run([GAME, *args, "--mute"], capture_output=True, text=True, cwd=ROOT)
    if r.returncode:
        raise RuntimeError("game failed (exit %d): %s" % (r.returncode, r.stderr.strip()))
    return r.stdout

def surfaces(room):
    out = game("--labels", "--frames", "1", "--nodraw", "--room", str(room))
    return [(tag, int(row), int(c0), int(c1))
            for r, tag, kind, row, c0, c1 in SURF.findall(out) if int(r) == room]

def home(room, tx, ty, seed):
    out = game("--wander", str(seed), "--at", "%d,%d" % (tx, ty), "--room", str(room),
               "--frames", str(FRAMES))
    m = re.search(r"HOME reached \((\d+)\)", out)
    return int(m.group(1)) if m else None

def check(job):
    room, tag, row, c0, c1 = job
    tx, ty = (c0 + c1) // 2, row - 1          # standing IN the tile above the surface
    for seed in SEEDS:
        f = home(room, tx, ty, seed)
        if f: return job, seed, f
    return job, None, None

if __name__ == "__main__":
    jobs = [(room, *s) for room in range(1) for s in surfaces(room)]
    with ThreadPoolExecutor(max_workers=6) as ex:
        results = list(ex.map(check, jobs))
    stuck = []
    for (room, tag, row, c0, c1), seed, f in results:
        where = "R%d %-3s row %2d cols %2d-%2d" % (room, tag, row, c0, c1)
        if f: print("%s  home in %5.0f s (seed %d)" % (where, f / 60.0, seed))
        else: print("%s  *** NO BOT GOT HOME" % where); stuck.append(where)
    print()
    print("%d surfaces; never home from: %s" % (len(results), ", ".join(stuck) if stuck else "none"))
    sys.exit(1 if stuck else 0)
