# SSAO Renderer

Headless OpenGL renderer for the GAMES101 final project. The current version
contains a deferred-rendering pipeline prepared for SSAO, but the SSAO sampling
algorithm is intentionally not implemented yet.

## Pipeline

The complete execution tree follows the current code path. OpenGL state calls
are shown under the project function that issues them.

```cpp
main(argc, argv) -> int                                           src/main.cpp
|   // Parse and validate command-line renderer configuration.
|-- parseAppConfig(argc, argv) -> AppConfig                       src/AppConfig.cpp
|
|   // Create the renderer and execute the complete frame pipeline.
`-- Renderer::render(config) -> void                              src/Renderer.cpp
    |
    |   // Set up the headless OpenGL context and initialize GLEW.
    |-- OpenGlContext::OpenGlContext() -> OpenGlContext           src/OpenGlContext.cpp
    |   |-- CGLChoosePixelFormat(...)
    |   |-- CGLCreateContext(...)
    |   |-- CGLSetCurrentContext(...)
    |   `-- glewInit()
    |
    |   // Load scene data, upload meshes, and sample the area light.
    |-- RenderScene::RenderScene(config) -> RenderScene           src/RenderScene.cpp
    |   |-- loadScene(config.modelDir) -> Scene                   src/SceneLoader.cpp
    |   |-- uploadMeshes(scene) -> std::vector<GpuMesh>
    |   |   |-- loadObjMesh(...) -> std::vector<Vertex>           src/ObjLoader.cpp
    |   |   `-- GpuMesh(...) -> GpuMesh                          src/GpuMesh.cpp
    |   `-- sampleAreaLight(...) -> std::vector<PointLight>       src/AreaLight.cpp
    |
    |   // Create the G-buffer position, normal, material, and depth attachments.
    |-- GeometryBuffer(width, height) -> GeometryBuffer           src/GeometryBuffer.cpp
    |
    |   // Create the single-channel AO visibility target.
    |-- AmbientOcclusionBuffer(width, height) -> AmbientOcclusionBuffer
    |                                                     src/AmbientOcclusionBuffer.cpp
    |
    |   // Create the HDR target that receives final lighting.
    |-- LightingBuffer(width, height) -> LightingBuffer           src/LightingBuffer.cpp
    |
    |   // Create the depth-only shader used for all shadow maps.
    |-- ShadowPass(shadowMapSize) -> ShadowPass                    src/ShadowPass.cpp
    |   `-- ShaderProgram(shadow shaders) -> ShaderProgram        src/ShaderProgram.cpp
    |
    |   // Create the shader that writes scene geometry into the G-buffer.
    |-- GeometryPass() -> GeometryPass                            src/GeometryPass.cpp
    |   `-- ShaderProgram(geometry shaders) -> ShaderProgram      src/ShaderProgram.cpp
    |
    |   // Create the current neutral AO stage.
    |-- AmbientOcclusionPass() -> AmbientOcclusionPass
    |
    |   // Create the deferred-lighting shader and fullscreen draw primitive.
    |-- LightingPass() -> LightingPass                            src/LightingPass.cpp
    |   |-- ShaderProgram(lighting shaders) -> ShaderProgram      src/ShaderProgram.cpp
    |   `-- FullscreenTriangle() -> FullscreenTriangle            src/FullscreenTriangle.cpp
    |
    |   // Render one light-space depth texture for every sampled point light.
    |-- ShadowPass::render(scene) -> std::vector<ShadowMap>        src/ShadowPass.cpp
    |   `-- for each PointLight
    |       |-- ShadowMap(shadowMapSize) -> ShadowMap              src/ShadowMap.cpp
    |       `-- ShadowMap::render(...) -> void
    |           |-- perspective(...) * lookAt(...) -> Mat4
    |           |-- glBindFramebuffer(GL_FRAMEBUFFER, shadowFbo)
    |           |-- glClear(GL_DEPTH_BUFFER_BIT)
    |           `-- GpuMesh::draw() -> void
    |
    |   // Rasterize scene meshes once and fill every G-buffer attachment.
    |-- GeometryPass::render(config, scene, geometryBuffer) -> void
    |                                                               src/GeometryPass.cpp
    |   |-- GeometryBuffer::bind() -> void
    |   |-- glEnable(GL_DEPTH_TEST)
    |   |-- ShaderProgram::use() -> void
    |   |-- set view and projection matrices
    |   `-- for each GpuMesh
    |       `-- GpuMesh::draw() -> void
    |
    |   // Initialize AO visibility to 1.0 until SSAO is implemented.
    |-- AmbientOcclusionPass::render(ambientOcclusionBuffer) -> void
    |                                                     src/AmbientOcclusionPass.cpp
    |   `-- AmbientOcclusionBuffer::clearNeutral() -> void
    |
    |   // Read G-buffer, AO, and shadow textures and accumulate all lights.
    |-- LightingPass::render(config, scene, shadowMaps, buffers...) -> void
    |                                                               src/LightingPass.cpp
    |   |-- LightingBuffer::bind() -> void
    |   |-- bindSurfaceInputs(...) -> void
    |   |   |-- bind position texture to GL_TEXTURE0
    |   |   |-- bind normal texture to GL_TEXTURE1
    |   |   |-- bind material texture to GL_TEXTURE2
    |   |   |-- bind depth texture to GL_TEXTURE3
    |   |   `-- bind AO texture to GL_TEXTURE4
    |   `-- for each PointLight
    |       |-- configureAccumulation(firstLight) -> void
    |       |-- bindLight(..., shadowMap) -> void
    |       |   `-- bind shadow texture to GL_TEXTURE5
    |       `-- FullscreenTriangle::draw() -> void
    |           `-- lighting fragment shader -> vec4 oColor
    |
    |   // Read the render targets and write final and debug PPM images.
    `-- RenderOutputWriter::write(config, buffers...) -> void      src/RenderOutputWriter.cpp
        |-- LightingBuffer::writeColor(...) -> void
        |-- GeometryBuffer::writeNormalDebug(...) -> void
        |-- GeometryBuffer::writeDepthDebug(...) -> void
        `-- AmbientOcclusionBuffer::writeDebug(...) -> void
```

