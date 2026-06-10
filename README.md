# SSAO Renderer

OpenGL rasterization scaffold for the GAMES101 final project. The current
version renders the HW7 Cornell Box OBJ files through a hidden macOS CGL OpenGL
context and writes framebuffer outputs to disk. SSAO can be added as a later
screen-space pass using the existing color, normal, and depth attachments.

The code is split by responsibility:

- `AppConfig`: command-line options and output paths.
- `SceneLoader`: parses meshes, camera, area light, and shadow settings from `scene.json`.
- `ObjLoader`: minimal OBJ triangle loading.
- `GpuMesh`: VAO/VBO upload and draw.
- `ShaderProgram`: shader compile/link and uniform helpers.
- `Framebuffer`: color, normal, and depth render targets plus PPM dumps.
- `AreaLight`: a configured rectangular light sampled into point lights.
- `ShadowMap`: one traditional 2D perspective depth map per sampled light.
- `Renderer`: OpenGL context setup and render-pass orchestration.

## Build

Dependencies used by CMake:

- OpenGL
- GLEW
- nlohmann-json

On macOS with Homebrew these can be installed with:

```sh
brew install glew nlohmann-json
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
`models/cornellbox/scene.json` selects the OBJ files and configures their
materials, transforms, camera, rectangular area light, and shadow projection.
The C++ renderer contains no Cornell Box object list or camera/light bounds.

## Scene Configuration

Each model directory must contain a `scene.json` with four top-level fields:

- `objects`: relative OBJ path, diffuse/emissive color, optional position
  offset, and optional `emissive` flag for every mesh.
- `camera`: position, target, up vector, vertical FOV, near plane, and far
  plane for the main rasterization pass.
- `area_light`: rectangle origin, two edge vectors, and sampled light color.
- `shadow`: target, up vector, vertical FOV, near plane, and far plane shared
  by the per-sample point-light shadow cameras.

`--model-dir` selects the directory, and `SceneLoader` always reads
`<model-dir>/scene.json`. Adding another scene therefore requires model files
and configuration data, not another scene-specific C++ source file.

Useful runtime options:

```sh
./build/SSAO_Renderer \
  --area-light-samples 8 \
  --shadow-map-size 512 \
  --ambient-strength 0.20 \
  --light-intensity 1.00 \
  --shadow-min-light 0.25 \
  --output build/render.ppm \
  --normal-output build/normal_debug.ppm \
  --depth-output build/depth_debug.ppm
