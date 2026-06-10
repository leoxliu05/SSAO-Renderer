#include "ShadowPass.hpp"

#include "ShadowShaders.hpp"

ShadowPass::ShadowPass(int shadowMapSize)
    : shadowMapSize_(shadowMapSize)
    , shader_(ShadowShaders::vertex, ShadowShaders::fragment)
{
}

std::vector<ShadowMap> ShadowPass::render(const RenderScene& scene) const
{
    std::vector<ShadowMap> shadowMaps;
    shadowMaps.reserve(scene.lights().size());

    for (const PointLight& light : scene.lights()) {
        shadowMaps.emplace_back(shadowMapSize_);
        shadowMaps.back().render(
            scene.meshes(), shader_, light.position, scene.description().shadow);
    }
    return shadowMaps;
}
