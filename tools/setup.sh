#!/usr/bin/env bash
# One-time toolchain setup for the Animal Well Redux slice.
# Idempotent-ish: skips steps whose outputs already exist.
set -x
LOG=/home/user/AWellClaude/tools/setup.log
exec >>"$LOG" 2>&1
echo "=== setup start $(date -u) ==="

export DEBIAN_FRONTEND=noninteractive

if ! dpkg -s mingw-w64 >/dev/null 2>&1; then
  apt-get update -qq
  apt-get install -y -qq mingw-w64 libglfw3-dev libgl1-mesa-dev libglu1-mesa-dev \
    libasound2-dev libwayland-dev libxkbcommon-dev xorg-dev cmake
  echo "APT_DONE=$?"
fi

if [ ! -d /home/claude/emsdk ]; then
  git clone --depth 1 https://github.com/emscripten-core/emsdk.git /home/claude/emsdk
fi
cd /home/claude/emsdk && ./emsdk install latest && ./emsdk activate latest
echo "EMSDK_DONE=$?"

if [ ! -d /home/claude/raylib ]; then
  git clone --depth 1 --branch 5.5 https://github.com/raysan5/raylib.git /home/claude/raylib
fi
echo "RAYLIB_CLONE_DONE=$?"

export EMSDK=/home/claude/emsdk
export PATH="$EMSDK:$EMSDK/upstream/emscripten:$PATH"
for d in "$EMSDK"/node/*/bin; do export PATH="$d:$PATH"; done

LIB=/home/user/AWellClaude/lib
mkdir -p "$LIB"
cd /home/claude/raylib/src

make PLATFORM=PLATFORM_WEB -B -j4 && cp libraylib.a "$LIB/libraylib_web.a"
echo "WEB_BUILD=$?"

make PLATFORM=PLATFORM_DESKTOP -B -j4 && cp libraylib.a "$LIB/libraylib_linux.a"
echo "LINUX_BUILD=$?"

make PLATFORM=PLATFORM_DESKTOP OS=Windows_NT CC=x86_64-w64-mingw32-gcc \
     AR=x86_64-w64-mingw32-ar RAYLIB_LIBTYPE=STATIC -B -j4 \
  && cp libraylib.a "$LIB/libraylib_win.a"
echo "WIN_BUILD=$?"

ls -la "$LIB"
echo "=== setup done $(date -u) ==="
touch /home/user/AWellClaude/tools/.setup_complete
