#include "Renderer.hpp"

#include "AmbientOcclusionBuffer.hpp"
#include "AreaLight.hpp"
#include "FullscreenTriangle.hpp"
#include "GeometryBuffer.hpp"
#include "GpuMesh.hpp"
#include "LightingBuffer.hpp"
#include "Math.hpp"
#include "ObjLoader.hpp"
#include "Scene.hpp"
#include "ShaderProgram.hpp"
#include "ShaderSources.hpp"
#include "ShadowMap.hpp"

#include <GL/glew.h>
#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#endif

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr GLenum kPositionUnit = GL_TEXTURE0;
constexpr GLenum kNormalUnit = GL_TEXTURE1;
constexpr GLenum kMaterialUnit = GL_TEXTURE2;
constexpr GLenum kDepthUnit = GL_TEXTURE3;
constexpr GLenum kAmbientOcclusionUnit = GL_TEXTURE4;
constexpr GLenum kShadowUnit = GL_TEXTURE5;

class OpenGlContext {
public:
    OpenGlContext()
    {
#ifdef __APPLE__
        CGLPixelFormatAttribute attributes[] = {
            kCGLPFAOpenGLProfile,
            static_cast<CGLPixelFormatAttribute>(kCGLOGLPVersion_3_2_Core),
            kCGLPFAAccelerated,
            kCGLPFAColorSize,
            static_cast<CGLPixelFormatAttribute>(24),
            kCGLPFADepthSize,
            static_cast<CGLPixelFormatAttribute>(24),
            kCGLPFAAlphaSize,
            static_cast<CGLPixelFormatAttribute>(8),
            static_cast<CGLPixelFormatAttribute>(0),
        };

        CGLPixelFormatObj pixelFormat = nullptr;
        GLint pixelFormatCount = 0;
        CGLError error = CGLChoosePixelFormat(attributes, &pixelFormat, &pixelFormatCount);
        if (error != kCGLNoError || pixelFormat == nullptr) {
            throw std::runtime_error("failed to choose CGL pixel format");
        }

        error = CGLCreateContext(pixelFormat, nullptr, &context_);
        CGLDestroyPixelFormat(pixelFormat);
        if (error != kCGLNoError || context_ == nullptr) {
            throw std::runtime_error("failed to create CGL context");
        }

        error = CGLSetCurrentContext(context_);
        if (error != kCGLNoError) {
            throw std::runtime_error("failed to make CGL context current");
        }
#else
        throw std::runtime_error(
            "headless OpenGL context setup is implemented for macOS in this scaffold");
#endif

        glewExperimental = GL_TRUE;
        const GLenum glewError = glewInit();
        glGetError();
        if (glewError != GLEW_OK) {
            throw std::runtime_error(
                reinterpret_cast<const char*>(glewGetErrorString(glewError)));
        }
    }

    ~OpenGlContext()
    {
#ifdef __APPLE__
        CGLSetCurrentContext(nullptr);
        if (context_ != nullptr) {
            CGLDestroyContext(context_);
        }
#endif
    }

    OpenGlContext(const OpenGlContext&) = delete;
    OpenGlContext& operator=(const OpenGlContext&) = delete;

private:
#ifdef __APPLE__
    CGLContextObj context_ = nullptr;
#endif
};

void checkGl(const std::string& label)
{
    const GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::ostringstream message;
        message << label << " failed with GL error 0x" << std::hex << error;
        throw std::runtime_error(message.str());
    }
}

std::vector<GpuMesh> uploadSceneMeshes(const Scene& scene)
{
    std::vector<GpuMesh> meshes;
    meshes.reserve(scene.objects.size());
    for (const SceneObject& object : scene.objects) {
        std::vector<Vertex> vertices = loadObjMesh(
            object.objPath, object.color, object.positionOffset);
        meshes.emplace_back(
            object.objPath.filename().string(), vertices, object.emissive);
    }
    return meshes;
}

Mat4 sceneView(const SceneCamera& camera)
{
    return lookAt(camera.position, camera.target, camera.up);
}

Mat4 sceneProjection(const SceneCamera& camera, const AppConfig& config)
{
    return perspective(
        radians(camera.fovYDegrees),
        static_cast<float>(config.width) / static_cast<float>(config.height),
        camera.nearPlane,
        camera.farPlane);
}

std::vector<ShadowMap> renderShadowMaps(const AppConfig& config,
    const std::vector<GpuMesh>& meshes,
    const std::vector<PointLight>& lights,
    const ShaderProgram& shader,
    const ShadowSettings& settings)
{
    std::vector<ShadowMap> shadowMaps;
    shadowMaps.reserve(lights.size());

    for (const PointLight& light : lights) {
        shadowMaps.emplace_back(config.shadowMapSize);
        shadowMaps.back().render(meshes, shader, light.position, settings);
    }
    return shadowMaps;
}

void renderGeometryPass(const AppConfig& config,
    const Scene& scene,
    const std::vector<GpuMesh>& meshes,
    const ShaderProgram& shader,
    const GeometryBuffer& geometryBuffer)
{
    geometryBuffer.bind();
    glViewport(0, 0, config.width, config.height);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shader.use();
    shader.setMat4("uView", sceneView(scene.camera));
    shader.setMat4("uProjection", sceneProjection(scene.camera, config));

    for (const GpuMesh& mesh : meshes) {
        shader.setBool("uEmissive", mesh.emissive());
        mesh.draw();
    }
    glBindVertexArray(0);
    checkGl("geometry pass");
}

