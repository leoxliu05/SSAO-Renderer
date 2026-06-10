#include "Renderer.hpp"

#include "AmbientOcclusionBuffer.hpp"
#include "AmbientOcclusionPass.hpp"
#include "GeometryBuffer.hpp"
#include "GeometryPass.hpp"
#include "LightingBuffer.hpp"
#include "LightingPass.hpp"
#include "OpenGlContext.hpp"
#include "RenderOutputWriter.hpp"
#include "RenderScene.hpp"
#include "ShadowPass.hpp"

void Renderer::render(const AppConfig& config)
{
    OpenGlContext context;
    RenderScene scene(config);

    GeometryBuffer geometryBuffer(config.width, config.height);
    AmbientOcclusionBuffer ambientOcclusionBuffer(config.width, config.height);
    LightingBuffer lightingBuffer(config.width, config.height);

    ShadowPass shadowPass(config.shadowMapSize);
    GeometryPass geometryPass;
    AmbientOcclusionPass ambientOcclusionPass;
    LightingPass lightingPass;
    RenderOutputWriter outputWriter;

    std::vector<ShadowMap> shadowMaps = shadowPass.render(scene);
    geometryPass.render(config, scene, geometryBuffer);
    ambientOcclusionPass.render(ambientOcclusionBuffer);
    lightingPass.render(config,
        scene,
        shadowMaps,
        geometryBuffer,
        ambientOcclusionBuffer,
        lightingBuffer);

    outputWriter.write(
        config, geometryBuffer, ambientOcclusionBuffer, lightingBuffer);
}
