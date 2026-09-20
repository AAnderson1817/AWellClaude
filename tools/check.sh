#!/usr/bin/env bash
# Headless regression checks: a C compiler, Python 3 and raylib's header suffice.
set -euo pipefail
source "$(dirname -- "${BASH_SOURCE[0]}")/env.sh"
cd "$AWELL_ROOT"
check_dir="$(mktemp -d)"
trap 'rm -rf "$check_dir"' EXIT
"${AWELL_CC[@]}" -std=c99 -O1 -g -DAWELL_HEADLESS -fsanitize=undefined -fno-sanitize-recover=all \
  -I"$RAYLIB" tools/tests/input_hash.c tools/tests/raylib_stubs.c \
  src/player.c src/room.c src/fx.c src/render.c src/audio.c src/life.c src/items.c src/props.c -lm -o "$check_dir/regressions"
"$check_dir/regressions"
"${AWELL_CC[@]}" -std=c99 -O1 -g -DAWELL_HEADLESS -fsanitize=undefined -fno-sanitize-recover=all \
  -I"$RAYLIB" tools/tests/presentation_snapshots.c tools/tests/raylib_stubs.c \
  src/player.c src/room.c src/fx.c src/render.c src/audio.c src/life.c src/items.c src/props.c -lm -o "$check_dir/snapshots"
"$check_dir/snapshots"
python3 tools/tests/test_tools.py
