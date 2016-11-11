#include "MeshObject.hpp"

namespace clgl {
    MeshObject::MeshObject(std::shared_ptr<pbd::Mesh> mesh,
                           std::shared_ptr<BaseShader> shader)
            : RenderObject(shader), mMesh(mesh) {}

    void MeshObject::render(const glm::mat4 &viewProjection) {
        mMesh->render(*mShader, viewProjection, getTransform());
    }
}
