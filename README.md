# SSAO Renderer

Headless OpenGL renderer for the GAMES101 final project. The current version
contains a deferred-rendering pipeline prepared for SSAO, but the SSAO sampling
algorithm is intentionally not implemented yet.

## Pipeline

The complete execution tree follows the current code path. OpenGL state calls
are shown under the project function that issues them.

```cpp
main(argc, argv)                                                  src/main.cpp
|-- parseAppConfig(argc, argv)                                    src/AppConfig.cpp
|   |-- parse command-line arguments into AppConfig
|   `-- validate dimensions, light samples, shadow size, and lighting values
|
`-- Renderer::render(config)                                      src/Renderer.cpp
    |
    |-- OpenGlContext::OpenGlContext()
    |   |-- CGLChoosePixelFormat(...)
    |   |-- CGLCreateContext(...)
    |   |-- CGLDestroyPixelFormat(...)
    |   |-- CGLSetCurrentContext(...)
    |   |-- glewInit()
    |   `-- glGetError()  // clear the benign GLEW core-profile error
    |
    |-- loadScene(config.modelDir)                                src/SceneLoader.cpp
    |   |-- open <modelDir>/scene.json
    |   |-- parse JSON
    |   |-- read scene objects and resolve OBJ paths
    |   |-- read camera settings
    |   |-- read rectangular area-light settings
    |   |-- read shadow-camera settings
    |   `-- validate scene values
    |
    |-- uploadSceneMeshes(scene)                                  src/Renderer.cpp
    |   `-- for each SceneObject
    |       |-- loadObjMesh(objPath, color, positionOffset)        src/ObjLoader.cpp
    |       `-- GpuMesh::GpuMesh(name, vertices, emissive)         src/GpuMesh.cpp
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
    |-- sampleAreaLight(areaLight, samplesPerSide)                 src/AreaLight.cpp
    |   `-- generate samplesPerSide x samplesPerSide PointLights
    |
    |-- ShaderProgram::ShaderProgram(                              src/ShaderProgram.cpp
    |       geometryVertex, geometryFragment)
    |   |-- compileShader(GL_VERTEX_SHADER, geometryVertex)
    |   |-- compileShader(GL_FRAGMENT_SHADER, geometryFragment)
    |   `-- link the geometry program
    |
    |-- ShaderProgram::ShaderProgram(
    |       fullscreenVertex, lightingFragment)
    |   |-- compileShader(GL_VERTEX_SHADER, fullscreenVertex)
    |   |-- compileShader(GL_FRAGMENT_SHADER, lightingFragment)
    |   `-- link the fullscreen lighting program
    |
    |-- ShaderProgram::ShaderProgram(shadowVertex, shadowFragment)
    |   |-- compileShader(GL_VERTEX_SHADER, shadowVertex)
    |   |-- compileShader(GL_FRAGMENT_SHADER, shadowFragment)
    |   `-- link the shadow program
    |
    |-- renderShadowMaps(config, meshes, lights, shadowShader, settings)
    |   `-- for each PointLight
    |       |-- ShadowMap::ShadowMap(config.shadowMapSize)          src/ShadowMap.cpp
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
    |       `-- ShadowMap::render(meshes, shader, lightPosition, settings)
    |           |-- makeLightViewProjection(lightPosition, settings)
    |           |   |-- perspective(...)
    |           |   |-- lookAt(...)
    |           |   `-- projection * view
    |           |-- glViewport(0, 0, shadowMapSize, shadowMapSize)
    |           |-- glBindFramebuffer(GL_FRAMEBUFFER, shadowFbo)
    |           |-- glDrawBuffer(GL_NONE)
    |           |-- glReadBuffer(GL_NONE)
    |           |-- glEnable(GL_DEPTH_TEST)
    |           |-- glDisable(GL_CULL_FACE)
    |           |-- glClear(GL_DEPTH_BUFFER_BIT)
    |           |-- ShaderProgram::use()
    |           |   `-- glUseProgram(shadowProgram)
    |           |-- ShaderProgram::setMat4("uLightViewProjection", ...)
    |           `-- for each non-emissive GpuMesh
    |               `-- GpuMesh::draw()
    |                   |-- glBindVertexArray(meshVao)
    |                   `-- glDrawArrays(GL_TRIANGLES, 0, vertexCount)
    |
    |-- GeometryBuffer::GeometryBuffer(width, height)              src/GeometryBuffer.cpp
    |   |-- createTexture(..., GL_RGB32F, GL_RGB)    // position
    |   |-- createTexture(..., GL_RGB16F, GL_RGB)    // normal
    |   |-- createTexture(..., GL_RGBA16F, GL_RGBA)  // material
    |   |-- createDepthTexture(..., GL_DEPTH_COMPONENT24)
    |   |-- glGenFramebuffers(...)
    |   |-- GeometryBuffer::bind()
    |   |   `-- glBindFramebuffer(GL_FRAMEBUFFER, geometryFbo)
    |   |-- glFramebufferTexture2D(GL_COLOR_ATTACHMENT0, position)
    |   |-- glFramebufferTexture2D(GL_COLOR_ATTACHMENT1, normal)
    |   |-- glFramebufferTexture2D(GL_COLOR_ATTACHMENT2, material)
    |   |-- glFramebufferTexture2D(GL_DEPTH_ATTACHMENT, depth)
    |   |-- glDrawBuffers(3, attachments)
    |   `-- requireCompleteFramebuffer("geometry framebuffer")
    |
    |-- AmbientOcclusionBuffer::AmbientOcclusionBuffer(width, height)
    |                                                     src/AmbientOcclusionBuffer.cpp
    |   |-- createTexture(..., GL_R16F, GL_RED)
    |   |-- glGenFramebuffers(...)
    |   |-- glBindFramebuffer(GL_FRAMEBUFFER, aoFbo)
    |   |-- glFramebufferTexture2D(GL_COLOR_ATTACHMENT0, aoTexture)
    |   |-- glDrawBuffer(GL_COLOR_ATTACHMENT0)
    |   `-- requireCompleteFramebuffer("ambient occlusion framebuffer")
    |
    |-- LightingBuffer::LightingBuffer(width, height)              src/LightingBuffer.cpp
    |   |-- createTexture(..., GL_RGBA16F, GL_RGBA)
    |   |-- glGenFramebuffers(...)
    |   |-- LightingBuffer::bind()
    |   |   `-- glBindFramebuffer(GL_FRAMEBUFFER, lightingFbo)
    |   |-- glFramebufferTexture2D(GL_COLOR_ATTACHMENT0, colorTexture)
    |   |-- glDrawBuffer(GL_COLOR_ATTACHMENT0)
    |   `-- requireCompleteFramebuffer("lighting framebuffer")
    |
    |-- FullscreenTriangle::FullscreenTriangle()                   src/FullscreenTriangle.cpp
    |   `-- glGenVertexArrays(...)
    |
    |-- renderGeometryPass(config, scene, meshes, shader, geometryBuffer)
    |                                                               src/Renderer.cpp
    |   |-- GeometryBuffer::bind()
    |   |-- glViewport(0, 0, width, height)
    |   |-- glEnable(GL_DEPTH_TEST)
    |   |-- glDepthMask(GL_TRUE)
    |   |-- glDisable(GL_BLEND)
    |   |-- glDisable(GL_CULL_FACE)
    |   |-- glClearColor(0, 0, 0, 0)
    |   |-- glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
    |   |-- ShaderProgram::use()
    |   |-- sceneView(scene.camera)
    |   |   `-- lookAt(camera.position, camera.target, camera.up)
    |   |-- ShaderProgram::setMat4("uView", view)
    |   |-- sceneProjection(scene.camera, config)
    |   |   `-- perspective(fov, aspect, near, far)
    |   |-- ShaderProgram::setMat4("uProjection", projection)
    |   `-- for each GpuMesh
    |       |-- ShaderProgram::setBool("uEmissive", mesh.emissive())
    |       `-- GpuMesh::draw()
    |           |-- glBindVertexArray(meshVao)
    |           `-- glDrawArrays(GL_TRIANGLES, 0, vertexCount)
    |
    |-- AmbientOcclusionBuffer::clearNeutral()          src/AmbientOcclusionBuffer.cpp
    |   |-- AmbientOcclusionBuffer::bind()
    |   |   `-- glBindFramebuffer(GL_FRAMEBUFFER, aoFbo)
    |   |-- glClearColor(1, 1, 1, 1)
    |   `-- glClear(GL_COLOR_BUFFER_BIT)
    |       // SSAO placeholder: every pixel has visibility 1.0
    |
    |-- renderLightingPass(config, scene, lights, shadowMaps, ...) src/Renderer.cpp
    |   |-- LightingBuffer::bind()
    |   |-- glViewport(0, 0, width, height)
    |   |-- glDisable(GL_DEPTH_TEST)
    |   |-- glDepthMask(GL_FALSE)
    |   |-- glClearColor(0.02, 0.025, 0.03, 1)
    |   |-- glClear(GL_COLOR_BUFFER_BIT)
    |   |-- ShaderProgram::use()
    |   |
    |   |-- bindLightingInputs(shader, geometryBuffer, aoBuffer)
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
    |       |-- configureLightAccumulation(lightIndex)
    |       |   |-- first light: glDisable(GL_BLEND)
    |       |   `-- later lights:
    |       |       |-- glEnable(GL_BLEND)
    |       |       `-- glBlendFunc(GL_ONE, GL_ONE)
    |       |
    |       |-- bindLight(shader, light, shadowMap, lightCount, firstPass)
    |       |   |-- ShadowMap::bind(GL_TEXTURE5)
    |       |   |   |-- glActiveTexture(GL_TEXTURE5)
    |       |   |   `-- glBindTexture(GL_TEXTURE_2D, shadowDepthTexture)
    |       |   |-- ShaderProgram::setMat4("uLightViewProjection", ...)
    |       |   |-- ShaderProgram::setVec3("uLightPosition", ...)
    |       |   |-- ShaderProgram::setVec3("uLightColor", ...)
    |       |   |-- ShaderProgram::setFloat("uInvLightCount", ...)
    |       |   `-- ShaderProgram::setBool("uFirstLightingPass", ...)
    |       |
    |       `-- FullscreenTriangle::draw()
    |           |-- glBindVertexArray(fullscreenVao)
    |           `-- glDrawArrays(GL_TRIANGLES, 0, 3)
    |               `-- lightingFragment()                         src/ShaderSources.hpp
    |                   |-- sample position, normal, material, and depth
    |                   |-- sample neutral ambient-occlusion texture
    |                   |-- pointShadow(worldPosition, normal)
    |                   |   `-- 3 x 3 PCF shadow-map samples
    |                   |-- first pass: ambient + direct light
    |                   `-- later passes: direct light only
    |
    |-- LightingBuffer::writeColor(config.colorOutput)             src/LightingBuffer.cpp
    |   |-- LightingBuffer::bind()
    |   |-- glReadBuffer(GL_COLOR_ATTACHMENT0)
    |   |-- readRgbPixels(width, height)
    |   |   `-- glReadPixels(..., GL_RGBA, GL_FLOAT, ...)
    |   `-- writePpm(path, width, height, rgb)
    |
    |-- GeometryBuffer::writeNormalDebug(config.normalOutput)
    |   |-- GeometryBuffer::bind()
    |   |-- glReadBuffer(GL_COLOR_ATTACHMENT1)
    |   |-- readNormalPixels(width, height)
    |   |   `-- glReadPixels(..., GL_RGB, GL_FLOAT, ...)
    |   `-- writePpm(path, width, height, rgb)
    |
    |-- GeometryBuffer::writeDepthDebug(config.depthOutput)
    |   |-- GeometryBuffer::bind()
    |   |-- readDepthPixels(width, height)
    |   |   `-- glReadPixels(..., GL_DEPTH_COMPONENT, GL_FLOAT, ...)
    |   `-- writePpm(path, width, height, rgb)
    |
    `-- AmbientOcclusionBuffer::writeDebug(config.ambientOcclusionOutput)
        |-- AmbientOcclusionBuffer::bind()
        |-- glReadBuffer(GL_COLOR_ATTACHMENT0)
        |-- readScalarPixels(width, height)
        |   `-- glReadPixels(..., GL_RED, GL_FLOAT, ...)
        `-- writePpm(path, width, height, rgb)
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
- `FramebufferSupport`: provides shared texture, FBO validation, and PPM helpers.
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
