#!/usr/bin/env bash
# Headless native screenshots. Usage: tools/shots.sh "<frame list>" [extra args]
# The game must support:  --shots N,N,N  --out DIR   and exit after the last frame.
set -e
cd /home/user/AWellClaude
export LIBGL_ALWAYS_SOFTWARE=1 GALLIUM_DRIVER=llvmpipe
mkdir -p shots
rm -f shots/*.png
FRAMES="${1:-1,30,90}"
shift || true
xvfb-run -a -s "-screen 0 1280x720x24" ./build/game --shots "$FRAMES" --out shots "$@"
ls -la shots/
