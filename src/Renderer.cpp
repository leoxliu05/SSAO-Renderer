#include "Renderer.hpp"

#include "AreaLight.hpp"
#include "CornellBoxScene.hpp"
#include "Framebuffer.hpp"
#include "GpuMesh.hpp"
#include "Math.hpp"
#include "ObjLoader.hpp"
#include "ShadowMap.hpp"
#include "ShaderProgram.hpp"

#include <GL/glew.h>
#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#endif

#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

const char* kVertexShader = R"GLSL(
#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aColor;

uniform mat4 uView;
uniform mat4 uProjection;
uniform mat4 uLightViewProjection;

out vec3 vWorldPosition;
out vec3 vNormal;
out vec3 vKd;
out vec4 vLightSpacePosition;

void main() {
    vWorldPosition = aPosition;
    vNormal = normalize(aNormal);
    vKd = aColor;
    vLightSpacePosition = uLightViewProjection * vec4(aPosition, 1.0);
    gl_Position = uProjection * uView * vec4(aPosition, 1.0);
}
)GLSL";

const char* kFragmentShader = R"GLSL(
#version 330 core

layout(location = 0) out vec4 oColor;
layout(location = 1) out vec4 oNormal;

in vec3 vWorldPosition;
in vec3 vNormal;
in vec3 vKd;
in vec4 vLightSpacePosition;

uniform vec3 uLightPosition;
uniform vec3 uLightColor;
uniform vec3 uCameraPosition;
uniform sampler2D uShadowMap;
uniform float uInvLightCount;
uniform float uAmbientStrength;
uniform float uLightIntensity;
uniform float uShadowMinLight;
uniform float uShininess;
uniform float uSpecularStrength;
uniform bool uEmissive;
uniform bool uFirstLightingPass;

float pointShadow(vec3 normal) {
    vec3 projected = vLightSpacePosition.xyz / vLightSpacePosition.w;
    projected = projected * 0.5 + 0.5;
    if (projected.z >= 1.0 || projected.x <= 0.0 || projected.x >= 1.0 ||
        projected.y <= 0.0 || projected.y >= 1.0) {
        return 1.0;
    }

    vec3 lightDir = normalize(uLightPosition - vWorldPosition);
    float bias = max(0.0025 * (1.0 - abs(dot(normal, lightDir))), 0.0005);
    vec2 texelSize = 1.0 / vec2(textureSize(uShadowMap, 0));

    float visibility = 0.0;
    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            float closestDepth = texture(
                uShadowMap, projected.xy + vec2(x, y) * texelSize).r;
            visibility += projected.z - bias > closestDepth ? 0.0 : 1.0;
        }
    }
    return visibility / 9.0;
}

void main() {
    vec3 n = normalize(vNormal);
    vec3 lit = vec3(0.0);

    if (uEmissive) {
        lit = uFirstLightingPass ? vec3(1.0, 0.94, 0.78) : vec3(0.0);
    } else {
        vec3 ka = vKd;
        vec3 kd = vKd;
        vec3 ks = vec3(uSpecularStrength);

        vec3 l = normalize(uLightPosition - vWorldPosition);
        vec3 v = normalize(uCameraPosition - vWorldPosition);
        vec3 h = normalize(l + v);

        float diffuseFactor = max(abs(dot(n, l)), 0.0);
        float specularFactor = pow(max(abs(dot(n, h)), 0.0), uShininess);
        float shadow = mix(uShadowMinLight, 1.0, pointShadow(n));

        vec3 ambient = uFirstLightingPass ? ka * uAmbientStrength : vec3(0.0);
        vec3 diffuse = kd * uLightColor * diffuseFactor;
        vec3 specular = ks * uLightColor * specularFactor;
        lit = ambient + (diffuse + specular) * shadow * uLightIntensity * uInvLightCount;
    }

    oColor = vec4(lit, 1.0);
    oNormal = vec4(n * 0.5 + 0.5, 1.0);
}
)GLSL";

const char* kShadowVertexShader = R"GLSL(
#version 330 core

layout(location = 0) in vec3 aPosition;

uniform mat4 uLightViewProjection;

void main() {
    gl_Position = uLightViewProjection * vec4(aPosition, 1.0);
}
)GLSL";

const char* kShadowFragmentShader = R"GLSL(
#version 330 core

void main() {
}
)GLSL";

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
        throw std::runtime_error("headless OpenGL context setup is implemented for macOS in this scaffold");
#endif

        glewExperimental = GL_TRUE;
        GLenum glewError = glewInit();
        glGetError();
        if (glewError != GLEW_OK) {
            throw std::runtime_error(reinterpret_cast<const char*>(glewGetErrorString(glewError)));
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
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::ostringstream oss;
        oss << label << " failed with GL error 0x" << std::hex << err;
        throw std::runtime_error(oss.str());
    }
}

