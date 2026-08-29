#!/usr/bin/env bash
# Inline build/game.js into a self-contained playable HTML page.
# Usage: tools/web/wrap.sh [in.js] [out.html] [title]
set -e
cd /home/user/AWellClaude
IN="${1:-build/game.js}"; OUT="${2:-build/play.html}"; TITLE="${3:-Slice}"
python3 - "$IN" "$OUT" "$TITLE" <<'PY'
import sys, html
src, out, title = sys.argv[1], sys.argv[2], sys.argv[3]
js = open(src, encoding='utf-8', errors='surrogateescape').read()
tpl = """<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>%TITLE%</title>
<style>
  :root { color-scheme: dark; }
  html,body { margin:0; height:100%%; background:#000; overflow:hidden; }
  #wrap { position:fixed; inset:0; display:flex; align-items:center; justify-content:center; }
  canvas { image-rendering: pixelated; image-rendering: crisp-edges; display:block;
           background:#000; outline:none; max-width:100%%; max-height:100%%; }
</style>
</head><body>
<div id="wrap"><canvas id="canvas" tabindex="1" oncontextmenu="event.preventDefault()"></canvas></div>
<script>%JS%</script>
<script>
  const cv = document.getElementById('canvas');
  window.__ready = false;
  RL({ canvas: cv, print: (t)=>console.log(t), printErr: (t)=>console.warn(t) })
    .then(m => { window.__mod = m; window.__ready = true; cv.focus(); });
  window.addEventListener('click', () => cv.focus());
</script>
</body></html>
"""
tpl = tpl.replace('%TITLE%', html.escape(title)).replace('%JS%', js)
open(out, 'w', encoding='utf-8', errors='surrogateescape').write(tpl)
print(out, len(tpl))
PY
