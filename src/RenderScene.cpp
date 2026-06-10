#include "RenderScene.hpp"

#include "ObjLoader.hpp"

namespace {

std::vector<GpuMesh> uploadMeshes(const Scene& scene)
{
    std::vector<GpuMesh> meshes;
    meshes.reserve(scene.objects.size());

    for (const SceneObject& object : scene.objects) {
        std::vector<Vertex> vertices = loadObjMesh(
            object.objPath, object.color, object.positionOffset);
        meshes.emplace_back(
            object.objPath.filename().string(), vertices, object.emissive);
    }
    return meshes;
}

} // namespace

RenderScene::RenderScene(const AppConfig& config)
    : description_(loadScene(config.modelDir))
    , meshes_(uploadMeshes(description_))
    , lights_(sampleAreaLight(
          description_.areaLight, config.areaLightSamplesPerSide))
{
}
