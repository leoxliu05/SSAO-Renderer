# SSAO Renderer

Headless OpenGL renderer for the GAMES101 final project. The current version
contains a deferred-rendering pipeline prepared for SSAO, but the SSAO sampling
algorithm is intentionally not implemented yet.

## Pipeline

The complete execution tree follows the current code path.

```cpp
// Parse and validate command-line renderer configuration.
main(int argc, char** argv) -> int status                     src/main.cpp
|-- parseAppConfig(argc, argv) -> AppConfig config            src/AppConfig.cpp
|
|   // Create the renderer and drive the full frame pipeline.
`-- Renderer::render(config) -> void                          src/Renderer.cpp
    |
    |   // Set up a headless macOS CGL context and initialize GLEW.
    |-- OpenGLHelpers::Context() -> Context context           src/OpenGLHelpers.cpp
    |   |-- CGLChoosePixelFormat(attrs) -> CGLPixelFormatObj  (CGL)
    |   |-- CGLCreateContext(pixelFormat) -> CGLContextObj    (CGL)
    |   |-- CGLSetCurrentContext(context)                     (CGL)
    |   `-- glewInit() -> GLenum                              (GLEW)
    |
    |   // Load scene JSON, upload all meshes to GPU, sample area light into point lights.
    |-- Scene::Scene(config) -> Scene scene                   src/Scene.cpp
    |   |-- Json::parse(input) -> json root                   <nlohmann/json.hpp>
    |   |-- readObjects(root, modelDir)                       src/Scene.cpp (static)
    |   |   `-- std::vector<SceneObject> objects
    |   |-- readCamera(root) -> SceneCamera camera            src/Scene.cpp (static)
    |   |-- readAreaLight(root) -> RectAreaLight areaLight    src/Scene.cpp (static)
    |   |-- readShadowSettings(root) -> ShadowSettings shadow src/Scene.cpp (static)
    |   |-- uploadMeshes(objects) -> std::vector<GpuMesh> meshes
    |   |   |                                           src/Scene.cpp (static)
    |   |   |-- loadObjMesh(path, color, offset)              src/ObjLoader.cpp
    |   |   |   `-> std::vector<Vertex> vertices
    |   |   `-- GpuMesh(name, vertices, emissive) -> GpuMesh  src/GpuMesh.cpp
    |   |       |-- glGenVertexArrays + glGenBuffers          (create VAO + VBO)
    |   |       |-- glBufferData(GL_ARRAY_BUFFER, vertices)   (upload vertex data)
    |   |       `-- glVertexAttribPointer × 3                 (layout: pos=0, normal=1, color=2)
    |   `-- sampleAreaLight(areaLight, samplesPerSide)        src/AreaLight.cpp
    |       `-> std::vector<PointLight> lights
    |
    |   // Create G-buffer: allocates 4 GPU textures and assembles the FBO.
    |-- GeometryBuffer(width, height) -> GeometryBuffer geometryBuffer
    |   |                                                   src/GeometryBuffer.cpp
    |   |-- FramebufferSupport::createTexture(w, h, GL_RGB32F, GL_RGB)
    |   |   `-> GLuint positionTexture_   (world-space position)
    |   |-- FramebufferSupport::createTexture(w, h, GL_RGB16F, GL_RGB)
    |   |   `-> GLuint normalTexture_     (world-space normal)
    |   |-- FramebufferSupport::createTexture(w, h, GL_RGBA16F, GL_RGBA)
    |   |   `-> GLuint materialTexture_   (albedo + emissive flag)
    |   |-- FramebufferSupport::createTexture(w, h, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT)
    |   |   `-> GLuint depthTexture_      (hardware depth)
    |   |-- glGenFramebuffers → attach 3 color + 1 depth → glDrawBuffers(3)
    |   `-- FramebufferSupport::requireComplete("geometry framebuffer")
    |
    |   // Create AO buffer: single-channel R16F texture + FBO.
    |-- AOBuffer(width, height) -> AOBuffer aoBuffer          src/AOBuffer.cpp
    |   |-- FramebufferSupport::createTexture(w, h, GL_R16F, GL_RED)
    |   |   `-> GLuint texture_   (AO visibility)
    |   |-- glGenFramebuffers → attach texture to GL_COLOR_ATTACHMENT0
    |   `-- FramebufferSupport::requireComplete("ambient occlusion framebuffer")
    |
    |   // Create HDR lighting buffer: RGBA16F texture + FBO.
    |-- LightingBuffer(width, height) -> LightingBuffer lightingBuffer
    |   |                                                   src/LightingBuffer.cpp
    |   |-- FramebufferSupport::createTexture(w, h, GL_RGBA16F, GL_RGBA)
    |   |   `-> GLuint texture_   (HDR accumulated color)
    |   |-- glGenFramebuffers → attach texture to GL_COLOR_ATTACHMENT0
    |   `-- FramebufferSupport::requireComplete("lighting framebuffer")
    |
    |   // Compile the depth-only shader used by every shadow map.
    |-- ShadowPass(shadowMapSize) -> ShadowPass shadowPass    src/ShadowPass.cpp
    |   `-- ShaderProgram(ShadowShaders::vertex, ShadowShaders::fragment)
    |       |                                              src/ShaderProgram.cpp
    |       |-- compileShader(GL_VERTEX_SHADER, src) -> GLuint
    |       |-- compileShader(GL_FRAGMENT_SHADER, src) -> GLuint
    |       |-- glCreateProgram → glAttachShader × 2 → glLinkProgram
    |       `-- glDeleteShader × 2 (detach after link)
    |
    |   // Compile the shader that writes world-space geometry into the G-buffer.
    |-- GeometryPass() -> GeometryPass geometryPass           src/GeometryPass.cpp
    |   `-- ShaderProgram(GeometryShaders::vertex, GeometryShaders::fragment)
    |       |                                              src/ShaderProgram.cpp
    |       |-- compileShader(vert) + compileShader(frag) → link
    |       `-- (same compile-link pattern as above)
    |
    |   // Currently a no-op constructor; will own SSAO sample kernel later.
    |-- AOPass() -> AOPass aoPass                             src/AOPass.cpp
    |
    |   // Compile the deferred-lighting shader and create the fullscreen draw VAO.
    |-- LightingPass() -> LightingPass lightingPass           src/LightingPass.cpp
    |   |-- ShaderProgram(LightingShaders::vertex, LightingShaders::fragment)
    |   |   |                                              src/ShaderProgram.cpp
    |   |   `-- (compile vert + frag → link, same pattern)
    |   `-- FullscreenTriangle() -> FullscreenTriangle        src/FullscreenTriangle.cpp
    |       `-- glGenVertexArrays(1, &vao_)                   (empty VAO for attrib-less draw)
    |
    |   // Render one depth texture per sampled point light.
    |-- shadowPass.render(scene) -> std::vector<ShadowMap> shadowMaps
    |   |                                                   src/ShadowPass.cpp
    |   `-- for each PointLight light in scene.lights
    |       |-- ShadowMap(shadowMapSize) -> ShadowMap         src/ShadowMap.cpp
    |       |   |-- glGenFramebuffers + glGenTextures(DEPTH_COMPONENT24)
    |       |   |-- set tex params: NEAREST, CLAMP_TO_BORDER
    |       |   |-- attach depth texture to FBO (no color buffer)
    |       |   `-- glCheckFramebufferStatus (validate complete)
    |       `-- ShadowMap::render(meshes, shader, lightPosition, settings)
    |           |                                           src/ShadowMap.cpp
    |           |-- makeLightViewProjection(lightPosition, settings) -> Mat4
    |           |   |                                       src/ShadowMap.cpp (static)
    |           |   |-- perspective(fovY, 1.0, near, far) -> Mat4
    |           |   `-- lookAt(lightPos, target, up) -> Mat4
    |           |-- shader.use() -> void
    |           |-- shader.setMat4("uLightViewProjection", mat) -> void
    |           `-- for each non-emissive GpuMesh
    |               `-- GpuMesh::draw() -> void               src/GpuMesh.cpp
    |
    |   // Rasterize scene meshes once, filling all G-buffer attachments.
    |-- geometryPass.render(config, scene, geometryBuffer) -> void
    |   |                                                   src/GeometryPass.cpp
    |   |-- geometryBuffer.bind() -> void                     src/GeometryBuffer.cpp
    |   |-- makeView(scene.camera) -> Mat4 view               src/GeometryPass.cpp (static)
    |   |   `-- lookAt(position, target, up) -> Mat4
    |   |-- makeProjection(scene.camera, config) -> Mat4 proj src/GeometryPass.cpp (static)
    |   |   `-- perspective(fovY, aspect, near, far) -> Mat4
    |   |-- shader_.setMat4("uView", view) -> void            src/ShaderProgram.cpp
    |   |-- shader_.setMat4("uProjection", proj) -> void      src/ShaderProgram.cpp
    |   `-- for each GpuMesh mesh in scene.meshes
    |       |-- shader_.setBool("uEmissive", mesh.emissive()) -> void
    |       `-- GpuMesh::draw() -> void                       src/GpuMesh.cpp
    |
    |   // Write neutral visibility (1.0) into the AO buffer.
    |-- aoPass.render(aoBuffer) -> void                       src/AOPass.cpp
    |   `-- AOBuffer::clearNeutral() -> void                  src/AOBuffer.cpp
    |
    |   // Accumulate deferred lighting from every point light.
    |-- lightingPass.render(config, scene, shadowMaps, geometryBuffer, aoBuffer, lightingBuffer)
    |   |   -> void                                          src/LightingPass.cpp
    |   |-- lightingBuffer.bind() -> void                     src/LightingBuffer.cpp
    |   |-- bindSurfaceInputs(shader_, geometryBuffer, aoBuffer) -> void
    |   |   |                                              src/LightingPass.cpp (static)
    |   |   |-- GeometryBuffer::bindPosition(GL_TEXTURE0) -> void
    |   |   |-- GeometryBuffer::bindNormal(GL_TEXTURE1) -> void
    |   |   |-- GeometryBuffer::bindMaterial(GL_TEXTURE2) -> void
    |   |   |-- GeometryBuffer::bindDepth(GL_TEXTURE3) -> void
    |   |   `-- AOBuffer::bindTexture(GL_TEXTURE4) -> void
    |   |-- shader_.setVec3("uCameraPosition", pos) -> void   src/ShaderProgram.cpp
    |   |-- shader_.setFloat("uAmbientStrength", val) -> void
    |   |-- shader_.setFloat("uLightIntensity", val) -> void
    |   `-- for each PointLight light with index i
    |       |-- configureAccumulation(i == 0) -> void         src/LightingPass.cpp (static)
    |       |-- bindLight(shader_, light, shadowMaps[i], lightCount, i == 0) -> void
    |       |   |                                          src/LightingPass.cpp (static)
    |       |   |-- ShadowMap::bind(GL_TEXTURE5) -> void      src/ShadowMap.cpp
    |       |   |-- shader_.setMat4("uLightViewProjection", mat) -> void
    |       |   |-- shader_.setVec3("uLightPosition", pos) -> void
    |       |   |-- shader_.setVec3("uLightColor", color) -> void
    |       |   `-- shader_.setFloat("uInvLightCount", 1.0f/N) -> void
    |       `-- fullscreenTriangle_.draw() -> void            src/FullscreenTriangle.cpp
    |
    |   // Read render targets and write final + debug PPM images to disk.
    `-- outputWriter.write(config, geometryBuffer, aoBuffer, lightingBuffer) -> void
        |                                                   src/RenderOutputWriter.cpp
        |-- LightingBuffer::writeColor(config.colorOutput) -> void
        |   |                                                   src/LightingBuffer.cpp
        |   `-- FramebufferSupport::writePpm(path, w, h, rgb) -> void
        |                                                       src/FramebufferSupport.hpp
        |-- GeometryBuffer::writeNormalDebug(config.normalOutput) -> void
        |   |                                                   src/GeometryBuffer.cpp
        |   `-- FramebufferSupport::writePpm(path, w, h, rgb) -> void
        |-- GeometryBuffer::writeDepthDebug(config.depthOutput) -> void
        |   |                                                   src/GeometryBuffer.cpp
        |   `-- FramebufferSupport::writePpm(path, w, h, rgb) -> void
        `-- AOBuffer::writeDebug(config.aoOutput) -> void       src/AOBuffer.cpp
            `-- FramebufferSupport::writePpm(path, w, h, rgb) -> void
