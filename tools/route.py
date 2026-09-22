#!/usr/bin/env python3
"""Every hop of the room's climb, checked by searching input timings.

The route below is the one I designed. The point of this file is that it does not
trust me: a hop counts as makeable only if some combination of take-off point,
jump timing, jump length and when-you-stop-pushing actually lands it."""
import sys, os
from concurrent.futures import ThreadPoolExecutor
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from probe import reach, run

def onto(rows, row, cols, room=0):
    return any(r["ground"] and round((r["y"] + 11) / 8) == row
               and (int(r["x"] // 8) in cols or int((r["x"] + 5) // 8) in cols) for r in rows)

# From floating in the basin to the first tread of the giant stair: swim, then a surface
# jump. The search varies how long you swim and how long you hold the jump.
def water_to(key, at, row, cols):
    for pre in range(10, 160, 10):
        for hold in (6, 12, 20):
            plan = "-:90,%s:%d,%sJ:%d,%s:6,-:60" % (key, pre, key, hold, key)
            if onto(run(plan, at=at), row, cols): return "from the water: " + plan
    return None

# Walking the top: from the passage along the gallery -- over the shelf where the arm comes
# up through it -- to the far end of the sill, never off row 14.
def the_long_walk():
    rows = run("R:900", at=(19, 13))
    upto = [r for r in rows if r["x"] < 108 * 8]
    ok = len(upto) < len(rows) and all(r["ground"] and round((r["y"] + 11) / 8) == 14 for r in upto[2:])
    return "walked to col 108 without leaving row 14 (%d frames)" % len(upto) if ok else None

# The drop: off the end of the step, into the chasm, down to the undercroft's floor.
def the_chasm():
    e = run("R:30,-:200", at=(11, 13))[-1]
    return "fell to row %d at col %d" % (round((e["y"] + 11) / 8), e["x"] // 8) \
        if e["ground"] and round((e["y"] + 11) / 8) == 35 else None

# Starting over, from the worst place: on the basin floor, holding the stone that put you
# there. And the two small ones: the stone goes home; let go early and nothing happens.
def reset_from_the_deep():
    rows = run("-:4,X:1,-:4,R:70,-:200,H:90,-:60", at=(STONE[0] - 1, STONE[1]))
    if not any(r["wet"] and r["hold"] == 2 for r in rows): return None   # never got down there
    e = rows[-1]
    return "from the basin, heavy: at the start, hands empty" \
        if e["ground"] and e["hold"] == 0 and abs(e["x"] - (START[0] * 8 + 1)) < 1 and round((e["y"] + 11) / 8) == START[1] + 1 \
        else None
def reset_puts_the_stone_back():
    rows = run("-:4,X:1,-:10,H:90,-:40", at=(STONE[0], STONE[1]))
    home = (rows[0]["stoneX"], rows[0]["stoneY"])
    e = rows[-1]
    return "stone taken up, R held: stone back at %d,%d" % home \
        if rows[14]["hold"] == 2 and e["hold"] == 0 and abs(e["stoneX"] - home[0]) <= 1 and abs(e["stoneY"] - home[1]) <= 1 else None
def reset_let_go_early_does_nothing():
    rows = run("-:4,X:1,-:10,H:40,-:40", at=(STONE[0], STONE[1]))
    a, e = rows[14], rows[-1]
    return "R held 40 frames and let go: still there, still holding, fade back to 0" \
        if e["hold"] == 2 and abs(e["x"] - a["x"]) < 0.5 and e["fade"] == 0 \
        and max(r["fade"] for r in rows) > 0.3 else None

START = (7, 13)          # the tile you stand in at the start
STONE = (42, 34)         # where the basin's stone lies

RESET = [
 ("X0 hold R on the basin floor, heavy",        reset_from_the_deep),
 ("X1 hold R: the stone goes home",             reset_puts_the_stone_back),
 ("X2 let go of R early: nothing happens",      reset_let_go_early_does_nothing),
]

# (name, start cols, start row (the row you stand IN), target row (the tile stood ON),
#  target cols, direction)
ROUTE = [
 ("T0 the step     -> over the chasm",        range(8, 13), 13, 14, range(18, 24), +1),
 ("T1 the long walk, passage to sill",        the_long_walk),
 ("T2 the chasm, down to the undercroft",     the_chasm),
 ("U1 hall floor   -> the low corbel",        range(40, 46), 34, 32, range(41, 45),  0),
 ("U2 low corbel   -> the planks",            range(41, 45), 31, 29, range(36, 40), -1),
 ("U3 the planks   -> the back of the hand",  range(36, 40), 28, 26, range(40, 46), +1),
 ("U4 the hand     -> the forearm's band",    range(40, 46), 25, 24, range(46, 50), +1),
 ("U5 band         -> band",                  range(46, 50), 23, 22, range(50, 54), +1),
 ("U6 band         -> the elbow",             range(50, 54), 21, 20, range(54, 58), +1),
 ("U7 the elbow    -> the armlet",            range(54, 58), 19, 17, range(57, 61), +1),
 ("U8 the armlet   -> the gallery, through",  range(57, 61), 16, 14, range(56, 61),  0),
 ("U9 the elbow    -> the lap",               range(54, 58), 19, 25, range(60, 73), +1),
 ("S0 the basin    -> the first tread",       lambda: water_to("R", (92, 37), 34, range(97, 100))),
 ("S1 tread        -> tread",                 range(97, 100), 33, 31, range(100, 103), +1),
 ("S2 tread        -> tread",                 range(100, 103), 30, 28, range(103, 106), +1),
 ("S3 tread        -> tread",                 range(103, 106), 27, 25, range(106, 109), +1),
 ("S4 tread        -> tread",                 range(106, 109), 24, 22, range(109, 112), +1),
 ("S5 tread        -> tread",                 range(109, 112), 21, 19, range(112, 115), +1),
 ("S6 tread        -> the top tread",         range(112, 115), 18, 16, range(115, 119), +1),
 ("S7 the top tread -> the sill's end",       range(115, 119), 15, 14, range(106, 112), -1),
]

def check(h):
    if len(h) == 2:
        name, fn = h
        return name, fn()
    name, sc, sr, tr, tc, d = h
    return name, reach(sc, sr, tr, tc, d)

if __name__ == "__main__":
    with ThreadPoolExecutor(max_workers=6) as ex:
        results = list(ex.map(check, ROUTE + RESET))
    bad = []
    for name, plan in results:
        print("%-42s %s" % (name, plan if plan else "*** NO WAY OF PLAYING IT LANDS THIS"))
        if not plan: bad.append(name.split()[0])
    print()
    print("unmakeable hops:", ", ".join(bad) if bad else "none -- the antechamber closes")
    sys.exit(1 if bad else 0)
