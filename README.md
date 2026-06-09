# SSAO Renderer

OpenGL rasterization scaffold for the GAMES101 final project. The current
version renders the HW7 Cornell Box OBJ files through a hidden macOS CGL OpenGL
context and writes framebuffer outputs to disk. SSAO can be added as a later
screen-space pass using the existing color, normal, and depth attachments.

The code is split by responsibility:

- `AppConfig`: command-line options and output paths.
- `CornellBoxScene`: Cornell Box object list and material colors.
- `ObjLoader`: minimal OBJ triangle loading.
- `GpuMesh`: VAO/VBO upload and draw.
- `ShaderProgram`: shader compile/link and uniform helpers.
- `Framebuffer`: color, normal, and depth render targets plus PPM dumps.
- `AreaLight`: Cornell Box rectangular light sampled into point lights.
- `ShadowMap`: point-light depth cubemap shadow maps.
- `Renderer`: OpenGL context setup and render-pass orchestration.

## Build

Dependencies used by CMake:

- OpenGL
- GLEW

On macOS with Homebrew these can be installed with:

```sh
brew install glew
```

Build and render:

```sh
cmake -S . -B build
cmake --build build
./build/SSAO_Renderer --output build/render.ppm \
  --normal-output build/normal_debug.ppm \
  --depth-output build/depth_debug.ppm
```

Default test assets are copied into `models/cornellbox` from `../../HW7`.

Useful runtime options:

```sh
./build/SSAO_Renderer \
  --area-light-samples 4 \
  --shadow-map-size 512 \
  --output build/render.ppm
```

`--area-light-samples N` samples the Cornell Box rectangle light into `N x N`
point lights. The current shader supports up to 16 point-light shadow maps, so
`N` is limited to 4. Each point light gets its own depth cubemap shadow map.

## Outputs

- `render.ppm`: base Cornell Box rasterization.
- `normal_debug.ppm`: normal attachment visualized as RGB.
- `depth_debug.ppm`: depth attachment visualized as grayscale.

The default camera matches the HW7 Cornell Box convention: eye
`(278, 273, -800)`, looking into the box.

The base render already includes shadow mapping. The Cornell Box area light is
approximated by many point lights, which gives soft-shadow behavior while still
keeping the pipeline easy to extend with a later SSAO pass.
