#!/usr/bin/env bash
# Builds the browser version of Snake with Emscripten.
# Usage: source ~/emsdk/emsdk_env.sh && bash web/build.sh [output-folder]
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="$(dirname "$HERE")"
OUT="${1:-$HOME/snake-wasm-build}"

command -v emcc >/dev/null || { echo "emcc not found. Run: source ~/emsdk/emsdk_env.sh"; exit 1; }
mkdir -p "$OUT"

emcc "$REPO/main.cpp" -std=c++17 -O2 \
  -sUSE_SDL=2 -sUSE_SDL_TTF=2 \
  -sALLOW_MEMORY_GROWTH=1 -sFORCE_FILESYSTEM=1 \
  -lidbfs.js -sEXPORTED_RUNTIME_METHODS=FS \
  --preload-file "$HERE/DejaVuSans-Bold.ttf@/DejaVuSans-Bold.ttf" \
  --shell-file "$HERE/shell.html" \
  -o "$OUT/snake.html"

echo "Built into $OUT"
ls -lh "$OUT"