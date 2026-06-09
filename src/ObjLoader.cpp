#include "ssao/ObjLoader.hpp"

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
    std::vector<Vertex> vertices;
    std::string line;

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

            for (size_t i = 1; i + 1 < indices.size(); ++i) {
                const Vec3& p0 = positions[indices[0]];
                const Vec3& p1 = positions[indices[i]];
                const Vec3& p2 = positions[indices[i + 1]];
                Vec3 normal = normalize(cross(p1 - p0, p2 - p0));
                if (!std::isfinite(normal.x) || !std::isfinite(normal.y) || !std::isfinite(normal.z)) {
                    normal = Vec3(0.0f, 1.0f, 0.0f);
                }

                vertices.push_back(Vertex{p0, normal, color});
                vertices.push_back(Vertex{p1, normal, color});
                vertices.push_back(Vertex{p2, normal, color});
            }
        }
    }

    if (vertices.empty()) {
        throw std::runtime_error("OBJ contained no triangles: " + path.string());
    }
    return vertices;
}
