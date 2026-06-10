#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$ROOT_DIR/build"

# Render settings. Edit these values for your usual test configuration.
WIDTH=1024
HEIGHT=1024
AREA_LIGHT_SAMPLES=8
SHADOW_MAP_SIZE=512
AMBIENT_STRENGTH=0.14
LIGHT_INTENSITY=1.00
MODEL_DIR="$ROOT_DIR/models/cornellbox"

# Derive output folder name from model directory.
MODEL_NAME="$(basename "$MODEL_DIR")"
OUT_DIR="$BUILD_DIR/$MODEL_NAME"

cmake -S "$ROOT_DIR" -B "$BUILD_DIR"
cmake --build "$BUILD_DIR" --parallel

"$BUILD_DIR/SSAO_Renderer" \
    --width "$WIDTH" \
    --height "$HEIGHT" \
    --area-light-samples "$AREA_LIGHT_SAMPLES" \
    --shadow-map-size "$SHADOW_MAP_SIZE" \
    --ambient-strength "$AMBIENT_STRENGTH" \
    --light-intensity "$LIGHT_INTENSITY" \
    --model-dir "$MODEL_DIR" \
    "$@"

if ! command -v sips >/dev/null 2>&1; then
    echo "error: sips is required to convert render.ppm to render.png" >&2
    exit 1
fi

sips -s format png "$OUT_DIR/render.ppm" \
    --out "$OUT_DIR/render.png" >/dev/null

printf 'Wrote %s\n' "$BUILD_DIR/render.png"
