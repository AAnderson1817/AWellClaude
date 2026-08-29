#!/usr/bin/env python3
"""Inline build/game.js into an artifact-shaped HTML fragment (no doctype/head/body:
the Artifact host supplies the skeleton, including charset=utf-8)."""
import sys
shell, js_path, out = sys.argv[1], sys.argv[2], sys.argv[3]
js = open(js_path, encoding='utf-8', errors='surrogateescape').read()
tpl = open(shell, encoding='utf-8').read()
assert '/*__GAME_JS__*/' in tpl, 'shell is missing the /*__GAME_JS__*/ marker'
open(out, 'w', encoding='utf-8', errors='surrogateescape').write(tpl.replace('/*__GAME_JS__*/', js))
print(out)
