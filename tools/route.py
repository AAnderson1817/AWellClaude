#!/usr/bin/env python3
"""Every hop of the room's climb, checked by searching input timings.

The route below is the one I designed. The point of this file is that it does not
trust me: a hop counts as makeable only if some combination of take-off point,
jump timing, jump length and when-you-stop-pushing actually lands it."""
import sys, os
from concurrent.futures import ThreadPoolExecutor
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from probe import reach

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

def check(h):
    name, sc, sr, tr, tc, d = h
    return name, reach(sc, sr, tr, tc, d)

if __name__ == "__main__":
    with ThreadPoolExecutor(max_workers=10) as ex:
        results = list(ex.map(check, ROUTE))
    bad = []
    for name, plan in results:
        print("%-42s %s" % (name, plan if plan else "*** NO WAY OF PLAYING IT LANDS THIS"))
        if not plan: bad.append(name.split()[0])
    print()
    print("unmakeable hops:", ", ".join(bad) if bad else "none -- the route closes")
    sys.exit(1 if bad else 0)
