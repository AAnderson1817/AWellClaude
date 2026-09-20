#!/usr/bin/env bash
# Inline the existing SINGLE_FILE web build; no checkout-specific absolute paths.
# Usage: tools/web/wrap.sh [in.js] [out.html] [title]
set -euo pipefail
project_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$project_root"
python3 tools/web/wrap.py "${1:-build/game.js}" "${2:-build/play.html}" "${3:-The Vault and the City Under It}"
