#include "Renderer.hpp"

#include <filesystem>

#include "SSAOBuffer.hpp"
#include "SSAOPass.hpp"
#include "GeometryBuffer.hpp"
#include "GeometryPass.hpp"
#include "LightingBuffer.hpp"
#include "LightingPass.hpp"
#include "Math.hpp"
#include "OpenGLHelpers.hpp"
#include "RenderOutputWriter.hpp"
#include "Scene.hpp"
#include "ShadowPass.hpp"

void Renderer::render(AppConfig config)
{
    // Place outputs under build/<model_name>/ so each scene stays isolated.
    auto outDir = std::filesystem::path("build") / config.modelDir.filename();
    std::filesystem::create_directories(outDir);
    config.colorOutput  = outDir / config.colorOutput.filename();
    config.normalOutput = outDir / config.normalOutput.filename();
    config.depthOutput  = outDir / config.depthOutput.filename();
    config.ssaoOutput   = outDir / config.ssaoOutput.filename();

    OpenGLHelpers::Context context;
    Scene scene(config);

    // Scene-provided settings override AppConfig defaults.
    config.areaLightSamplesPerSide = scene.areaLightSamplesPerSide;
    config.shadowMapSize = scene.shadowMapSize;
    config.ka = scene.ka;
    config.kd = scene.kd;
    config.ks = scene.ks;
    config.lightIntensity = scene.lightIntensity;

    GeometryBuffer geometryBuffer(config.width, config.height);
    SSAOBuffer ssaoBuffer(config.width, config.height);
    LightingBuffer lightingBuffer(config.width, config.height);

    ShadowPass shadowPass(config.shadowMapSize);
    GeometryPass geometryPass;
    SSAOPass ssaoPass;
    LightingPass lightingPass;
    RenderOutputWriter outputWriter;

    std::vector<ShadowMap> shadowMaps = shadowPass.render(scene);
    geometryPass.render(config, scene, geometryBuffer);

    Mat4 view = lookAt(scene.camera.position, scene.camera.target, scene.camera.up);
    Mat4 proj = perspective(radians(scene.camera.fovYDegrees),
        static_cast<float>(config.width) / static_cast<float>(config.height),
        scene.camera.nearPlane, scene.camera.farPlane);

    if (config.enableSSAO) {
        ssaoPass.render(ssaoBuffer, geometryBuffer, view, proj,
                        config.width, config.height);
    } else {
        ssaoBuffer.clearNeutral();
    }
    lightingPass.render(config,
        scene,
        shadowMaps,
        geometryBuffer,
        ssaoBuffer,
        lightingBuffer);

    outputWriter.write(
        config, geometryBuffer, ssaoBuffer, lightingBuffer);
}