```

The G-buffer stores:

- world-space position in `RGB32F`;
- world-space normal in `RGB16F`;
- albedo and emissive flag in `RGBA16F`;
- hardware depth in `DEPTH_COMPONENT24`.

World-space geometry data keeps the existing shadow calculation direct. The
SSAO implementation can transform position and normal into view space inside
`AOPass` before constructing its sample basis.

## Code Structure

- `Renderer`: contains only top-level resource creation and pass ordering.
- `Scene`: owns parsed scene settings, uploaded meshes, and sampled lights.
- `ShadowPass`: creates and renders the per-light shadow maps.
- `GeometryPass`: rasterizes scene meshes into the G-buffer.
- `AOPass`: owns the AO stage; currently writes neutral visibility.
- `LightingPass`: owns deferred lighting, texture binding, and light accumulation.
- `RenderOutputWriter`: writes final color and debug attachments to disk.
- `OpenGLHelpers`: owns the headless context type and OpenGL error checking.
- `GeometryBuffer`: owns position, normal, material, and depth attachments.
- `AOBuffer`: owns the single-channel AO attachment.
- `LightingBuffer`: owns the final HDR color attachment.
- `FramebufferSupport.hpp`: header-only internal texture, FBO validation, and
  PPM helpers shared by the buffer implementations.
- `FullscreenTriangle`: shared draw primitive for screen-space passes.
- `GeometryShaders`, `LightingShaders`, and `ShadowShaders`: shader sources
  grouped by their owning pass.
- `ShadowMap`: depth target and rendering for one sampled point light.
- `ShaderProgram`: shader compilation, linking, and uniform helpers.
- `GpuMesh`: owns one mesh VAO and VBO.

The AO target is already sampled by the lighting shader only for the ambient
term:

```glsl
ambient = albedo * ambientStrength * ao;
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

The next implementation is isolated to `AOPass`:

1. generate a hemisphere sample kernel and rotation-noise texture;
2. read G-buffer position, normal, and depth;
3. transform geometry into view space and evaluate visibility;
4. write visibility into `AOBuffer`;
5. add a separate edge-aware blur target before lighting consumes AO.

No geometry, shadow-map, or direct-lighting restructuring should be necessary
for that step.
