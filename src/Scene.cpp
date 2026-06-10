#include "Scene.hpp"

#include "AreaLight.hpp"
#include "ObjLoader.hpp"

#include <nlohmann/json.hpp>

#include <cmath>
#include <fstream>
#include <stdexcept>
#include <string>

namespace {

using Json = nlohmann::json;

Vec3 readVec3(const Json& object, const char* field)
{
    const Json& value = object.at(field);
    if (!value.is_array() || value.size() != 3) {
        throw std::runtime_error(std::string(field) + " must be an array of three numbers");
    }

    Vec3 result(value.at(0).get<float>(), value.at(1).get<float>(), value.at(2).get<float>());
    if (!std::isfinite(result.x) || !std::isfinite(result.y) || !std::isfinite(result.z)) {
        throw std::runtime_error(std::string(field) + " must contain finite numbers");
    }
    return result;
}

float readPositiveFloat(const Json& object, const char* field)
{
    const float value = object.at(field).get<float>();
    if (!std::isfinite(value) || value <= 0.0f) {
        throw std::runtime_error(std::string(field) + " must be a positive finite number");
    }
    return value;
}

SceneCamera readCamera(const Json& root)
{
    const Json& camera = root.at("camera");
    SceneCamera result;
    result.position = readVec3(camera, "position");
    result.target = readVec3(camera, "target");
    result.up = readVec3(camera, "up");
    result.fovYDegrees = readPositiveFloat(camera, "fov_y_degrees");
    result.nearPlane = readPositiveFloat(camera, "near");
    result.farPlane = readPositiveFloat(camera, "far");
    if (result.fovYDegrees >= 180.0f) {
        throw std::runtime_error("camera.fov_y_degrees must be less than 180");
    }
    if (result.farPlane <= result.nearPlane) {
        throw std::runtime_error("camera.far must be greater than camera.near");
    }
    return result;
}

RectAreaLight readAreaLight(const Json& root)
{
    const Json& light = root.at("area_light");
    RectAreaLight result;
    result.origin = readVec3(light, "origin");
    result.edgeU = readVec3(light, "edge_u");
    result.edgeV = readVec3(light, "edge_v");
    result.color = readVec3(light, "color");
    if (length(result.edgeU) <= 1e-6f || length(result.edgeV) <= 1e-6f) {
        throw std::runtime_error("area_light edges must be non-zero");
    }
    if (length(cross(result.edgeU, result.edgeV)) <= 1e-6f) {
        throw std::runtime_error("area_light edges must not be parallel");
    }
    return result;
}

ShadowSettings readShadowSettings(const Json& root)
{
    const Json& shadow = root.at("shadow");
    ShadowSettings result;
    result.target = readVec3(shadow, "target");
    result.up = readVec3(shadow, "up");
    result.fovYDegrees = readPositiveFloat(shadow, "fov_y_degrees");
    result.nearPlane = readPositiveFloat(shadow, "near");
    result.farPlane = readPositiveFloat(shadow, "far");
    if (result.fovYDegrees >= 180.0f) {
        throw std::runtime_error("shadow.fov_y_degrees must be less than 180");
    }
    if (result.farPlane <= result.nearPlane) {
        throw std::runtime_error("shadow.far must be greater than shadow.near");
    }
    return result;
}

std::vector<SceneObject> readObjects(const Json& root, const std::filesystem::path& modelDir)
{
    const Json& objects = root.at("objects");
    if (!objects.is_array() || objects.empty()) {
        throw std::runtime_error("objects must be a non-empty array");
    }

    std::vector<SceneObject> result;
    result.reserve(objects.size());
    for (const Json& object : objects) {
        const std::filesystem::path mesh = object.at("mesh").get<std::string>();
        if (mesh.empty() || mesh.is_absolute()) {
            throw std::runtime_error("object mesh paths must be non-empty and relative");
        }

        result.push_back(SceneObject{
            modelDir / mesh,
            readVec3(object, "color"),
            object.contains("offset") ? readVec3(object, "offset") : Vec3(0.0f),
            object.value("emissive", false),
        });
    }
    return result;
}

std::vector<GpuMesh> uploadMeshes(const std::vector<SceneObject>& objects)
{
    std::vector<GpuMesh> meshes;
    meshes.reserve(objects.size());

    for (const SceneObject& object : objects) {
        std::vector<Vertex> vertices = loadObjMesh(
            object.objPath, object.color, object.positionOffset);
        meshes.emplace_back(
            object.objPath.filename().string(), vertices, object.emissive);
    }
    return meshes;
}

} // namespace

Scene::Scene(const AppConfig& config)
{
    const std::filesystem::path& modelDir = config.modelDir;
    const std::filesystem::path scenePath = modelDir / "scene.json";
    std::ifstream input(scenePath);
    if (!input) {
        throw std::runtime_error("failed to open scene file: " + scenePath.string());
    }

    try {
        const Json root = Json::parse(input);
        objects = readObjects(root, modelDir);
        camera = readCamera(root);
        areaLight = readAreaLight(root);
        shadow = readShadowSettings(root);
        meshes = uploadMeshes(objects);

        areaLightSamplesPerSide = root.value("area_light_samples", 8);
        shadowMapSize = root.value("shadow_map_size", 512);
        ambientStrength = root.value("ambient_strength", 0.2f);
        lightIntensity = root.value("light_intensity", 1.0f);

        lights = sampleAreaLight(areaLight, areaLightSamplesPerSide);
    } catch (const std::exception& error) {
        throw std::runtime_error("failed to parse " + scenePath.string() + ": " + error.what());
    }
}
