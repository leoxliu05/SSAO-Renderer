#include "GeometryPass.hpp"

#include "GeometryShaders.hpp"
#include "Math.hpp"
#include "OpenGLHelpers.hpp"

#include <GL/glew.h>

namespace {

Mat4 makeView(const SceneCamera& camera)
{
    return lookAt(camera.position, camera.target, camera.up);
}

Mat4 makeProjection(const SceneCamera& camera, const AppConfig& config)
{
    return perspective(
        radians(camera.fovYDegrees),
        static_cast<float>(config.width) / static_cast<float>(config.height),
        camera.nearPlane,
        camera.farPlane);
}

} // namespace

GeometryPass::GeometryPass()
    : shader_(GeometryShaders::vertex, GeometryShaders::fragment)
{
}

void GeometryPass::render(const AppConfig& config,
    const Scene& scene,
    const GeometryBuffer& output) const
{
    output.bind();
    glViewport(0, 0, config.width, config.height);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shader_.use();
    shader_.setMat4("uView", makeView(scene.camera));
    shader_.setMat4("uProjection", makeProjection(scene.camera, config));

    for (const GpuMesh& mesh : scene.meshes) {
        shader_.setBool("uEmissive", mesh.emissive());
        mesh.draw();
    }

    glBindVertexArray(0);
    OpenGLHelpers::checkError("geometry pass");
}
