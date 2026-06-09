cmake_minimum_required(VERSION 3.16)
project(SSAO_Renderer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew" "/usr/local")

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

add_executable(SSAO_Renderer
    src/AreaLight.cpp
    src/AppConfig.cpp
    src/CornellBoxScene.cpp
    src/Framebuffer.cpp
    src/GpuMesh.cpp
    src/ObjLoader.cpp
    src/Renderer.cpp
    src/ShadowMap.cpp
    src/ShaderProgram.cpp
    src/main.cpp
)

target_compile_definitions(SSAO_Renderer PRIVATE GL_SILENCE_DEPRECATION)
target_include_directories(SSAO_Renderer PRIVATE include)

target_link_libraries(SSAO_Renderer PRIVATE
    GLEW::GLEW
    OpenGL::GL
)
