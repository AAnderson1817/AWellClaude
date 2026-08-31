#!/usr/bin/env python3
"""Show a single room with the reachable set marked, so a cut can be seen not guessed."""
import sys
sys.argv = [sys.argv[0], 'rooms/world.txt']
import importlib.util
spec = importlib.util.spec_from_file_location("t", "tools/traverse.py")
