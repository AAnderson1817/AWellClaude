#!/usr/bin/env bash
# Debian/Ubuntu toolchain setup. Overrides are documented in TOOLCHAIN.md.
set -euo pipefail
source "$(dirname -- "${BASH_SOURCE[0]}")/env.sh"
LOG="$AWELL_ROOT/tools/setup.log"
# Everything goes to the log; the terminal gets one line if it fails. An EXIT trap,
# not ERR: an unbound variable under set -u never fires ERR.
exec 3>&2
trap 'st=$?; if [ "$st" -ne 0 ]; then echo "setup failed (exit $st); see $LOG" >&3; fi' EXIT
exec >>"$LOG" 2>&1
# A failed rerun must not leave an old success marker behind.
rm -f "$AWELL_ROOT/tools/.setup_complete"
echo "=== setup start $(date -u) ==="

if [ "${AWELL_SKIP_SYSTEM_DEPS:-0}" != 1 ]; then
  export DEBIAN_FRONTEND=noninteractive
  packages=(build-essential git python3 mingw-w64 libglfw3-dev libgl1-mesa-dev
    libglu1-mesa-dev libasound2-dev libwayland-dev libxkbcommon-dev xorg-dev cmake)
  missing=()
  for package in "${packages[@]}"; do
    if ! dpkg-query -W -f='${Status}' "$package" 2>/dev/null | grep -qx 'install ok installed'; then
      missing+=("$package")
    fi
  done
  if (( ${#missing[@]} )); then
    apt-get update -qq
    apt-get install -y -qq "${missing[@]}"
  fi
fi

if [ ! -d "$EMSDK" ]; then
  mkdir -p "$(dirname -- "$EMSDK")"
  git clone --depth 1 https://github.com/emscripten-core/emsdk.git "$EMSDK"
fi
cd "$EMSDK"
./emsdk install "${EMSDK_VERSION:-6.0.8}"
./emsdk activate "${EMSDK_VERSION:-6.0.8}"

if [ ! -d "$RAYLIB" ]; then
  raylib_root="$(dirname -- "$RAYLIB")"
  mkdir -p "$(dirname -- "$raylib_root")"
  git clone --depth 1 --branch 5.5 https://github.com/raysan5/raylib.git "$raylib_root"
fi
# The SDK's node directory may only have appeared during installation.
source "$AWELL_ROOT/tools/env.sh"

mkdir -p "$AWELL_LIB_DIR"
cd "$RAYLIB"
make PLATFORM=PLATFORM_WEB -B -j4
cp libraylib.a "$AWELL_LIB_DIR/libraylib_web.a"
make PLATFORM=PLATFORM_DESKTOP -B -j4
cp libraylib.a "$AWELL_LIB_DIR/libraylib_linux.a"
make PLATFORM=PLATFORM_DESKTOP OS=Windows_NT CC=x86_64-w64-mingw32-gcc \
  AR=x86_64-w64-mingw32-ar RAYLIB_LIBTYPE=STATIC -B -j4
cp libraylib.a "$AWELL_LIB_DIR/libraylib_win.a"

ls -la "$AWELL_LIB_DIR"
echo "=== setup done $(date -u) ==="
touch "$AWELL_ROOT/tools/.setup_complete"
