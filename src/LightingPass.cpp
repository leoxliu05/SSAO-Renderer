#include "LightingPass.hpp"

#include "LightingShaders.hpp"
#include "OpenGLHelpers.hpp"

#include <GL/glew.h>

namespace {

constexpr GLenum kPositionUnit = GL_TEXTURE0;
constexpr GLenum kNormalUnit = GL_TEXTURE1;
constexpr GLenum kMaterialUnit = GL_TEXTURE2;
constexpr GLenum kDepthUnit = GL_TEXTURE3;
constexpr GLenum kAOUnit = GL_TEXTURE4;
constexpr GLenum kShadowUnit = GL_TEXTURE5;

void bindSurfaceInputs(const ShaderProgram& shader,
    const GeometryBuffer& geometryBuffer,
    const AOBuffer& aoBuffer)
{
    geometryBuffer.bindPosition(kPositionUnit);
    geometryBuffer.bindNormal(kNormalUnit);
    geometryBuffer.bindMaterial(kMaterialUnit);
    geometryBuffer.bindDepth(kDepthUnit);
    aoBuffer.bindTexture(kAOUnit);

    shader.setInt("uPosition", 0);
    shader.setInt("uNormal", 1);
    shader.setInt("uMaterial", 2);
    shader.setInt("uDepth", 3);
    shader.setInt("uAO", 4);
    shader.setInt("uShadowMap", 5);
}

void bindLight(const ShaderProgram& shader,
    const PointLight& light,
    const ShadowMap& shadowMap,
    size_t lightCount,
    bool firstLight)
{
    shadowMap.bind(kShadowUnit);
    shader.setMat4("uLightViewProjection", shadowMap.lightViewProjection());
    shader.setVec3("uLightPosition", light.position);
    shader.setVec3("uLightColor", light.color);
    shader.setFloat("uInvLightCount", 1.0f / static_cast<float>(lightCount));
    shader.setBool("uFirstLightingPass", firstLight);
}

void configureAccumulation(bool firstLight)
{
    if (firstLight) {
        glDisable(GL_BLEND);
        return;
    }

    glEnable(GL_BLEND);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ZERO, GL_ONE);
}

} // namespace

LightingPass::LightingPass()
    : shader_(LightingShaders::vertex, LightingShaders::fragment)
{
}

void LightingPass::render(const AppConfig& config,
    const Scene& scene,
    const std::vector<ShadowMap>& shadowMaps,
    const GeometryBuffer& geometryBuffer,
    const AOBuffer& aoBuffer,
    const LightingBuffer& output) const
{
    output.bind();
    glViewport(0, 0, config.width, config.height);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glClearColor(0.02f, 0.025f, 0.03f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    shader_.use();
    bindSurfaceInputs(shader_, geometryBuffer, aoBuffer);
    shader_.setVec3("uCameraPosition", scene.camera.position);
    shader_.setVec3("uBackgroundColor", Vec3(0.02f, 0.025f, 0.03f));
    shader_.setFloat("uAmbientStrength", config.ambientStrength);
    shader_.setFloat("uLightIntensity", config.lightIntensity);
    shader_.setFloat("uShininess", 32.0f);
    shader_.setFloat("uSpecularStrength", 0.0f);

    for (size_t lightIndex = 0; lightIndex < scene.lights.size(); ++lightIndex) {
        const bool firstLight = lightIndex == 0;
        configureAccumulation(firstLight);
        bindLight(shader_,
            scene.lights[lightIndex],
            shadowMaps[lightIndex],
            scene.lights.size(),
            firstLight);
        fullscreenTriangle_.draw();
    }

    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    OpenGLHelpers::checkError("lighting pass");
}
