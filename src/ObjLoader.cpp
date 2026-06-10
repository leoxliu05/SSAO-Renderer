#include "ObjLoader.hpp"

#include <array>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

int parseObjIndex(const std::string& token)
{
    size_t end = token.find('/');
    std::string indexText = token.substr(0, end);
    if (indexText.empty()) {
        throw std::runtime_error("unsupported OBJ face token: " + token);
    }
    return std::stoi(indexText);
}

} // namespace

std::vector<Vertex> loadObjMesh(const std::filesystem::path& path, const Vec3& color, const Vec3& positionOffset)
{
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("failed to open OBJ: " + path.string());
    }

    std::vector<Vec3> positions;
    std::vector<std::array<int, 3>> triangles;
    std::string line;

    // ── Pass 1: read positions and collect face index triplets ──
    while (std::getline(in, line)) {
        std::istringstream iss(line);
        std::string tag;
        iss >> tag;

        if (tag == "v") {
            Vec3 p{};
            iss >> p.x >> p.y >> p.z;
            p = p + positionOffset;
            positions.push_back(p);
        } else if (tag == "f") {
            std::vector<int> indices;
            std::string token;
            while (iss >> token) {
                int index = parseObjIndex(token);
                if (index < 0) {
                    index = static_cast<int>(positions.size()) + index + 1;
                }
                if (index <= 0 || index > static_cast<int>(positions.size())) {
                    throw std::runtime_error("OBJ face index out of range in " + path.string());
                }
                indices.push_back(index - 1);
            }
            // Fan triangulation (works for quads and n-gons)
            for (size_t i = 1; i + 1 < indices.size(); ++i) {
                triangles.push_back({indices[0], indices[i], indices[i + 1]});
            }
        }
    }

    if (triangles.empty()) {
        throw std::runtime_error("OBJ contained no triangles: " + path.string());
    }

    // ── Pass 2: accumulate face normals per vertex index ──
    //  cross(p1-p0, p2-p0) is intentionally NOT normalized —
    //  its magnitude = 2 × triangle area, so accumulation gives
    //  area-weighted vertex normals after the final normalize.
    //  Vertices shared across faces → smooth; unique → flat.
    std::vector<Vec3> smoothNormals(positions.size(), Vec3(0.0f));

    for (const auto& tri : triangles) {
        const Vec3& p0 = positions[tri[0]];
        const Vec3& p1 = positions[tri[1]];
        const Vec3& p2 = positions[tri[2]];
        Vec3 faceNormal = cross(p1 - p0, p2 - p0);
        if (std::isfinite(faceNormal.x) && std::isfinite(faceNormal.y) && std::isfinite(faceNormal.z)) {
            smoothNormals[tri[0]] = smoothNormals[tri[0]] + faceNormal;
            smoothNormals[tri[1]] = smoothNormals[tri[1]] + faceNormal;
            smoothNormals[tri[2]] = smoothNormals[tri[2]] + faceNormal;
        }
    }

    for (auto& n : smoothNormals) {
        float len = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
        if (len > 1e-10f) {
            n.x /= len; n.y /= len; n.z /= len;
        } else {
            n = Vec3(0.0f, 1.0f, 0.0f);
        }
    }

    // ── Pass 3: output vertices with smooth normals + uniform color ──
    std::vector<Vertex> vertices;
    vertices.reserve(triangles.size() * 3);
    for (const auto& tri : triangles) {
        vertices.push_back(Vertex{positions[tri[0]], smoothNormals[tri[0]], color});
        vertices.push_back(Vertex{positions[tri[1]], smoothNormals[tri[1]], color});
        vertices.push_back(Vertex{positions[tri[2]], smoothNormals[tri[2]], color});
    }

    return vertices;
}