```

`--area-light-samples N` samples the Cornell Box rectangle light into `N x N`
point lights. The default is `8`, so the area light is approximated with 64
point-light shadow maps. `N` is capped at 16 to avoid accidental excessive
shadow memory use. Each point light gets its own 2D perspective shadow map, and
the renderer accumulates lighting in multiple additive passes so it is not
limited by the number of simultaneously bound fragment textures.

`--ambient-strength` is a simple rasterization fill term that stands in for the
Cornell Box indirect diffuse bounce that a path tracer would normally capture.
`--light-intensity` scales direct Blinn-Phong lighting from the sampled area
light.
`--shadow-min-light` keeps a controllable amount of direct-light contribution
even for fully shadowed points, which prevents raster shadows from crushing to
black when no true indirect bounce is being computed.

Color accumulation uses an `RGBA16F` framebuffer and point-light shadow maps use
small PCF filtering to reduce additive-pass banding and hard shadow-map stripes.

## Outputs

- `build/render.ppm`: base Cornell Box rasterization.
- `build/normal_debug.ppm`: normal attachment visualized as RGB.
- `build/depth_debug.ppm`: depth attachment visualized as grayscale.

The default camera in `models/cornellbox/scene.json` matches the HW7 Cornell
Box convention: eye `(278, 273, -800)`, looking into the box.

The base render already includes shadow mapping. The Cornell Box area light is
approximated by many point lights, which gives soft-shadow behavior while still
keeping the pipeline easy to extend with a later SSAO pass.

The lighting code is organized as a Blinn-Phong model with `ka`, `kd`, `ks`,
`ambient`, `diffuse`, and `specular` terms. Cornell Box surfaces are configured
as diffuse by setting specular strength to zero, matching the usual Lambertian
Cornell Box material assumption.

## Full Pipeline

The pipeline below follows the codebase in execution order. Small OpenGL state
changes are grouped, but all project modules and important GPU calls are shown.

```text
main(argc, argv)                                    src/main.cpp
|-- parseAppConfig(argc, argv)                      src/AppConfig.cpp
|   |-- parse, apply and validate values            include/AppConfig.hpp
|   |   |-- width/height = 1024
|   |   |-- areaLightSamplesPerSide = 8
|   |   |-- shadowMapSize = 512
|   |   |-- ambientStrength = 0.20
|   |   |-- lightIntensity = 1.00
|   |   |-- shadowMinLight = 0.25
|   |   |-- modelDir = models/cornellbox
|   |   |-- outputs = render.ppm, normal_debug.ppm, depth_debug.ppm
|   |-- return AppConfig
|
|-- Renderer renderer
|-- renderer.render(config)                         src/Renderer.cpp
|   |
|   |-- OpenGlContext context
|   |   |-- CGLChoosePixelFormat(OpenGL 3.2 core, accelerated, color/depth/alpha)
|   |   |-- CGLCreateContext(...)
|   |   |-- CGLDestroyPixelFormat(...)
|   |   |-- CGLSetCurrentContext(...)
|   |   |-- glewExperimental = GL_TRUE
|   |   |-- glewInit()
|   |   |-- glGetError() to clear benign GLEW core-profile error
|   |
|   |-- ShaderProgram shader(kVertexShader, kFragmentShader)             src/ShaderProgram.cpp
|   |   |-- ShaderProgram(vertexSource, nullptr, fragmentSource)
|   |   |   |-- compileShader(GL_VERTEX_SHADER, vertexSource)
|   |   |   |   |-- glCreateShader
|   |   |   |   |-- glShaderSource
|   |   |   |   |-- glCompileShader
|   |   |   |   |-- glGetShaderiv(GL_COMPILE_STATUS)
|   |   |   |   |-- on failure: glGetShaderInfoLog, glDeleteShader, throw
|   |   |   |-- compileShader(GL_FRAGMENT_SHADER, fragmentSource)
|   |   |   |-- glCreateProgram
|   |   |   |-- glAttachShader(vertex)
|   |   |   |-- glAttachShader(fragment)
|   |   |   |-- glLinkProgram
|   |   |   |-- glDeleteShader(vertex/fragment)
|   |   |   |-- glGetProgramiv(GL_LINK_STATUS)
|   |   |   |-- on failure: glGetProgramInfoLog, glDeleteProgram, throw
|   |
|   |-- ShaderProgram shadowShader(kShadowVertexShader, kShadowFragmentShader)
|   |   |-- same compile/link path as above
|   |
|   |-- loadScene(config.modelDir)                                      src/SceneLoader.cpp
|   |   |-- open modelDir/scene.json
|   |   |-- nlohmann::json::parse(input)
|   |   |-- readObjects(root, modelDir)
|   |   |   |-- read mesh, color, optional offset, optional emissive
|   |   |   |-- resolve each relative mesh path against modelDir
|   |   |-- readCamera(root)
|   |   |   |-- read position, target, up, FOV, near, far
|   |   |-- readAreaLight(root)
|   |   |   |-- read origin, edge_u, edge_v, color
|   |   |-- readShadowSettings(root)
|   |   |   |-- read target, up, FOV, near, far
|   |   |-- validate arrays, finite values, ranges, and non-parallel light edges
|   |   |-- return Scene
|   |
|   |-- uploadSceneMeshes(scene)
|   |   |-- for each SceneObject
|   |   |   |-- loadObjMesh(objPath, color, positionOffset)              src/ObjLoader.cpp
|   |   |   |   |-- open OBJ file
|   |   |   |   |-- throw if mesh has no triangles
|   |   |   |-- GpuMesh(name, vertices, emissive)                        src/GpuMesh.cpp
|   |   |       |-- glGenVertexArrays
|   |   |       |-- glGenBuffers
|   |   |       |-- glBindVertexArray
|   |   |       |-- glBindBuffer(GL_ARRAY_BUFFER)
|   |   |       |-- glBufferData(vertices, GL_STATIC_DRAW)
|   |   |       |-- glEnableVertexAttribArray(0), position
|   |   |       |-- glVertexAttribPointer(0, offsetof(Vertex, position))
|   |   |       |-- glEnableVertexAttribArray(1), normal
|   |   |       |-- glVertexAttribPointer(1, offsetof(Vertex, normal))
|   |   |       |-- glEnableVertexAttribArray(2), color
|   |   |       |-- glVertexAttribPointer(2, offsetof(Vertex, color))
|   |   |       |-- glBindVertexArray(0)
|   |   |-- return vector<GpuMesh>
|   |
|   |-- sampleAreaLight(scene.areaLight, config.areaLightSamplesPerSide) src/AreaLight.cpp
|   |   |-- samplesPerSide = max(samplesPerSide, 1)
|   |   |-- for row in samplesPerSide
|   |   |   |-- for col in samplesPerSide
|   |   |       |-- sample cell center using origin + edge_u * u + edge_v * v
|   |   |       |-- push PointLight{position, configured color}           include/AreaLight.hpp
|   |   |-- return vector<PointLight>
|   |
|   |-- renderShadowMaps(config, meshes, lights, shadowShader, scene.shadow)
|   |   |-- for each PointLight
|   |   |   |-- ShadowMap(config.shadowMapSize)                        src/ShadowMap.cpp
|   |   |   |   |-- glGenFramebuffers
|   |   |   |   |-- glGenTextures
|   |   |   |   |-- glBindTexture(GL_TEXTURE_2D)
|   |   |   |   |-- glTexImage2D(GL_DEPTH_COMPONENT24)
|   |   |   |   |-- glTexParameteri(nearest filtering, clamp to white border)
|   |   |   |   |-- glTexParameteri(GL_TEXTURE_COMPARE_MODE, GL_NONE)
|   |   |   |   |-- glBindFramebuffer
|   |   |   |   |-- glFramebufferTexture2D(GL_DEPTH_ATTACHMENT, depth texture)
|   |   |   |   |-- glDrawBuffer(GL_NONE)
|   |   |   |   |-- glReadBuffer(GL_NONE)
|   |   |   |   |-- glCheckFramebufferStatus
|   |   |   |-- ShadowMap::render(meshes, shadowShader, light.position, scene.shadow)
|   |   |       |-- projection = perspective(configured FOV/near/far, aspect 1)   include/Math.hpp
|   |   |       |-- view = lookAt(lightPosition, configured target/up)             include/Math.hpp
|   |   |       |-- store lightViewProjection = projection * view
|   |   |       |-- glViewport(0, 0, shadowMapSize, shadowMapSize)
|   |   |       |-- glBindFramebuffer(shadow FBO)
|   |   |       |-- glDrawBuffer(GL_NONE), glReadBuffer(GL_NONE)
|   |   |       |-- glEnable(GL_DEPTH_TEST), glDisable(GL_CULL_FACE)
|   |   |       |-- glClear(GL_DEPTH_BUFFER_BIT)
|   |   |       |-- shadowShader.use()
|   |   |       |   |-- glUseProgram
|   |   |       |-- shadowShader.setMat4("uLightViewProjection", stored matrix)
|   |   |       |-- for each GpuMesh
|   |   |           |-- if mesh.emissive(): skip
|   |   |           |-- mesh.draw()
|   |   |               |-- glBindVertexArray
|   |   |               |-- glDrawArrays(GL_TRIANGLES)
|   |   |       |-- shadow vertex shader: world position -> light clip position
|   |   |       |-- normal depth testing writes projected depth automatically
|   |   |       |-- glBindVertexArray(0)
|   |   |       |-- glBindFramebuffer(0)
|   |   |-- return vector<ShadowMap>
|   |
|   |-- Framebuffer framebuffer(config.width, config.height)             src/Framebuffer.cpp
|   |   |-- createColorTexture(width, height, GL_RGBA16F, GL_RGBA, GL_FLOAT)
|   |   |   |-- glGenTextures
|   |   |   |-- glBindTexture(GL_TEXTURE_2D)
|   |   |   |-- glTexImage2D
|   |   |   |-- glTexParameteri(nearest filtering, clamp to edge)
|   |   |-- createColorTexture(width, height, GL_RGBA16F, GL_RGBA, GL_FLOAT) for normals
|   |   |-- createDepthTexture(width, height)
|   |   |   |-- glTexImage2D(GL_DEPTH_COMPONENT24)
|   |   |-- glGenFramebuffers
|   |   |-- bind()
|   |   |   |-- glBindFramebuffer(main FBO)
|   |   |-- glFramebufferTexture2D(GL_COLOR_ATTACHMENT0, colorTexture)
|   |   |-- glFramebufferTexture2D(GL_COLOR_ATTACHMENT1, normalTexture)
|   |   |-- glFramebufferTexture2D(GL_DEPTH_ATTACHMENT, depthTexture)
|   |   |-- glDrawBuffers(color0, color1)
|   |   |-- glCheckFramebufferStatus
|   |
|   |-- Main camera pass setup
|   |   |-- framebuffer.bind()
|   |   |-- glViewport(0, 0, width, height)
|   |   |-- glEnable(GL_DEPTH_TEST)
|   |   |-- glDisable(GL_CULL_FACE)
|   |   |-- glClearColor(...)
|   |   |-- shader.use()
|   |   |-- shader.setMat4("uView", sceneView(scene.camera))
|   |   |   |-- lookAt(configured position, target, up)                  include/Math.hpp
|   |   |-- shader.setMat4("uProjection", sceneProjection(scene.camera, config))
|   |   |   |-- perspective(configured FOV/near/far, output aspect)      include/Math.hpp
|   |   |-- shader.setVec3("uCameraPosition", scene.camera.position)
|   |   |-- shader.setFloat("uAmbientStrength", config.ambientStrength)
|   |   |-- shader.setFloat("uLightIntensity", config.lightIntensity)
|   |   |-- shader.setFloat("uShadowMinLight", config.shadowMinLight)
|   |   |-- shader.setFloat("uShininess", 32.0)
|   |   |-- shader.setFloat("uSpecularStrength", 0.0)
|   |
|   |-- for each lightIndex in lights
|   |   |-- configureLightingPass(lightIndex)
|   |   |   |-- if first pass
|   |   |   |   |-- glDisable(GL_BLEND)
|   |   |   |   |-- glDepthMask(GL_TRUE)
|   |   |   |   |-- glDepthFunc(GL_LESS)
|   |   |   |   |-- glColorMaski(color attachment 0 and 1 enabled)
|   |   |   |   |-- glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
|   |   |   |-- else additive pass
|   |   |       |-- glEnable(GL_BLEND)
|   |   |       |-- glBlendFunc(GL_ONE, GL_ONE)
|   |   |       |-- glDepthMask(GL_FALSE)
|   |   |       |-- glDepthFunc(GL_LEQUAL)
|   |   |       |-- glColorMaski(attachment 0 enabled)
|   |   |       |-- glColorMaski(attachment 1 disabled)
|   |   |-- bindLightUniforms(shader, lights[lightIndex], shadowMaps[lightIndex], ...)
|   |   |   |-- shadowMap.bind(GL_TEXTURE0)
|   |   |   |   |-- glActiveTexture
|   |   |   |   |-- glBindTexture(GL_TEXTURE_2D)
|   |   |   |-- shader.setInt("uShadowMap", 0)
|   |   |   |-- shader.setMat4("uLightViewProjection", shadowMap.lightViewProjection())
|   |   |   |-- shader.setVec3("uLightPosition", light.position)
|   |   |   |-- shader.setVec3("uLightColor", light.color)
|   |   |   |-- shader.setFloat("uInvLightCount", 1.0 / lights.size())
|   |   |   |-- shader.setBool("uFirstLightingPass", lightIndex == 0)
|   |   |-- for each GpuMesh
|   |       |-- shader.setBool("uEmissive", mesh.emissive())
|   |       |-- mesh.draw()
|   |           |-- main vertex shader
|   |           |   |-- read aPosition, aNormal, aColor from VAO attributes
|   |           |   |-- output vWorldPosition, vNormal, vKd
|   |           |   |-- output vLightSpacePosition = light VP * world position
|   |           |   |-- gl_Position = uProjection * uView * vec4(aPosition, 1)
|   |           |-- rasterization
|   |           |-- main fragment shader
|   |               |-- if emissive
|   |               |   |-- first pass writes warm visible light color
|   |               |   |-- later passes write zero
|   |               |-- else Blinn-Phong-style diffuse material
|   |                   |-- ka = vKd, kd = vKd, ks = vec3(uSpecularStrength)
|   |                   |-- compute light direction, view direction, half vector
|   |                   |-- diffuseFactor = max(abs(dot(n, l)), 0)
|   |                   |-- specularFactor = pow(max(abs(dot(n, h)), 0), shininess)
|   |                   |-- pointShadow(n)
|   |                   |   |-- perspective divide vLightSpacePosition
|   |                   |   |-- map light NDC to [0, 1] UV/depth
|   |                   |   |-- outside the light frustum => visible
|   |                   |   |-- compute normal-dependent bias
|   |                   |   |-- sample sampler2D uShadowMap with 3x3 PCF
|   |                   |   |-- return averaged visibility
|   |                   |-- shadow = mix(uShadowMinLight, 1, visibility)
|   |                   |-- ambient only on first lighting pass
|   |                   |-- diffuse + specular scaled by shadow, intensity, and uInvLightCount
|   |               |-- write oColor to attachment 0
|   |               |-- write encoded normal to attachment 1 on first pass only
|   |
|   |-- Restore state and validate
|   |   |-- glBindVertexArray(0)
|   |   |-- glDisable(GL_BLEND)
|   |   |-- glDepthMask(GL_TRUE)
|   |   |-- glDepthFunc(GL_LESS)
|   |   |-- glColorMaski(attachment 0 and 1 enabled)
|   |   |-- checkGl("render")
|   |       |-- glGetError()
|   |       |-- throw if error != GL_NO_ERROR
|   |
|   |-- framebuffer.writeColor(config.colorOutput)
|   |   |-- bind()
|   |   |-- glReadBuffer(GL_COLOR_ATTACHMENT0)
|   |   |-- readColorPixels(width, height)
|   |   |   |-- glReadPixels(GL_RGBA, GL_FLOAT)
|   |   |   |-- clamp floats to [0, 1]
|   |   |   |-- convert to 8-bit RGB
|   |   |-- writePpm(path, width, height, rgb)
|   |       |-- write P6 header
|   |       |-- write rows flipped vertically
|   |
|   |-- framebuffer.writeNormalDebug(config.normalOutput)
|   |   |-- glReadBuffer(GL_COLOR_ATTACHMENT1)
|   |   |-- readNormalPixels()
|   |   |   |-- glReadPixels(GL_RGBA, GL_FLOAT)
|   |   |   |-- clamp/convert to RGB
|   |   |-- writePpm(...)
|   |
|   |-- framebuffer.writeDepthDebug(config.depthOutput)
|   |   |-- readDepthPixels()
|   |   |-- writePpm(...)
|   |
|   |-- RAII cleanup while leaving Renderer::render
|       |-- Framebuffer::~Framebuffer()
|       |-- ShadowMap::~ShadowMap()
|       |-- GpuMesh::~GpuMesh()
|       |-- ShaderProgram::~ShaderProgram()
|       |-- OpenGlContext::~OpenGlContext()
|
|-- return 0 on success
```

Notes on the shared data structures:

```text
Vertex                                  include/Vertex.hpp
|-- Vec3 position
|-- Vec3 normal
|-- Vec3 color

Scene / SceneObject                     include/Scene.hpp
|-- objPath
|-- color
|-- positionOffset
|-- emissive
|-- SceneCamera
|-- RectAreaLight
|-- ShadowSettings

PointLight                              include/AreaLight.hpp
|-- position
|-- color

Vec3 / Mat4 and inline math             include/Math.hpp
|-- used by OBJ loading, camera setup, light sampling, and shadow matrices
```
