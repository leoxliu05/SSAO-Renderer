# SSAO Renderer

Headless OpenGL renderer for the GAMES101 final project. The current version
contains a deferred-rendering pipeline prepared for SSAO, but the SSAO sampling
algorithm is intentionally not implemented yet.

## Pipeline

The complete execution tree follows the current code path. OpenGL state calls
are shown under the project function that issues them.

```cpp
main(argc, argv) -> int                                           src/main.cpp
|-- parseAppConfig(argc, argv) -> AppConfig                       src/AppConfig.cpp
|   |-- parse command-line arguments into AppConfig
|   `-- validate dimensions, light samples, shadow size, and lighting values
|
`-- Renderer::render(config) -> void                              src/Renderer.cpp
    |
    |-- OpenGlContext::OpenGlContext() -> initialized OpenGlContext
    |   |-- CGLChoosePixelFormat(...)
    |   |-- CGLCreateContext(...)
    |   |-- CGLDestroyPixelFormat(...)
    |   |-- CGLSetCurrentContext(...)
    |   |-- glewInit()
    |   `-- glGetError()  // clear the benign GLEW core-profile error
    |
    |-- loadScene(config.modelDir) -> Scene                       src/SceneLoader.cpp
    |   |-- open <modelDir>/scene.json
    |   |-- parse JSON
    |   |-- read scene objects and resolve OBJ paths
    |   |-- read camera settings
    |   |-- read rectangular area-light settings
    |   |-- read shadow-camera settings
    |   `-- validate scene values
    |
    |-- uploadSceneMeshes(scene) -> std::vector<GpuMesh>          src/Renderer.cpp
    |   `-- for each SceneObject
    |       |-- loadObjMesh(objPath, color, positionOffset) -> std::vector<Vertex>
    |       |                                                       src/ObjLoader.cpp
    |       `-- GpuMesh::GpuMesh(name, vertices, emissive) -> GpuMesh
    |                                                               src/GpuMesh.cpp
    |           |-- glGenVertexArrays(...)
    |           |-- glGenBuffers(...)
    |           |-- glBindVertexArray(...)
    |           |-- glBindBuffer(GL_ARRAY_BUFFER, ...)
    |           |-- glBufferData(..., GL_STATIC_DRAW)
    |           |-- glEnableVertexAttribArray(0)  // position
    |           |-- glVertexAttribPointer(0, ...)
    |           |-- glEnableVertexAttribArray(1)  // normal
    |           |-- glVertexAttribPointer(1, ...)
    |           |-- glEnableVertexAttribArray(2)  // color
    |           |-- glVertexAttribPointer(2, ...)
    |           `-- glBindVertexArray(0)
    |
    |-- sampleAreaLight(areaLight, samplesPerSide) -> std::vector<PointLight>
    |                                                               src/AreaLight.cpp
    |   `-- generate samplesPerSide x samplesPerSide PointLights
    |
    |-- ShaderProgram::ShaderProgram(
    |       geometryVertex, geometryFragment) -> ShaderProgram    src/ShaderProgram.cpp
    |   |-- compileShader(GL_VERTEX_SHADER, geometryVertex) -> GLuint
    |   |-- compileShader(GL_FRAGMENT_SHADER, geometryFragment) -> GLuint
    |   `-- link the geometry program
    |
    |-- ShaderProgram::ShaderProgram(
    |       fullscreenVertex, lightingFragment) -> ShaderProgram  src/ShaderProgram.cpp
    |   |-- compileShader(GL_VERTEX_SHADER, fullscreenVertex) -> GLuint
    |   |-- compileShader(GL_FRAGMENT_SHADER, lightingFragment) -> GLuint
    |   `-- link the fullscreen lighting program
    |
    |-- ShaderProgram::ShaderProgram(shadowVertex, shadowFragment) -> ShaderProgram
    |                                                               src/ShaderProgram.cpp
    |   |-- compileShader(GL_VERTEX_SHADER, shadowVertex) -> GLuint
    |   |-- compileShader(GL_FRAGMENT_SHADER, shadowFragment) -> GLuint
    |   `-- link the shadow program
    |
    |-- renderShadowMaps(config, meshes, lights, shadowShader, settings) -> std::vector<ShadowMap>
    |                                                               src/Renderer.cpp
    |   `-- for each PointLight
    |       |-- ShadowMap::ShadowMap(config.shadowMapSize) -> ShadowMap
    |       |                                                       src/ShadowMap.cpp
    |       |   |-- glGenFramebuffers(...)
    |       |   |-- glGenTextures(...)
    |       |   |-- glBindTexture(GL_TEXTURE_2D, depthTexture)
    |       |   |-- glTexImage2D(..., GL_DEPTH_COMPONENT24, ...)
    |       |   |-- glTexParameteri(...)  // nearest, clamp to border
    |       |   |-- glBindFramebuffer(GL_FRAMEBUFFER, fbo)
    |       |   |-- glFramebufferTexture2D(GL_DEPTH_ATTACHMENT, ...)
    |       |   |-- glDrawBuffer(GL_NONE)
    |       |   |-- glReadBuffer(GL_NONE)
    |       |   `-- glCheckFramebufferStatus(GL_FRAMEBUFFER)
    |       |
    |       `-- ShadowMap::render(meshes, shader, lightPosition, settings) -> void
    |               // writes the light-space depth texture
    |           |-- makeLightViewProjection(lightPosition, settings) -> Mat4
    |           |   |-- perspective(...) -> Mat4
    |           |   |-- lookAt(...) -> Mat4
    |           |   `-- projection * view
    |           |-- glViewport(0, 0, shadowMapSize, shadowMapSize)
    |           |-- glBindFramebuffer(GL_FRAMEBUFFER, shadowFbo)
    |           |-- glDrawBuffer(GL_NONE)
    |           |-- glReadBuffer(GL_NONE)
    |           |-- glEnable(GL_DEPTH_TEST)
    |           |-- glDisable(GL_CULL_FACE)
    |           |-- glClear(GL_DEPTH_BUFFER_BIT)
    |           |-- ShaderProgram::use() -> void; binds shadowProgram
    |           |   `-- glUseProgram(shadowProgram)
    |           |-- ShaderProgram::setMat4("uLightViewProjection", ...)
    |           `-- for each non-emissive GpuMesh
    |               `-- GpuMesh::draw() -> void; submits mesh triangles
    |                   |-- glBindVertexArray(meshVao)
    |                   `-- glDrawArrays(GL_TRIANGLES, 0, vertexCount)
    |
    |-- GeometryBuffer::GeometryBuffer(width, height) -> GeometryBuffer
    |                                                               src/GeometryBuffer.cpp
    |   |-- FramebufferSupport::createTexture(..., GL_RGB32F, GL_RGB)
    |   |       -> GLuint  // position
    |   |-- FramebufferSupport::createTexture(..., GL_RGB16F, GL_RGB)
    |   |       -> GLuint  // normal
    |   |-- FramebufferSupport::createTexture(..., GL_RGBA16F, GL_RGBA)
    |   |       -> GLuint  // material
    |   |-- FramebufferSupport::createTexture(..., GL_DEPTH_COMPONENT24, ...)
    |   |       -> GLuint  // depth
    |   |-- glGenFramebuffers(...)
    |   |-- GeometryBuffer::bind() -> void; binds geometryFbo
    |   |   `-- glBindFramebuffer(GL_FRAMEBUFFER, geometryFbo)
    |   |-- glFramebufferTexture2D(GL_COLOR_ATTACHMENT0, position)
    |   |-- glFramebufferTexture2D(GL_COLOR_ATTACHMENT1, normal)
    |   |-- glFramebufferTexture2D(GL_COLOR_ATTACHMENT2, material)
    |   |-- glFramebufferTexture2D(GL_DEPTH_ATTACHMENT, depth)
    |   |-- glDrawBuffers(3, attachments)
    |   `-- FramebufferSupport::requireComplete("geometry framebuffer") -> void
    |
    |-- AmbientOcclusionBuffer::AmbientOcclusionBuffer(width, height) -> AmbientOcclusionBuffer
    |                                                     src/AmbientOcclusionBuffer.cpp
    |   |-- FramebufferSupport::createTexture(..., GL_R16F, GL_RED) -> GLuint
    |   |-- glGenFramebuffers(...)
    |   |-- glBindFramebuffer(GL_FRAMEBUFFER, aoFbo)
    |   |-- glFramebufferTexture2D(GL_COLOR_ATTACHMENT0, aoTexture)
    |   |-- glDrawBuffer(GL_COLOR_ATTACHMENT0)
    |   `-- FramebufferSupport::requireComplete("ambient occlusion framebuffer")
    |           -> void
    |
    |-- LightingBuffer::LightingBuffer(width, height) -> LightingBuffer
    |                                                               src/LightingBuffer.cpp
    |   |-- FramebufferSupport::createTexture(..., GL_RGBA16F, GL_RGBA)
    |   |       -> GLuint
    |   |-- glGenFramebuffers(...)
    |   |-- LightingBuffer::bind() -> void; binds lightingFbo
    |   |   `-- glBindFramebuffer(GL_FRAMEBUFFER, lightingFbo)
    |   |-- glFramebufferTexture2D(GL_COLOR_ATTACHMENT0, colorTexture)
    |   |-- glDrawBuffer(GL_COLOR_ATTACHMENT0)
    |   `-- FramebufferSupport::requireComplete("lighting framebuffer") -> void
    |
    |-- FullscreenTriangle::FullscreenTriangle() -> FullscreenTriangle
    |                                                               src/FullscreenTriangle.cpp
    |   `-- glGenVertexArrays(...)
    |
    |-- renderGeometryPass(config, scene, meshes, shader, geometryBuffer) -> void
    |       // writes position, normal, material, and depth        src/Renderer.cpp
    |   |-- GeometryBuffer::bind() -> void; binds geometryFbo
    |   |-- glViewport(0, 0, width, height)
    |   |-- glEnable(GL_DEPTH_TEST)
    |   |-- glDepthMask(GL_TRUE)
    |   |-- glDisable(GL_BLEND)
    |   |-- glDisable(GL_CULL_FACE)
    |   |-- glClearColor(0, 0, 0, 0)
    |   |-- glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
    |   |-- ShaderProgram::use() -> void; binds geometryProgram
    |   |-- sceneView(scene.camera) -> Mat4
    |   |   `-- lookAt(camera.position, camera.target, camera.up) -> Mat4
    |   |-- ShaderProgram::setMat4("uView", view)
    |   |-- sceneProjection(scene.camera, config) -> Mat4
    |   |   `-- perspective(fov, aspect, near, far) -> Mat4
    |   |-- ShaderProgram::setMat4("uProjection", projection)
    |   `-- for each GpuMesh
    |       |-- ShaderProgram::setBool("uEmissive", mesh.emissive())
    |       `-- GpuMesh::draw() -> void; submits mesh triangles
    |           |-- glBindVertexArray(meshVao)
    |           `-- glDrawArrays(GL_TRIANGLES, 0, vertexCount)
    |
    |-- AmbientOcclusionBuffer::clearNeutral() -> void
    |       // writes AO visibility 1.0                   src/AmbientOcclusionBuffer.cpp
    |   |-- AmbientOcclusionBuffer::bind() -> void; binds aoFbo
    |   |   `-- glBindFramebuffer(GL_FRAMEBUFFER, aoFbo)
    |   |-- glClearColor(1, 1, 1, 1)
    |   `-- glClear(GL_COLOR_BUFFER_BIT)
    |       // SSAO placeholder: every pixel has visibility 1.0
    |
    |-- renderLightingPass(config, scene, lights, shadowMaps, ...) -> void
    |       // writes accumulated HDR color                       src/Renderer.cpp
    |   |-- LightingBuffer::bind() -> void; binds lightingFbo
    |   |-- glViewport(0, 0, width, height)
    |   |-- glDisable(GL_DEPTH_TEST)
    |   |-- glDepthMask(GL_FALSE)
    |   |-- glClearColor(0.02, 0.025, 0.03, 1)
    |   |-- glClear(GL_COLOR_BUFFER_BIT)
    |   |-- ShaderProgram::use() -> void; binds lightingProgram
    |   |
    |   |-- bindLightingInputs(shader, geometryBuffer, aoBuffer) -> void
    |   |   |-- GeometryBuffer::bindPosition(GL_TEXTURE0)
    |   |   |-- GeometryBuffer::bindNormal(GL_TEXTURE1)
    |   |   |-- GeometryBuffer::bindMaterial(GL_TEXTURE2)
    |   |   |-- GeometryBuffer::bindDepth(GL_TEXTURE3)
    |   |   |-- AmbientOcclusionBuffer::bindTexture(GL_TEXTURE4)
    |   |   `-- ShaderProgram::setInt(...) for texture units 0 through 5
    |   |
    |   |-- set camera, background, ambient, light, shadow, and material uniforms
    |   |
    |   `-- for each lightIndex
    |       |-- configureLightAccumulation(lightIndex) -> void
    |       |   |-- first light: glDisable(GL_BLEND)
    |       |   `-- later lights:
    |       |       |-- glEnable(GL_BLEND)
    |       |       `-- glBlendFunc(GL_ONE, GL_ONE)
    |       |
    |       |-- bindLight(shader, light, shadowMap, lightCount, firstPass) -> void
    |       |   |-- ShadowMap::bind(GL_TEXTURE5) -> void
    |       |   |   |-- glActiveTexture(GL_TEXTURE5)
    |       |   |   `-- glBindTexture(GL_TEXTURE_2D, shadowDepthTexture)
    |       |   |-- ShaderProgram::setMat4("uLightViewProjection", ...)
    |       |   |-- ShaderProgram::setVec3("uLightPosition", ...)
    |       |   |-- ShaderProgram::setVec3("uLightColor", ...)
    |       |   |-- ShaderProgram::setFloat("uInvLightCount", ...)
    |       |   `-- ShaderProgram::setBool("uFirstLightingPass", ...)
    |       |
    |       `-- FullscreenTriangle::draw() -> void; submits one screen triangle
    |           |-- glBindVertexArray(fullscreenVao)
    |           `-- glDrawArrays(GL_TRIANGLES, 0, 3)
    |               `-- lightingFragment() -> void; writes vec4 oColor
    |                                                               src/ShaderSources.hpp
    |                   |-- sample position, normal, material, and depth
    |                   |-- sample neutral ambient-occlusion texture
    |                   |-- pointShadow(worldPosition, normal) -> float visibility
    |                   |   `-- 3 x 3 PCF shadow-map samples
    |                   |-- first pass: ambient + direct light
    |                   `-- later passes: direct light only
    |
    |-- LightingBuffer::writeColor(config.colorOutput) -> void
    |       // writes final PPM                                   src/LightingBuffer.cpp
    |   |-- LightingBuffer::bind() -> void; binds lightingFbo
    |   |-- glReadBuffer(GL_COLOR_ATTACHMENT0)
    |   |-- readRgbPixels(width, height) -> std::vector<unsigned char>
    |   |   `-- glReadPixels(..., GL_RGBA, GL_FLOAT, ...)
    |   `-- FramebufferSupport::writePpm(path, width, height, rgb) -> void
    |
    |-- GeometryBuffer::writeNormalDebug(config.normalOutput) -> void
    |       // writes normal PPM
    |   |-- GeometryBuffer::bind() -> void; binds geometryFbo
    |   |-- glReadBuffer(GL_COLOR_ATTACHMENT1)
    |   |-- readNormalPixels(width, height) -> std::vector<unsigned char>
    |   |   `-- glReadPixels(..., GL_RGB, GL_FLOAT, ...)
    |   `-- FramebufferSupport::writePpm(path, width, height, rgb) -> void
    |
    |-- GeometryBuffer::writeDepthDebug(config.depthOutput) -> void
    |       // writes depth PPM
    |   |-- GeometryBuffer::bind() -> void; binds geometryFbo
    |   |-- readDepthPixels(width, height) -> std::vector<unsigned char>
    |   |   `-- glReadPixels(..., GL_DEPTH_COMPONENT, GL_FLOAT, ...)
    |   `-- FramebufferSupport::writePpm(path, width, height, rgb) -> void
    |
    `-- AmbientOcclusionBuffer::writeDebug(config.ambientOcclusionOutput) -> void
            // writes AO PPM
        |-- AmbientOcclusionBuffer::bind() -> void; binds aoFbo
        |-- glReadBuffer(GL_COLOR_ATTACHMENT0)
        |-- readScalarPixels(width, height) -> std::vector<unsigned char>
        |   `-- glReadPixels(..., GL_RED, GL_FLOAT, ...)
        `-- FramebufferSupport::writePpm(path, width, height, rgb) -> void
```

