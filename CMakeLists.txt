cmake_minimum_required(VERSION 3.16)
project(SSAO_Renderer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew" "/usr/local")

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_executable(SSAO_Renderer
    src/SSAOBuffer.cpp
    src/SSAOPass.cpp
    src/AreaLight.cpp
    src/AppConfig.cpp
    src/FullscreenTriangle.cpp
    src/GeometryBuffer.cpp
    src/GeometryPass.cpp
    src/GpuMesh.cpp
    src/LightingBuffer.cpp
    src/LightingPass.cpp
    src/ObjLoader.cpp
    src/OpenGLHelpers.cpp
    src/RenderOutputWriter.cpp
    src/Renderer.cpp
    src/Scene.cpp
    src/ShadowPass.cpp
    src/ShadowMap.cpp
    src/ShaderProgram.cpp
    src/main.cpp
)

target_compile_definitions(SSAO_Renderer PRIVATE GL_SILENCE_DEPRECATION)
target_include_directories(SSAO_Renderer PRIVATE include)

target_link_libraries(SSAO_Renderer PRIVATE
    GLEW::GLEW
    nlohmann_json::nlohmann_json
    OpenGL::GL
)
