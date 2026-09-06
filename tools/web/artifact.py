#!/usr/bin/env python3
"""Inline build/game.js into an artifact-shaped HTML fragment (no doctype/head/body:
the Artifact host supplies the skeleton, including charset=utf-8)."""
import sys
shell, js_path, out = sys.argv[1], sys.argv[2], sys.argv[3]
# newline='': the wasm is embedded as a binary-coded string, and a 0x0D byte in it is a
# '\r' that universal-newline mode silently rewrites as '\n'. One byte off and the module
# fails to instantiate ("section was shorter than expected size"). Which build has such a
# byte is luck; this one did.
js = open(js_path, encoding='utf-8', errors='surrogateescape', newline='').read()
tpl = open(shell, encoding='utf-8').read()
assert '/*__GAME_JS__*/' in tpl, 'shell is missing the /*__GAME_JS__*/ marker'
open(out, 'w', encoding='utf-8', errors='surrogateescape', newline='').write(tpl.replace('/*__GAME_JS__*/', js))
print(out)
