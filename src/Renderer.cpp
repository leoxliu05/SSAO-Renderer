#include "Renderer.hpp"

#include <filesystem>

#include "AOBuffer.hpp"
#include "AOPass.hpp"
#include "GeometryBuffer.hpp"
#include "GeometryPass.hpp"
#include "LightingBuffer.hpp"
#include "LightingPass.hpp"
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
    config.aoOutput     = outDir / config.aoOutput.filename();

    OpenGLHelpers::Context context;
    Scene scene(config);

    GeometryBuffer geometryBuffer(config.width, config.height);
    AOBuffer aoBuffer(config.width, config.height);
    LightingBuffer lightingBuffer(config.width, config.height);

    ShadowPass shadowPass(config.shadowMapSize);
    GeometryPass geometryPass;
    AOPass aoPass;
    LightingPass lightingPass;
    RenderOutputWriter outputWriter;

    std::vector<ShadowMap> shadowMaps = shadowPass.render(scene);
    geometryPass.render(config, scene, geometryBuffer);
    aoPass.render(aoBuffer);
    lightingPass.render(config,
        scene,
        shadowMaps,
        geometryBuffer,
        aoBuffer,
        lightingBuffer);

    outputWriter.write(
        config, geometryBuffer, aoBuffer, lightingBuffer);
}
