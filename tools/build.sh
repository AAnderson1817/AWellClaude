#!/usr/bin/env bash
# Build the slice. Usage: tools/build.sh [web|linux|win|all]   (default: all)
set -euo pipefail
source "$(dirname -- "${BASH_SOURCE[0]}")/env.sh"

cd "$AWELL_ROOT"
mkdir -p build

TARGET="${1:-all}"
SRC=(src/*.c)

build_linux() {
  echo ">> linux"
  "${AWELL_CC[@]}" "${SRC[@]}" -o build/game -I"$RAYLIB" "$AWELL_LIB_DIR/libraylib_linux.a" \
    -lGL -lm -lpthread -ldl -lrt -lX11 -O2 -Wall -Wno-unused-function
}
build_web() {
  echo ">> web"
  emcc "${SRC[@]}" -o build/game.js -I"$RAYLIB" "$AWELL_LIB_DIR/libraylib_web.a" \
    -Os -DPLATFORM_WEB -s USE_GLFW=3 -s ASYNCIFY -s SINGLE_FILE=1 \
    -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE=1 -s EXPORT_NAME=RL -s ENVIRONMENT=web \
    -s EXPORTED_RUNTIME_METHODS=HEAPF32,HEAP8,HEAPU8,HEAP16,HEAPU16,HEAP32,HEAPU32
  # The heap views: miniaudio's web glue reads Module.HEAPF32 in every audio callback,
  # and emscripten stopped attaching those to the module by default. Without this the
  # web build is silent and throws once per audio buffer.
}
build_win() {
  echo ">> win"
  x86_64-w64-mingw32-gcc "${SRC[@]}" -o build/game.exe -I"$RAYLIB" \
    "$AWELL_LIB_DIR/libraylib_win.a" -lopengl32 -lgdi32 -lwinmm -static -static-libgcc -O2 -mwindows
}

case "$TARGET" in
  linux) build_linux ;;
  web)   build_web ;;
  win)   build_win ;;
  all)   build_linux; build_web; build_win ;;
  *) echo "unknown target $TARGET"; exit 1 ;;
esac
ls -la build/
