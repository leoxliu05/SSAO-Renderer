#pragma once

namespace SSAOShaders {

inline constexpr const char* vertex = R"GLSL(
#version 330 core

out vec2 vUv;

void main() {
    const vec2 positions[3] = vec2[3](
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)
    );
    vec2 pos = positions[gl_VertexID];
    vUv = pos * 0.5 + 0.5;
    gl_Position = vec4(pos, 0.0, 1.0);
}
)GLSL";

inline constexpr const char* fragment = R"GLSL(
#version 330 core

in vec2 vUv;

uniform sampler2D uPosition;
uniform sampler2D uNormal;
uniform sampler2D uDepth;
uniform sampler2D uNoise;

uniform mat4 uView;
uniform mat4 uProjection;
uniform vec3 uSamples[128];
uniform float uRadius;
uniform float uBias;
uniform float uNear;
uniform float uFar;
uniform float uScreenWidth;
uniform float uScreenHeight;

layout(location = 0) out float oSSAO;

float linearizeDepth(float depth) {
    float ndc = depth * 2.0 - 1.0;
    return 2.0 * uNear * uFar / (uFar + uNear - ndc * (uFar - uNear));
}

void main() {
    float depth = texture(uDepth, vUv).r;
    if (depth >= 1.0) {
        oSSAO = 1.0;
        return;
    }

    vec3 worldPos    = texture(uPosition, vUv).rgb;
    vec3 worldNormal = texture(uNormal,   vUv).rgb;
    if (length(worldNormal) < 0.01) {
        oSSAO = 1.0;
        return;
    }
    worldNormal = normalize(worldNormal);

    // Transform to view space.
    vec3 viewPos    = (uView * vec4(worldPos, 1.0)).xyz;
    vec3 viewNormal = normalize(mat3(uView) * worldNormal);

    // Random rotation from noise texture (tiled).
    vec2 noiseUV = vUv * vec2(uScreenWidth, uScreenHeight) / 4.0;
    vec3 randomVec = vec3(texture(uNoise, noiseUV).rg, 0.0);

    // Build TBN: tangent + bitangent + normal (view space).
    vec3 tangent   = normalize(randomVec - viewNormal * dot(randomVec, viewNormal));
    vec3 bitangent = cross(viewNormal, tangent);
    mat3 tbn = mat3(tangent, bitangent, viewNormal);

    float occlusion = 0.0;
    for (int i = 0; i < 128; ++i) {
        // Rotate sample from tangent space to view space.
        vec3 sampleView = viewPos + tbn * uSamples[i] * uRadius;

        // Project sample to screen.
        vec4 offset = uProjection * vec4(sampleView, 1.0);
        offset.xy = offset.xy / offset.w * 0.5 + 0.5;

        // Sample the depth buffer at that screen position.
        float sampleDepth = texture(uDepth, offset.xy).r;
        float sampleLinearZ = linearizeDepth(sampleDepth);

        // Range check: ignore samples too far from current surface.
        float viewLinearZ = linearizeDepth(depth);
        float rangeCheck = smoothstep(0.0, 1.0, uRadius / abs(viewLinearZ - sampleLinearZ));

        float sampleDist = -sampleView.z;
        if (sampleLinearZ < sampleDist - uBias) {
            occlusion += rangeCheck;
        }
    }

    occlusion = 1.0 - occlusion / 128.0;
    // Contrast boost: amplify the occlusion for more visible effect.
    occlusion = pow(occlusion, 3.0);
    oSSAO = occlusion;
}
)GLSL";

} // namespace SSAOShaders
