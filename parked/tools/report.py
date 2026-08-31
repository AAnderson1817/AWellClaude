#!/usr/bin/env python3
"""Produce the Phase 4 traversal report: static analysis, composition, and real play."""
import subprocess, sys, re, os
def run(cmd):
    return subprocess.run(cmd, capture_output=True, text=True).stdout

print("# Phase 4 — traversal report\n")
print("## Static analysis — tools/traverse.py")
print("""
Models walking, falling with air control, whole-tile jumps, dropping through one-way
platforms, load-signed buoyancy, and changing load at a rim. Does NOT model the gaff,
so it under-states what is reachable rather than over-stating it.
""")
t = run(["python3", "tools/traverse.py", "rooms/world.txt"])
print("```")
print(t.strip())
print("```\n")

print("## Composition — tools/compose.py")
print("""
The camera is locked, so all 22 rows are the composition. This measures how much of the
frame each room uses, detects the diagonal plank staircase, and compares every pair of
rooms as coarse silhouettes to catch two rooms that read as the same picture.
""")
c = run(["python3", "tools/compose.py", "rooms/world.txt"])
print("```")
print(c.strip())
print("```")