The G-buffer stores:

- world-space position in `RGB32F`;
- world-space normal in `RGB16F`;
- albedo and emissive flag in `RGBA16F`;
- hardware depth in `DEPTH_COMPONENT24`.

World-space geometry data keeps the existing shadow calculation direct. The
SSAO implementation can transform position and normal into view space inside
`AmbientOcclusionPass` before constructing its sample basis.

## Code Structure

- `Renderer`: contains only top-level resource creation and pass ordering.
- `RenderScene`: owns the parsed scene, uploaded meshes, and sampled lights.
- `ShadowPass`: creates and renders the per-light shadow maps.
- `GeometryPass`: rasterizes scene meshes into the G-buffer.
- `AmbientOcclusionPass`: owns the AO stage; currently writes neutral visibility.
- `LightingPass`: owns deferred lighting, texture binding, and light accumulation.
- `RenderOutputWriter`: writes final color and debug attachments to disk.
- `OpenGlContext`: owns the headless macOS OpenGL context.
- `GeometryBuffer`: owns position, normal, material, and depth attachments.
- `AmbientOcclusionBuffer`: owns the single-channel AO attachment.
- `LightingBuffer`: owns the final HDR color attachment.
- `FramebufferSupport.hpp`: header-only internal texture, FBO validation, and
  PPM helpers shared by the buffer implementations.
- `FullscreenTriangle`: shared draw primitive for screen-space passes.
- `GeometryShaders`, `LightingShaders`, and `ShadowShaders`: shader sources
  grouped by their owning pass.
- `ShadowMap`: depth target and rendering for one sampled point light.
- `ShaderProgram`: shader compilation, linking, and uniform helpers.
- `SceneLoader`: reads scene objects, camera, light, and shadow settings.
- `GpuMesh`: owns one mesh VAO and VBO.

The AO target is already sampled by the lighting shader only for the ambient
term:

```glsl
ambient = albedo * ambientStrength * ambientOcclusion;
```

Direct lighting and shadow mapping are independent from AO.

## Build and Render

Dependencies:

- OpenGL
- GLEW
- nlohmann-json

On macOS with Homebrew:

```sh
brew install glew nlohmann-json
./render.sh
```

`render.sh` configures CMake, builds the executable, renders the Cornell Box,
and converts the final color output to `build/render.png`. Command-line
arguments appended to the script override its normal test settings:

```sh
./render.sh --area-light-samples 2 --width 256 --height 256
```

Important options:

```text
--width N
--height N
--area-light-samples N
--shadow-map-size N
--ambient-strength value
--light-intensity value
--model-dir path
--output path
--normal-output path
--depth-output path
--ao-output path
```

## Outputs

- `build/render.ppm`: final deferred-lighting result.
- `build/render.png`: PNG conversion of the final result.
- `build/normal_debug.ppm`: encoded world-space G-buffer normals.
- `build/depth_debug.ppm`: normalized depth visualization.
- `build/ambient_occlusion_debug.ppm`: currently solid white because the AO
  algorithm has not been implemented.

Scene files and parameters live in `models/cornellbox/scene.json`. Adding a new
scene requires model files plus a matching `scene.json`; no scene-specific C++
code is required.

## Next SSAO Step

The next implementation is isolated to `AmbientOcclusionPass`:

1. generate a hemisphere sample kernel and rotation-noise texture;
2. read G-buffer position, normal, and depth;
3. transform geometry into view space and evaluate visibility;
4. write visibility into `AmbientOcclusionBuffer`;
5. add a separate edge-aware blur target before lighting consumes AO.

No geometry, shadow-map, or direct-lighting restructuring should be necessary
for that step.
