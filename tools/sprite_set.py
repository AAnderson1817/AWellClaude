#!/usr/bin/env python3
"""Replace one sprite's rows in src/sprites.c: sprite_set.py NAME < rows (one per line)."""
import re, sys, pathlib
name = sys.argv[1]; rows = [r.rstrip("\n") for r in sys.stdin if r.strip()]
w = len(rows[0]); assert all(len(r) == w for r in rows), [len(r) for r in rows]
p = pathlib.Path(__file__).resolve().parents[1] / "src/sprites.c"; s = p.read_text()
body = "\n".join('    "%s",' % r for r in rows)
new = 'static const char *const %s_ROWS[] = {\n%s\n};\nconst Sprite %s = { %d, %d, %s_ROWS };' % (name, body, name, w, len(rows), name)
pat = re.compile(r"static const char \*const %s_ROWS\[\] = \{.*?\};\nconst Sprite %s = \{[^}]*\};" % (name, name), re.S)
if pat.search(s): s = pat.sub(lambda m: new, s)
else: s = s.rstrip("\n") + "\n\n" + new + "\n"
p.write_text(s); print(name, w, "x", len(rows))
