#include "Renderer.hpp"

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

void Renderer::render(const AppConfig& config)
{
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