std::vector<GpuMesh> uploadSceneMeshes(const AppConfig& config)
{
    std::vector<GpuMesh> meshes;
    for (const SceneObject& object : loadCornellBoxScene(config.modelDir)) {
        std::vector<Vertex> vertices = loadObjMesh(object.objPath, object.color, object.positionOffset);
        meshes.emplace_back(object.objPath.filename().string(), vertices, object.emissive);
    }
    return meshes;
}

std::vector<ShadowMap> renderShadowMaps(const AppConfig& config,
    const std::vector<GpuMesh>& meshes,
    const std::vector<PointLight>& lights,
    const ShaderProgram& shadowShader,
    float farPlane)
{
    std::vector<ShadowMap> shadowMaps;
    shadowMaps.reserve(lights.size());

    for (const PointLight& light : lights) {
        shadowMaps.emplace_back(config.shadowMapSize);
        shadowMaps.back().render(meshes, shadowShader, light.position, farPlane);
    }
    return shadowMaps;
}

void bindLightUniforms(const ShaderProgram& shader,
    const PointLight& light,
    const ShadowMap& shadowMap,
    size_t lightCount,
    bool firstLightingPass)
{
    shadowMap.bind(GL_TEXTURE0);
    shader.setInt("uShadowMap", 0);
    shader.setMat4("uLightViewProjection", shadowMap.lightViewProjection());
    shader.setVec3("uLightPosition", light.position);
    shader.setVec3("uLightColor", light.color);
    shader.setFloat("uInvLightCount", 1.0f / static_cast<float>(lightCount));
    shader.setBool("uFirstLightingPass", firstLightingPass);
}

Vec3 cornellCameraPosition()
{
    return Vec3(278.0f, 273.0f, -800.0f);
}

void configureLightingPass(size_t lightIndex)
{
    if (lightIndex == 0) {
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
        glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glColorMaski(1, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        return;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glColorMaski(1, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
}

Mat4 cornellView()
{
    return lookAt(
        cornellCameraPosition(),
        Vec3(278.0f, 273.0f, 279.6f),
        Vec3(0.0f, 1.0f, 0.0f));
}

Mat4 cornellProjection(const AppConfig& config)
{
    return perspective(
        radians(39.3077f),
        static_cast<float>(config.width) / static_cast<float>(config.height),
        0.1f,
        2500.0f);
}

} // namespace

void Renderer::render(const AppConfig& config)
{
    OpenGlContext context;

    {
        ShaderProgram shader(kVertexShader, kFragmentShader);
        ShaderProgram shadowShader(kShadowVertexShader, kShadowFragmentShader);
        std::vector<GpuMesh> meshes = uploadSceneMeshes(config);
        std::vector<PointLight> lights = sampleCornellAreaLight(config.areaLightSamplesPerSide);

        constexpr float shadowFarPlane = 1200.0f;
        std::vector<ShadowMap> shadowMaps = renderShadowMaps(config, meshes, lights, shadowShader, shadowFarPlane);

        Framebuffer framebuffer(config.width, config.height);

        framebuffer.bind();
        glViewport(0, 0, config.width, config.height);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glClearColor(0.02f, 0.025f, 0.03f, 1.0f);

        shader.use();
        shader.setMat4("uView", cornellView());
        shader.setMat4("uProjection", cornellProjection(config));
        shader.setVec3("uCameraPosition", cornellCameraPosition());
        shader.setFloat("uAmbientStrength", config.ambientStrength);
        shader.setFloat("uLightIntensity", config.lightIntensity);
        shader.setFloat("uShadowMinLight", config.shadowMinLight);
        shader.setFloat("uShininess", 32.0f);
        shader.setFloat("uSpecularStrength", 0.0f);

        for (size_t lightIndex = 0; lightIndex < lights.size(); ++lightIndex) {
            configureLightingPass(lightIndex);
            bindLightUniforms(shader, lights[lightIndex], shadowMaps[lightIndex], lights.size(), lightIndex == 0);

            for (const GpuMesh& mesh : meshes) {
                shader.setBool("uEmissive", mesh.emissive());
                mesh.draw();
            }
        }
        glBindVertexArray(0);
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
        glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glColorMaski(1, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        checkGl("render");

        framebuffer.writeColor(config.colorOutput);
        framebuffer.writeNormalDebug(config.normalOutput);
        framebuffer.writeDepthDebug(config.depthOutput);
    }

    std::cout << "Wrote " << config.colorOutput << "\n";
    std::cout << "Wrote " << config.normalOutput << "\n";
    std::cout << "Wrote " << config.depthOutput << "\n";
}
