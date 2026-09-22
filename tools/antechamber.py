#!/usr/bin/env python3
"""The antechamber, drawn. Run it to rewrite src/antechamber.c.

Six screens, three across and two down. The lore is claude/LORE.md, the design and the
reasons for each shape are claude/ANTECHAMBER.md; this is only where things are.

    A entry      | B upper gallery | C great window
    D undercroft | E hall floor    | F inner gate

Rows that matter, top to bottom: the walk along the top (the step, the gallery, the sill)
stands on row 14; the undercroft and the hall floor stand on row 35; the basin's surface is
row 36. Everything between is the climb, and the climb is the colossus.
"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from roomgen import Room, write

R = Room(120, 44)
R.fill(0, 0, 119, 43, '#')             # start solid, and carve

# ------------------------------------------------------------------ A: the forecourt
# A tall cave inside the vault, and the whole of its far wall is the door: fifteen times
# your height, floor to roof (the backdrop piece F_DOOR). You wake at its foot. To the right
# a chasm drops to the undercroft, and a slope of fallen rock climbs to the passage.
R.clear(1, 1, 33, 20)
R.clear(34, 1, 39, 13)
R.fill(1, 0, 22, 0, 'X')                # the door's head runs up into the roof: its art draws it
# the roof over the right-hand side, ragged, and hanging in over the chasm
R.fill(23, 1, 39, 1, '#')
R.fill(24, 2, 39, 2, '#')
R.fill(26, 3, 28, 3, '#'); R.fill(33, 3, 35, 3, '#'); R.at(27, 4, '#')
# the flue slot: out of reach, in the roof over the slope, one amber seam inside it
R.clear(30, 1, 31, 3)
R.at(32, 1, '*')
# the chasm, down to the undercroft; the slope of fallen rock up to the passage
R.clear(24, 21, 26, 26)
R.fill(29, 18, 31, 20, '#')
R.fill(32, 16, 33, 20, '#')
R.fill(34, 14, 39, 20, '#')
# seams in the raw rock: warm down the left wall and in the roof and the slope
for x, y in ((0, 7), (0, 15), (25, 2), (31, 18), (33, 16)):
    R.at(x, y, '*')
R.at(23, 22, '*'); R.at(27, 24, '*')    # in the chasm's walls, lighting the ropes going down
R.at(2, 20, ','); R.at(21, 20, ','); R.at(28, 20, ',')

R.at(12, 20, 'P')                       # you, at the door's foot

# ------------------------------------------------------------------ D: the undercroft
# Low, raw, amber. The fire is the whole composition; its mouth opens right into the hall.
R.clear(1, 27, 39, 34)
R.clear(2, 26, 12, 26)
R.clear(18, 26, 33, 26)
R.clear(30, 22, 39, 26)                 # the mouth rises, and the colossus's hand hangs in
R.fill(1, 26, 3, 29, '#'); R.fill(1, 30, 1, 31, '#')    # no pocket too low to stand in
R.at(0, 31, '*'); R.at(5, 26, '*'); R.at(24, 25, '*'); R.at(10, 25, '*'); R.at(28, 25, '*'); R.at(3, 29, '*')
R.at(1, 34, ','); R.at(10, 34, ',')

# ------------------------------------------------------------------ B/E/C/F: the hall
# One volume, four screens: everything from x 40 is the tall ones' masonry.
R.clear(40, 1, 118, 35)
# the ceiling: a coffered vault would be a lie at this size; it is lost in the dark, with a
# rib or two of stone where the light reaches
R.fill(40, 0, 119, 0, '#')

# the gallery: row 14, masonry two thick, crossing the colossus's shoulder
R.fill(40, 14, 79, 15, '#')
R.fill(56, 14, 60, 15, '.'); R.shelf(56, 60, 14)   # the one place you come up through it

# the colossus: collision only where it holds you. 'X' is carved solid, 'x' carved shelf:
# the art draws them, the tiles only stand under it.
R.fill(40, 26, 45, 26, 'X')             # the back of the hand
R.fill(46, 24, 49, 24, 'X')             # the forearm, in bands
R.fill(50, 22, 53, 22, 'X')
R.fill(54, 20, 57, 20, 'X')             # the elbow
R.fill(57, 17, 60, 17, 'X')             # the band on the upper arm
R.fill(60, 25, 72, 25, 'X')             # the lap

# the way up to the hand from the floor: two corbels on the niche's wall, one-way, so the
# floor under them stays open to the water -- a body carrying a stone jumps too low to
# climb, and must still be able to walk into the basin and sink
R.shelf(41, 44, 32)
R.shelf(36, 39, 29)                     # and the hunters' planks, lashed up under its fingers
R.at(42, 34, 's')                       # the stone, on the hall floor by the basin

# the floor: paving at the undercroft's mouth, stepping down into the basin
R.fill(40, 35, 49, 43, '#')
R.fill(50, 37, 51, 43, '#')
R.fill(52, 39, 53, 43, '#')
# the basin, under the lap and on to the fireguard
R.fill(50, 36, 96, 42, '~')
R.fill(50, 37, 51, 42, '#'); R.fill(52, 39, 53, 42, '#')

# F: the giant stair, built for the tall ones: treads a jump high, up from the water to
# the sill's far end
treads = [(97, 34), (100, 31), (103, 28), (106, 25), (109, 22), (112, 19), (115, 16)]
for x0, top in treads:
    R.fill(x0, top, 118, 42, '#')
R.fill(97, 35, 118, 42, '#')

# C: the sill, the one long flat run, a balcony across the window's foot
R.fill(80, 14, 111, 15, '#')
R.fill(112, 14, 118, 15, '.')

# the city's glass lamps, set in their masonry: one in the threshold's arch, so the right of the
# first screen is washed green; one in the altar's face, lighting the hand from below; one in
# the foot of the stair; two in the sill's face
for x, y in ((37, 2), (97, 35), (86, 15), (106, 15)):
    R.at(x, y, '*')
# E/F: the dead seams where the rock runs in
for x, y in ((41, 36), (44, 38), (33, 17)):
    R.at(x, y, 'k')

# ------------------------------------------------------------------ zones
R.city(40, 0, 119, 43)                  # the hall
R.city(34, 2, 39, 13)                   # the threshold: the passage's last few steps are theirs
R.city(1, 0, 22, 20)                    # and the door's wall: theirs, built into the rock

# ------------------------------------------------------------------ backdrop pieces
# (kind, x, y, w, h, a) in tiles
R.feat('F_DOOR', 1, 0, 22, 21)          # the door: canvas at (8, 2) px, see tools/art/door_*.py
R.feat('F_CORNICE', 40, 1, 79, 1)
R.feat('F_NICHE', 45, 3, 32, 40)
R.feat('F_PILLAR', 78, 2, 3, 34)
R.feat('F_COLOSSUS', 40, 1, 36, 42)
R.feat('F_WINDOW', 82, 2, 30, 19)
R.feat('F_GRILLE', 81, 24, 15, 18)

# ------------------------------------------------------------------ props
R.prop(24, 21, 'r'); R.prop(26, 21, 'r')    # old lines over the chasm's lip: every anchor at the top
R.prop(44, 16, 'r'); R.prop(52, 16, 'r')    # and from the gallery rail into the hall
R.prop(28, 4, 'R'); R.prop(33, 3, 'R')
R.prop(47, 2, 'c')                      # a lamp on a long chain before the colossus's face
R.prop(68, 16, 'c')                     # and one under the gallery, over its lap
R.prop(91, 16, 'c')                     # and one under the sill
R.prop(20, 34, 'B')                     # the camp: bedroll, cairn, fire, pack
R.prop(26, 34, 'C')
R.prop(33, 34, 'F')
R.prop(36, 34, 'K')
R.prop(8, 27, 'R'); R.prop(19, 27, 'R'); R.prop(31, 27, 'R')    # roots through the undercroft's roof
R.prop(17, 34, 'P')                     # a pot by the bedroll
R.prop(11, 34, 'X')                     # and an older sleeper, long cold, by the dead lamps

# ------------------------------------------------------------------ the draft
# In low under the fireguard, across the basin, up past the colossus, out of the window.
for x in range(92, 52, -6): R.draft(x, 34.5, -0.020, 0.0, 10)
for y in range(33, 4, -5): R.draft(76, y, 0.0, -0.020, 12)
for x in range(78, 100, 6): R.draft(x, 4, 0.020, 0.0, 12)

ok = write(R, "// antechamber.c -- the room: tiles, dressing, zones, backdrop pieces, drafts. Written by\n"
              "// tools/antechamber.py (run it after editing that); read once at startup, never at runtime.")
sys.exit(0 if ok else 1)
