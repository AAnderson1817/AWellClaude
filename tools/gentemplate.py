#!/usr/bin/env python3
"""Emit rooms/template.txt: every room's border already drawn, interiors blank.

Connectivity is fixed here, centrally, so 24 independently authored rooms cannot
disagree about where their shared doors are. Authors fill the interior only.
"""
W=H=5; RW,RH=40,22

# (x,y)-(x+1,y) : rows open in the shared column
HOR = {
 (0,0):(15,18),(1,0):(15,18),(2,0):(8,11),(3,0):(15,18),
 (0,1):(15,18),(1,1):(15,18),(2,1):(15,18),(3,1):(8,11),
 (0,2):(15,18),(1,2):(15,18),(2,2):(15,18),(3,2):(15,18),
 (0,3):(15,18),(1,3):(15,18),(2,3):(8,11),(3,3):(15,18),
 (0,4):(15,18),(1,4):(15,18),(2,4):(15,18),(3,4):(15,18),
}
# (x,y)-(x,y+1) : cols open in the shared row
VER = {
 (0,0):(18,21),(2,0):(6,9),(4,0):(30,33),
 (1,1):(18,21),(3,1):(12,15),
 (0,2):(25,28),(2,2):(18,21),(4,2):(8,11),
 (1,3):(18,21),(3,3):(26,29),
}

def build(x,y):
    g=[['.']*RW for _ in range(RH)]
    for c in range(RW): g[0][c]='#'; g[RH-1][c]='#'
    for r in range(RH): g[r][0]='#'; g[r][RW-1]='#'
    if (x-1,y) in HOR:
        a,b=HOR[(x-1,y)]
        for r in range(a,b+1): g[r][0]='.'
    if (x,y) in HOR:
        a,b=HOR[(x,y)]
        for r in range(a,b+1): g[r][RW-1]='.'
    if (x,y-1) in VER:
        a,b=VER[(x,y-1)]
        for c in range(a,b+1): g[0][c]='.'
    if (x,y) in VER:
        a,b=VER[(x,y)]
        for c in range(a,b+1): g[RH-1][c]='.'
    return g

def edges(x,y):
    e=[]
    if (x-1,y) in HOR: e.append(f"LEFT open rows {HOR[(x-1,y)][0]}-{HOR[(x-1,y)][1]} -> room ({x-1},{y})")
    if (x,y)   in HOR: e.append(f"RIGHT open rows {HOR[(x,y)][0]}-{HOR[(x,y)][1]} -> room ({x+1},{y})")
    if (x,y-1) in VER: e.append(f"TOP open cols {VER[(x,y-1)][0]}-{VER[(x,y-1)][1]} -> room ({x},{y-1})")
    if (x,y)   in VER: e.append(f"BOTTOM open cols {VER[(x,y)][0]}-{VER[(x,y)][1]} -> room ({x},{y+1})")
    return e

out=["; TEMPLATE - borders and doors are fixed. Fill interiors only (rows 1-20, cols 1-38)."]
spec=[]
for y in range(H):
    for x in range(W):
        g=build(x,y)
        out.append(f"ROOM {x},{y}")
        out += ["".join(r) for r in g]
        out.append("")
        spec.append(f"({x},{y}): " + " | ".join(edges(x,y)))
open('rooms/template.txt','w').write("\n".join(out)+"\n")
open('rooms/EDGES.txt','w').write("\n".join(spec)+"\n")
print(f"template written; {len(HOR)} horizontal + {len(VER)} vertical doors")
