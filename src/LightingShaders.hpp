#pragma once

namespace LightingShaders {

inline constexpr const char* vertex = R"GLSL(
#version 330 core

out vec2 vUv;

void main() {
    const vec2 positions[3] = vec2[3](
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)
    );
    vec2 position = positions[gl_VertexID];
    vUv = position * 0.5 + 0.5;
    gl_Position = vec4(position, 0.0, 1.0);
}
)GLSL";

inline constexpr const char* fragment = R"GLSL(
#version 330 core

layout(location = 0) out vec4 oColor;

in vec2 vUv;

uniform sampler2D uPosition;
uniform sampler2D uNormal;
uniform sampler2D uMaterial;
uniform sampler2D uDepth;
uniform sampler2D uAO;
uniform sampler2D uShadowMap;

uniform mat4 uLightViewProjection;
uniform vec3 uLightPosition;
uniform vec3 uLightColor;
uniform vec3 uCameraPosition;
uniform vec3 uBackgroundColor;
uniform float uInvLightCount;
uniform float uAmbientStrength;
uniform float uLightIntensity;
uniform float uShininess;
uniform float uSpecularStrength;
uniform bool uFirstLightingPass;

float pointShadow(vec3 worldPosition, vec3 normal) {
    vec4 lightSpacePosition = uLightViewProjection * vec4(worldPosition, 1.0);
    vec3 projected = lightSpacePosition.xyz / lightSpacePosition.w;
    projected = projected * 0.5 + 0.5;
    if (projected.z >= 1.0 || projected.x <= 0.0 || projected.x >= 1.0 ||
        projected.y <= 0.0 || projected.y >= 1.0) {
        return 1.0;
    }

    vec3 lightDirection = normalize(uLightPosition - worldPosition);
    float bias = max(0.0025 * (1.0 - abs(dot(normal, lightDirection))), 0.0005);
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
    if (texture(uDepth, vUv).r >= 1.0) {
        oColor = vec4(uFirstLightingPass ? uBackgroundColor : vec3(0.0), 1.0);
        return;
    }

    vec3 worldPosition = texture(uPosition, vUv).rgb;
    vec3 normal = normalize(texture(uNormal, vUv).rgb);
    vec4 material = texture(uMaterial, vUv);
    vec3 albedo = material.rgb;
    bool emissive = material.a > 0.5;

    if (emissive) {
        oColor = vec4(uFirstLightingPass ? albedo : vec3(0.0), 1.0);
        return;
    }

    vec3 lightDirection = normalize(uLightPosition - worldPosition);
    vec3 viewDirection = normalize(uCameraPosition - worldPosition);
    vec3 halfwayDirection = normalize(lightDirection + viewDirection);
    float diffuseFactor = max(dot(normal, lightDirection), 0.0);
    float specularFactor = pow(
        max(dot(normal, halfwayDirection), 0.0), uShininess);
    float shadow = pointShadow(worldPosition, normal);

    float ao = texture(uAO, vUv).r;
    vec3 ambient = uFirstLightingPass
        ? albedo * uAmbientStrength * ao
        : vec3(0.0);
    vec3 diffuse = albedo * uLightColor * diffuseFactor;
    vec3 specular = vec3(uSpecularStrength) * uLightColor * specularFactor;
    vec3 direct = (diffuse + specular) * shadow * uLightIntensity * uInvLightCount;

    oColor = vec4(ambient + direct, 1.0);
}
)GLSL";

} // namespace LightingShaders
