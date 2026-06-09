#include "ssao/CornellBoxScene.hpp"

std::vector<SceneObject> loadCornellBoxScene(const std::filesystem::path& modelDir)
{
    return {
        {modelDir / "floor.obj", Vec3(0.725f, 0.710f, 0.680f), Vec3(0.0f), false},
        {modelDir / "shortbox.obj", Vec3(0.725f, 0.710f, 0.680f), Vec3(0.0f), false},
        {modelDir / "tallbox.obj", Vec3(0.725f, 0.710f, 0.680f), Vec3(0.0f), false},
        {modelDir / "left.obj", Vec3(0.630f, 0.065f, 0.050f), Vec3(0.0f), false},
        {modelDir / "right.obj", Vec3(0.140f, 0.450f, 0.091f), Vec3(0.0f), false},
        {modelDir / "light.obj", Vec3(1.000f, 0.940f, 0.780f), Vec3(0.0f, -3.7f, 0.0f), true},
    };
}