void bindLightingInputs(const ShaderProgram& shader,
    const GeometryBuffer& geometryBuffer,
    const AmbientOcclusionBuffer& ambientOcclusionBuffer)
{
    geometryBuffer.bindPosition(kPositionUnit);
    geometryBuffer.bindNormal(kNormalUnit);
    geometryBuffer.bindMaterial(kMaterialUnit);
    geometryBuffer.bindDepth(kDepthUnit);
    ambientOcclusionBuffer.bindTexture(kAmbientOcclusionUnit);

    shader.setInt("uPosition", 0);
    shader.setInt("uNormal", 1);
    shader.setInt("uMaterial", 2);
    shader.setInt("uDepth", 3);
    shader.setInt("uAmbientOcclusion", 4);
    shader.setInt("uShadowMap", 5);
}

void bindLight(const ShaderProgram& shader,
    const PointLight& light,
    const ShadowMap& shadowMap,
    size_t lightCount,
    bool firstLightingPass)
{
    shadowMap.bind(kShadowUnit);
    shader.setMat4("uLightViewProjection", shadowMap.lightViewProjection());
    shader.setVec3("uLightPosition", light.position);
    shader.setVec3("uLightColor", light.color);
    shader.setFloat("uInvLightCount", 1.0f / static_cast<float>(lightCount));
    shader.setBool("uFirstLightingPass", firstLightingPass);
}

void configureLightAccumulation(size_t lightIndex)
{
    if (lightIndex == 0) {
        glDisable(GL_BLEND);
        return;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
}

void renderLightingPass(const AppConfig& config,
    const Scene& scene,
    const std::vector<PointLight>& lights,
    const std::vector<ShadowMap>& shadowMaps,
    const GeometryBuffer& geometryBuffer,
    const AmbientOcclusionBuffer& ambientOcclusionBuffer,
    const LightingBuffer& lightingBuffer,
    const FullscreenTriangle& fullscreenTriangle,
    const ShaderProgram& shader)
{
    lightingBuffer.bind();
    glViewport(0, 0, config.width, config.height);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glClearColor(0.02f, 0.025f, 0.03f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    shader.use();
    bindLightingInputs(shader, geometryBuffer, ambientOcclusionBuffer);
    shader.setVec3("uCameraPosition", scene.camera.position);
    shader.setVec3("uBackgroundColor", Vec3(0.02f, 0.025f, 0.03f));
    shader.setFloat("uAmbientStrength", config.ambientStrength);
    shader.setFloat("uLightIntensity", config.lightIntensity);
    shader.setFloat("uShininess", 32.0f);
    shader.setFloat("uSpecularStrength", 0.0f);

    for (size_t lightIndex = 0; lightIndex < lights.size(); ++lightIndex) {
        configureLightAccumulation(lightIndex);
        bindLight(shader,
            lights[lightIndex],
            shadowMaps[lightIndex],
            lights.size(),
            lightIndex == 0);
        fullscreenTriangle.draw();
    }

    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    checkGl("lighting pass");
}

} // namespace

void Renderer::render(const AppConfig& config)
{
    OpenGlContext context;

    {
        const Scene scene = loadScene(config.modelDir);
        std::vector<GpuMesh> meshes = uploadSceneMeshes(scene);
        std::vector<PointLight> lights = sampleAreaLight(
            scene.areaLight, config.areaLightSamplesPerSide);

        ShaderProgram geometryShader(
            ShaderSources::geometryVertex, ShaderSources::geometryFragment);
        ShaderProgram lightingShader(
            ShaderSources::fullscreenVertex, ShaderSources::lightingFragment);
        ShaderProgram shadowShader(
            ShaderSources::shadowVertex, ShaderSources::shadowFragment);

        std::vector<ShadowMap> shadowMaps = renderShadowMaps(
            config, meshes, lights, shadowShader, scene.shadow);

        GeometryBuffer geometryBuffer(config.width, config.height);
        AmbientOcclusionBuffer ambientOcclusionBuffer(config.width, config.height);
        LightingBuffer lightingBuffer(config.width, config.height);
        FullscreenTriangle fullscreenTriangle;

        renderGeometryPass(config, scene, meshes, geometryShader, geometryBuffer);

        // SSAO will replace this neutral texture with computed visibility values.
        ambientOcclusionBuffer.clearNeutral();

        renderLightingPass(config,
            scene,
            lights,
            shadowMaps,
            geometryBuffer,
            ambientOcclusionBuffer,
            lightingBuffer,
            fullscreenTriangle,
            lightingShader);

        lightingBuffer.writeColor(config.colorOutput);
        geometryBuffer.writeNormalDebug(config.normalOutput);
        geometryBuffer.writeDepthDebug(config.depthOutput);
        ambientOcclusionBuffer.writeDebug(config.ambientOcclusionOutput);
    }

    std::cout << "Wrote " << config.colorOutput << "\n";
    std::cout << "Wrote " << config.normalOutput << "\n";
    std::cout << "Wrote " << config.depthOutput << "\n";
    std::cout << "Wrote " << config.ambientOcclusionOutput << "\n";
}
