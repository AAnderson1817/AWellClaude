# Shared paths for build/setup/check scripts. Dependency overrides must be absolute.
AWELL_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
AWELL_TOOLCHAIN_DIR="${AWELL_TOOLCHAIN_DIR:-$AWELL_ROOT/.toolchain}"
RAYLIB="${RAYLIB:-$AWELL_TOOLCHAIN_DIR/raylib/src}"
export EMSDK="${EMSDK:-$AWELL_TOOLCHAIN_DIR/emsdk}"
AWELL_LIB_DIR="${AWELL_LIB_DIR:-$AWELL_ROOT/lib}"
export PATH="$EMSDK:$EMSDK/upstream/emscripten:$PATH"
for awell_node_bin in "$EMSDK"/node/*/bin; do
  if [ -d "$awell_node_bin" ]; then
    export PATH="$awell_node_bin:$PATH"
  fi
done