The G-buffer stores:

- world-space position in `RGB32F`;
- world-space normal in `RGB16F`;
- albedo and emissive flag in `RGBA16F`;
- hardware depth in `DEPTH_COMPONENT24`.

World-space geometry data keeps the existing shadow calculation direct. A
future SSAO pass can transform position and normal into view space with the
camera view matrix before constructing its sample basis.

## Code Structure

- `Renderer`: owns render-pass ordering and shared state transitions.
- `GeometryBuffer`: owns position, normal, material, and depth attachments.
- `AmbientOcclusionBuffer`: owns the single-channel AO attachment.
- `LightingBuffer`: owns the final HDR color attachment.
- `FramebufferSupport.hpp`: header-only internal texture, FBO validation, and
  PPM helpers shared by the buffer implementations.
- `FullscreenTriangle`: shared draw primitive for screen-space passes.
- `ShaderSources`: geometry, lighting, fullscreen, and shadow shaders.
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

The next implementation can be isolated to a dedicated screen-space pass:

1. generate a hemisphere sample kernel and rotation-noise texture;
2. read G-buffer position, normal, and depth;
3. transform geometry into view space and evaluate visibility;
4. write visibility into `AmbientOcclusionBuffer`;
5. add a separate edge-aware blur target before lighting consumes AO.

No geometry, shadow-map, or direct-lighting restructuring should be necessary
for that step.
