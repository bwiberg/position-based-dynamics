#pragma once

#include <memory>
#include <vector>
#include <geometry/Mesh.hpp>

namespace pbd {
    namespace MeshLoader {
        std::shared_ptr<Mesh> LoadMesh(const std::string &path);

        std::shared_ptr<ClothMesh> LoadClothMesh(const std::string &path);
    };
}