#pragma once

#include "RenderObject.hpp"
#include <geometry/Mesh.hpp>

namespace clgl {
    /// @brief //todo add brief description to MeshObject
    /// @author Benjamin Wiberg
    class MeshObject : public RenderObject {
    public:
        MeshObject(std::shared_ptr<pbd::Mesh> mesh, std::shared_ptr<BaseShader> shader);

        virtual void render(const glm::mat4 &viewProjection) override;

    private:
        const std::shared_ptr<pbd::Mesh> mMesh;
    };


}