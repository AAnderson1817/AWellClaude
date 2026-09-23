#!/usr/bin/env python3
"""The antechamber, drawn. Run it to rewrite src/antechamber.c.

Six screens, three across and two down, and one volume: the archive's great hall, from the
roof to the floor and wall to wall, nothing across it. The lore is claude/LORE.md, the
building's language claude/ARCHIVE.md, the design and the reasons for each shape
claude/ANTECHAMBER.md; this is only where things are.

    A the door's crown, the flue | B the keeper's head  | C the window
    D the door, the camp         | E the keeper's lap   | F the stair

The far wall is one wheel (tools/art/archive.py): the heart at its hub behind the keeper,
spokes out from it, and on its rim, at the two ends of one diameter, the door (low left)
and the window (high right). You wake at the door's foot. Rows that matter: the floor is
row 38 and the basin's surface is level with it; the window's lip is row 20.
"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from roomgen import Room, write

R = Room(120, 44)
R.fill(0, 0, 119, 43, '#')             # start solid, and carve
R.clear(1, 1, 118, 37)                  # the hall: one volume, the floor under it at row 38

# ------------------------------------------------------------------ D: the door's foot
R.at(16, 37, 'P')                       # you, at the door's foot
# the hunters' way up the door: planks driven into the joints of its ring, up its right
# side to its crown -- and from the crown, on up the raw rock toward the flue, to a ledge
# under it where one of them stayed
R.shelf(31, 33, 35)
R.shelf(30, 32, 32)
R.shelf(29, 31, 29)
R.shelf(28, 30, 26)
R.shelf(26, 28, 23)
R.shelf(23, 25, 20)
R.shelf(14, 20, 17)                     # the crown of the door
R.shelf(9, 11, 14)
R.shelf(5, 7, 11)
R.fill(1, 8, 4, 8, '#')                 # the ledge under the flue
R.at(4, 0, '*')                         # the flue: a slot of amber in the roof, out of reach

# ------------------------------------------------------------------ E: the keeper
# The colossus: collision only where it holds you. 'X' is carved solid: the art draws it,
# the tile only stands under it. tools/art/colossus.py has the same list (STANDS).
R.fill(40, 26, 45, 26, 'X')             # the back of the hand
R.fill(46, 24, 49, 24, 'X')             # the forearm, in bands
R.fill(50, 22, 53, 22, 'X')
R.fill(54, 20, 57, 20, 'X')             # the elbow
R.fill(57, 17, 60, 17, 'X')             # the band on the upper arm
R.fill(60, 14, 64, 14, 'X')             # the shoulder
R.fill(60, 25, 72, 25, 'X')             # the lap
# the way up to the hand from the floor: a plank and a corbel, one-way, so the floor under
# them stays open to the water -- a body carrying a stone jumps too low to climb, and must
# still be able to walk into the basin and sink -- and the hunters' planks under its fingers
R.shelf(42, 45, 35)
R.shelf(41, 44, 32)
R.shelf(36, 39, 29)
R.at(47, 37, 's')                       # the stone, on the paving by the basin
for x in (62, 66, 70): R.at(x, 24, 'b') # growth on the keeper's lap

# the basin: level with the floor, under the keeper's shins and before the heart, stepping
# down at its near end
R.fill(50, 38, 96, 42, '~')
R.fill(50, 40, 51, 42, '#'); R.fill(52, 41, 53, 42, '#')

# ------------------------------------------------------------------ F: the stair, C: the window
# The tall ones' stair, outside the rim: treads a jump high, grown out of a rib on either
# side of it by turns, from the floor at the basin's far end up to the window; from the top
# tread you turn back onto the window's lip, inside its ring, where three of your size sit
# looking out.
treads = [(108, 35), (112, 32), (108, 29), (112, 26), (108, 23)]
for x0, top in treads:
    R.fill(x0, top, x0 + 2, top + 1, '#')
    R.at(x0 + 1, top - 1, 'b')          # each tread a bed
R.fill(100, 20, 107, 20, 'X')           # the window's lip: the art draws it
R.at(107, 20, '*')                      # a glass lamp set in its end
R.at(112, 33, '*'); R.at(112, 27, '*')  # and in the treads' ends

# ------------------------------------------------------------------ seams
# amber in the raw rock at the door's end; black where the archive has drunk them
for x, y in ((0, 5), (0, 31), (12, 0), (1, 38), (26, 38)):
    R.at(x, y, '*')
for x, y in ((114, 26), (108, 36), (45, 40), (40, 41)):
    R.at(x, y, 'k')

# ------------------------------------------------------------------ zones
# The raw rock is the mountain the wheel is cut into: the door's end, its floor and roof.
# Everything else is theirs.
R.city(36, 0, 119, 43)

# ------------------------------------------------------------------ the openings
# The far wall is one picture (tools/art/archive.py); these are the holes in it, each a
# circle in its square of tiles (x, y, w, h): the window's cell, and the heart's grate.
# archive.py cuts the picture to the same circles (the heart's only above the water).
R.feat('F_WINDOW', 94, -1, 21, 21)
R.feat('F_GRILLE', 43, 3, 36, 36)

# ------------------------------------------------------------------ props
R.prop(32, 37, 'B')                     # the camp: bedroll, pot, cairn, fire, pack
R.prop(31, 37, 'P')
R.prop(34, 37, 'C')
R.prop(37, 37, 'F')                     # under the keeper's fingertips
R.prop(38, 37, 'K')
R.prop(2, 7, 'X')                       # the one who climbed toward the light, on the ledge under the flue
R.prop(4, 9, 'r')                       # and his line, down from it
R.prop(47, 1, 'c')                      # a lamp on a long chain before the keeper's face
R.prop(91, 1, 'c')                      # and one before the window
R.prop(8, 1, 'R'); R.prop(24, 1, 'R'); R.prop(116, 1, 'R')   # roots through the roof

# ------------------------------------------------------------------ the draft
# Out of the heart, up behind the keeper, out of the window.
for y in range(34, 5, -5): R.draft(76, y, 0.0, -0.020, 12)
for x in range(78, 102, 6): R.draft(x, 5, 0.020, 0.008, 12)

ok = write(R, "// antechamber.c -- the room: tiles, dressing, zones, openings, drafts. Written by\n"
              "// tools/antechamber.py (run it after editing that); read once at startup, never at runtime.")
sys.exit(0 if ok else 1)
