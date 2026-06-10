#pragma once

namespace ShadowShaders {

inline constexpr const char* vertex = R"GLSL(
#version 330 core

layout(location = 0) in vec3 aPosition;

uniform mat4 uLightViewProjection;

void main() {
    gl_Position = uLightViewProjection * vec4(aPosition, 1.0);
}
)GLSL";

inline constexpr const char* fragment = R"GLSL(
#version 330 core

void main() {
}
)GLSL";

} // namespace ShadowShaders
