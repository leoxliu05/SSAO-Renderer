#pragma once

namespace GeometryShaders {

inline constexpr const char* vertex = R"GLSL(
#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aColor;

uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vWorldPosition;
out vec3 vWorldNormal;
out vec3 vAlbedo;

void main() {
    vWorldPosition = aPosition;
    vWorldNormal = normalize(aNormal);
    vAlbedo = aColor;
    gl_Position = uProjection * uView * vec4(aPosition, 1.0);
}
)GLSL";

inline constexpr const char* fragment = R"GLSL(
#version 330 core

layout(location = 0) out vec3 oWorldPosition;
layout(location = 1) out vec3 oWorldNormal;
layout(location = 2) out vec4 oMaterial;

in vec3 vWorldPosition;
in vec3 vWorldNormal;
in vec3 vAlbedo;

uniform bool uEmissive;

void main() {
    oWorldPosition = vWorldPosition;
    oWorldNormal = normalize(vWorldNormal);
    oMaterial = vec4(vAlbedo, uEmissive ? 1.0 : 0.0);
}
)GLSL";

} // namespace GeometryShaders
