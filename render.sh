#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$ROOT_DIR/build"

# Render settings.
WIDTH=2048
HEIGHT=1024
MODEL_DIR="$ROOT_DIR/models/ssao-demo"

# Derive output folder name from model directory.
MODEL_NAME="$(basename "$MODEL_DIR")"
OUT_DIR="$BUILD_DIR/$MODEL_NAME"
PNG_OUTPUT="$OUT_DIR/render.png"

cmake -S "$ROOT_DIR" -B "$BUILD_DIR"
cmake --build "$BUILD_DIR" --parallel

"$BUILD_DIR/SSAO_Renderer" \
    --width "$WIDTH" \
    --height "$HEIGHT" \
    --model-dir "$MODEL_DIR" \
    "$@"

if ! command -v sips >/dev/null 2>&1; then
    echo "error: sips is required to convert render.ppm to render.png" >&2
    exit 1
fi

sips -s format png "$OUT_DIR/render.ppm" \
    --out "$PNG_OUTPUT" >/dev/null

printf 'Wrote %s\n' "$PNG_OUTPUT"
