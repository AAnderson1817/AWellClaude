#!/usr/bin/env bash
# The first screen with each design of the door, as the game draws it: once as you wake
# (your lamp at your feet), once in the dark (the lamp left up on the ledge). Stitched.
# Usage: tools/doorshots.sh OUTDIR
set -e
cd "$(dirname "$0")/.."
OUT="$1"; mkdir -p "$OUT"
export DISPLAY="${DISPLAY:-:99}"
for k in 0 1 2 3; do
  mkdir -p "$OUT/t"
  ./build/game --door $k --shots 30 --out "$OUT/t" --scale 1 --mute >/dev/null
  mv "$OUT/t/f0030.png" "$OUT/door$k.png"
  ./build/game --door $k --lamp 0,36,13 --shots 400 --out "$OUT/t" --scale 1 --mute >/dev/null
  mv "$OUT/t/f0400.png" "$OUT/door${k}_dark.png"
done
rmdir "$OUT/t"
python3 - "$OUT" <<'PY'
import sys
from PIL import Image
o = sys.argv[1]
ims = [[Image.open('%s/door%d%s.png' % (o, k, s)) for s in ('', '_dark')] for k in range(4)]
w, h = ims[0][0].size
sheet = Image.new('RGB', (w * 2 + 2, h * 4 + 6), (255, 0, 255))
for k in range(4):
    for j in range(2): sheet.paste(ims[k][j], (j * (w + 2), k * (h + 2)))
sheet.resize((sheet.width * 2, sheet.height * 2), Image.NEAREST).save(o + '/doors.png')
for k in range(4):
    ims[k][0].resize((w * 3, h * 3), Image.NEAREST).save('%s/door%d_x3.png' % (o, k))
PY
