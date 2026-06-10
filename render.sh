#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$ROOT_DIR/build"

# Render settings. Edit these values for your usual test configuration.
WIDTH=1024
HEIGHT=1024
AREA_LIGHT_SAMPLES=8
SHADOW_MAP_SIZE=512
AMBIENT_STRENGTH=0.20
LIGHT_INTENSITY=1.00
SHADOW_MIN_LIGHT=0.25
MODEL_DIR="$ROOT_DIR/models/cornellbox"

cmake -S "$ROOT_DIR" -B "$BUILD_DIR"
cmake --build "$BUILD_DIR" --parallel

"$BUILD_DIR/SSAO_Renderer" \
    --width "$WIDTH" \
    --height "$HEIGHT" \
    --area-light-samples "$AREA_LIGHT_SAMPLES" \
    --shadow-map-size "$SHADOW_MAP_SIZE" \
    --ambient-strength "$AMBIENT_STRENGTH" \
    --light-intensity "$LIGHT_INTENSITY" \
    --shadow-min-light "$SHADOW_MIN_LIGHT" \
    --model-dir "$MODEL_DIR" \
    --output "$BUILD_DIR/render.ppm" \
    --normal-output "$BUILD_DIR/normal_debug.ppm" \
    --depth-output "$BUILD_DIR/depth_debug.ppm" \
    "$@"

if ! command -v sips >/dev/null 2>&1; then
    echo "error: sips is required to convert render.ppm to render.png" >&2
    exit 1
fi

sips -s format png "$BUILD_DIR/render.ppm" \
    --out "$BUILD_DIR/render.png" >/dev/null

printf 'Wrote %s\n' "$BUILD_DIR/render.png"
