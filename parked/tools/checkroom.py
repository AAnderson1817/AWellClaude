#!/usr/bin/env python3
"""Validate one authored room file against its template borders. Prints no grid."""
import sys, re
p = sys.argv[1]
L = [l.rstrip('\n') for l in open(p) if l.strip()]
hdr = L[0]
m = re.match(r'^ROOM (\d),(\d) teaches=(\S+)', hdr)
g = L[1:]
print('hdr ok' if m else 'HDR BAD', hdr[:60])
bad = [(i, len(l)) for i, l in enumerate(g) if len(l) != 40]
print('rows', len(g), 'badlen', bad)
if not m or bad or len(g) != 22:
    sys.exit(1)
x, y = m.group(1), m.group(2)
t = [l.rstrip('\n') for l in open(f'rooms/tmpl/{x}_{y}.txt') if l.strip()]
errs = []
for c in range(40):
    if g[0][c] != t[0][c]: errs.append(('r0', c))
    if g[21][c] != t[21][c]: errs.append(('r21', c))
for r in range(22):
    if g[r][0] != t[r][0]: errs.append(('c0', r))
    if g[r][39] != t[r][39]: errs.append(('c39', r))
print('border errors', errs[:12], len(errs))
legal = set('.#X-T~=R,')
ill = sorted({ch for row in g for ch in row} - legal)
print('illegal chars', ill)
def band(a, b): return sum(1 for r in range(a, b+1) for c in range(1, 39) if g[r][c] != '.')
tot = band(1, 20)
gap = run = 0
for r in range(1, 21):
    if all(g[r][c] == '.' for c in range(1, 39)): run += 1; gap = max(gap, run)
    else: run = 0
print(f'top {band(1,7)} mid {band(8,14)} bot {band(15,20)} fill {100.0*tot/760:.1f}% gap {gap}')
