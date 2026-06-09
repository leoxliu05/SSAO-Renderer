#pragma once

#include "ssao/Vertex.hpp"

#include <filesystem>
#include <vector>

std::vector<Vertex> loadObjMesh(const std::filesystem::path& path, const Vec3& color);
