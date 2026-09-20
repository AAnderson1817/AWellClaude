#!/usr/bin/env python3
"""Inline the existing Emscripten SINGLE_FILE module into a portable game page.

Usage: python tools/web/wrap.py [in.js] [out.html] [title]
Relative paths are relative to the invoking working directory. Preserve embedded
WASM string line endings: universal-newline translation can corrupt its bytes.
"""
import argparse
import html
import json
from pathlib import Path
import re

TEMPLATE = """<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>%TITLE%</title>
<style>
  :root { color-scheme: dark; }
  html,body { margin:0; height:100%; background:#000; overflow:hidden; }
  #wrap { position:fixed; inset:0; display:flex; align-items:center; justify-content:center; }
  canvas { display:block; background:#000; outline:none; max-width:100%; max-height:100%; }
</style>
</head><body>
<div id="wrap"><canvas id="canvas" tabindex="0" oncontextmenu="event.preventDefault()" aria-label="The Vault and the City Under It game"></canvas></div>
<script>%JS%</script>
<script>
  const cv = document.getElementById('canvas');
  window.__ready = false;
  window.__moduleError = null;
  RL({ canvas: cv, arguments: %ARGS%, print: t => console.log(t), printErr: t => console.warn(t) })
    .then(m => { window.__mod = m; window.__ready = true; cv.focus(); })
    .catch(error => { window.__moduleError = String(error); console.error('Game initialization failed:', error); });
  window.addEventListener('click', () => cv.focus());
</script>
</body></html>
"""

def parse_arguments(value):
    try:
        result = json.loads(value)
    except json.JSONDecodeError as error:
        raise argparse.ArgumentTypeError(f"Arguments must be a JSON array of strings: {error}") from error
    if not isinstance(result, list) or not all(isinstance(item, str) for item in result):
        raise argparse.ArgumentTypeError("Arguments must be a JSON array of strings")
    return result

def wrap(source, destination, title, arguments=None):
    arguments = [] if arguments is None else arguments
    if not isinstance(arguments, list) or not all(isinstance(item, str) for item in arguments):
        raise ValueError("Module arguments must be a list of strings")
    with open(source, encoding="utf-8", errors="surrogateescape", newline="") as stream:
        javascript = stream.read()
    # HTML closes a script even inside a JS string. Preserve the JS string value by
    # escaping the slash, including a possible sequence inside the embedded WASM.
    javascript = re.sub(r"</script", lambda match: "<\\/" + match.group()[2:], javascript, flags=re.I)
    # Arguments are data, not JavaScript source. Escape '<' to prevent even a quoted
    # malicious </script> argument from becoming an HTML end tag.
    argument_json = json.dumps(arguments, ensure_ascii=True).replace("<", "\\u003c")
    page = TEMPLATE.replace("%TITLE%", html.escape(title)).replace("%ARGS%", argument_json).replace("%JS%", javascript)
    destination = Path(destination)
    destination.parent.mkdir(parents=True, exist_ok=True)
    with open(destination, "w", encoding="utf-8", errors="surrogateescape", newline="") as stream:
        stream.write(page)
    return destination.stat().st_size

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", nargs="?", default="build/game.js")
    parser.add_argument("destination", nargs="?", default="build/play.html")
    parser.add_argument("title", nargs="?", default="The Vault and the City Under It")
    parser.add_argument("--arguments", type=parse_arguments, default=[],
                        help="Developer launch arguments as a JSON array of strings; normal play defaults to [].")
    args = parser.parse_args()
    size = wrap(args.source, args.destination, args.title, args.arguments)
    print(f"{args.destination}: {size:,} bytes; embedded module, no runtime asset downloads")

if __name__ == "__main__":
    main()
