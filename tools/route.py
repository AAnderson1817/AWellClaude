#!/usr/bin/env python3
"""Every hop of the room's climb, checked by searching input timings.

The route below is the one I designed. The point of this file is that it does not
trust me: a hop counts as makeable only if some combination of take-off point,
jump timing, jump length and when-you-stop-pushing actually lands it."""
import sys, os
from concurrent.futures import ThreadPoolExecutor
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from probe import reach, run

def onto(rows, row, cols, room):
    return any(r["ground"] and r["room"] == room and round((r["y"] + 11) / 8) == row
               and (int(r["x"] // 8) in cols or int((r["x"] + 5) // 8) in cols) for r in rows)

# From floating in the water to an island top: swim, then a surface jump. The search
# varies how long you swim and how long you hold the jump.
def water_to(key, cols):
    for pre in range(40, 160, 12):
        for hold in (6, 12, 20):
            plan = "-:110,%s:%d,%sJ:%d,%s:6,-:60" % (key, pre, key, hold, key)
            if onto(run(plan, at=(22, 3), room=1), 6, cols, 1): return "from the water: " + plan
    return None

# The shaft. Down: stand on the shelf over it and hold Down; you should end up floating
# in the room below. Up: from the shelf in its throat, one jump should land you on the
# shelf over it in the room above.
def shaft_down():
    e = run("-:20,D:300", at=(22, 19))[-1]
    return "D from (22,19): room %d, floating" % e["room"] if e["room"] == 1 and e["wet"] else None
def shaft_up():
    e = run("-:10,J:30,-:60", at=(22, 0), room=1)[-1]
    return "J from the throat: room %d, row %.0f" % (e["room"], (e["y"] + 11) / 8) \
        if e["room"] == 0 and e["ground"] and round((e["y"] + 11) / 8) == 20 else None

# The two complaints from play. A plain vertical jump from the end of A2 or A3 must
# land on A1 -- no run-up, no steering. And a jump from A1 that fails to reach the
# floor above must come back down ONTO A1, not through it into the water; only Down
# held takes you through.
def standing_jump_to_throat(col):
    e = run("-:6,J:20,-:60", at=(col, 2), room=1)[-1]
    ok = e["room"] == 1 and e["ground"] and round((e["y"] + 11) / 8) == 1
    return "J from (%d,2): on A1" % col if ok else None
def failed_exit_lands_on_throat():
    e = run("-:6,J:6,-:80", at=(22, 0), room=1)[-1]
    if not (e["room"] == 1 and e["ground"] and round((e["y"] + 11) / 8) == 1 and not e["wet"]): return None
    w = run("-:6,J:6,-:6,D:120", at=(22, 0), room=1)[-1]
    return "short jump: back on A1; with Down held: in the water" if w["wet"] else None

ROUTE = [
 ("A  floor           -> lower left step",   range(6, 10), 19, 18, range(4, 6),   -1),
 ("B  lower step      -> the lip (4,17)",    range(4, 6),  17, 17, range(4, 5),   -1),
 ("C  the lip         -> upper left step",   range(4, 5),  16, 15, range(1, 4),   -1),
 ("D  upper left step -> shelf row 15",      range(1, 4),  14, 15, range(6, 10),  +1),
 ("E  shelf row 15    -> shelf row 12",      range(6, 10), 14, 12, range(11, 15), +1),
 ("F  shelf row 12    -> shelf row 9",       range(11, 15),11,  9, range(6, 10),  -1),
 ("G  shelf row 9     -> shelf row 6",       range(6, 10),  8,  6, range(6, 12),   0),
 ("H  shelf row 6     -> upper-left shelf",  range(6, 12),  5,  6, range(1, 5),   -1),
 ("I  upper-left      -> shelf row 6",       range(1, 5),   5,  6, range(6, 12),  +1),
 ("J  shelf row 6     -> shelf row 5",       range(6, 12),  5,  5, range(13, 18), +1),
 ("K  shelf row 5     -> shelf row 4",       range(13, 18), 4,  4, range(19, 24), +1),
 ("L  shelf row 4     -> shelf row 5 (rt)",  range(19, 24), 3,  5, range(27, 32), +1),
 ("M  shelf row 5 (rt)-> upper-right shelf", range(27, 32), 4,  6, range(33, 39), +1),
 ("N  upper-right     -> shelf row 8",       range(33, 39), 5,  8, range(28, 33), -1),
 ("O  shelf row 8     -> shelf row 11",      range(28, 33), 7, 11, range(26, 30), -1),
 ("P  shelf row 11    -> right mass",        range(26, 30),10, 14, range(30, 39), +1),
 ("Q  right mass      -> shelf row 16",      range(30, 39),13, 16, range(22, 26), -1),
 ("R  shelf row 16    -> pillar top",        range(22, 26),15, 13, range(18, 21), -1),
 ("S  pillar top      -> the floor",         range(18, 21),12, 20, range(21, 30), +1),
]

FLOODED = [
 ("W0 the shaft, down (hold Down on the shelf)", shaft_down),
 ("W1 water -> left island",                     lambda: water_to("L", range(3, 8))),
 ("W2 water -> right island",                    lambda: water_to("R", range(31, 36))),
 ("W3 left island -> shelf row 5",               lambda: reach(range(3, 8),   5, 5, range(9, 14),  +1, room=1)),
 ("W4 shelf row 5 -> shelf row 3",               lambda: reach(range(9, 14),  4, 3, range(16, 22), +1, room=1)),
 ("W5 shelf row 3 -> the throat shelf",          lambda: reach(range(16, 22), 2, 1, range(21, 27), +1, room=1)),
 ("W6 right island -> shelf row 5 (rt)",         lambda: reach(range(31, 36), 5, 5, range(27, 32), -1, room=1)),
 ("W7 shelf row 5 (rt) -> shelf row 3 (rt)",     lambda: reach(range(27, 32), 4, 3, range(26, 32), -1, room=1)),
 ("W8 shelf row 3 (rt) -> the throat shelf",     lambda: reach(range(26, 32), 2, 1, range(21, 27), -1, room=1)),
 ("W9 the shaft, up (jump from the throat)",     shaft_up),
 ("W10 standing jump, end of A2 -> A1",          lambda: standing_jump_to_throat(21)),
 ("W11 standing jump, end of A3 -> A1",          lambda: standing_jump_to_throat(26)),
 ("W12 failed exit lands back on A1",            failed_exit_lands_on_throat),
]

def check(h):
    if len(h) == 2:
        name, fn = h
        return name, fn()
    name, sc, sr, tr, tc, d = h
    return name, reach(sc, sr, tr, tc, d)

if __name__ == "__main__":
    with ThreadPoolExecutor(max_workers=6) as ex:
        results = list(ex.map(check, ROUTE + FLOODED))
    bad = []
    for name, plan in results:
        print("%-42s %s" % (name, plan if plan else "*** NO WAY OF PLAYING IT LANDS THIS"))
        if not plan: bad.append(name.split()[0])
    print()
    print("unmakeable hops:", ", ".join(bad) if bad else "none -- both rooms close")
    sys.exit(1 if bad else 0)
