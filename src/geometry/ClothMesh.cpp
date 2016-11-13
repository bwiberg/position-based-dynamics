#include "Mesh.hpp"

#include <util/OCL_CALL.hpp>

namespace pbd {
    ClothMesh::ClothMesh(std::vector<Vertex> &&vertices,
                         std::vector<ClothVertexData> &&clothVertexData,
                         std::vector<Triangle> &&triangles,
                         std::vector<ClothTriangleData> &&clothTriangleData)
            : Mesh(std::move(vertices), std::move(triangles), GL_DYNAMIC_DRAW),
              mVertexClothBuffer(GL_ARRAY_BUFFER, GL_DYNAMIC_DRAW),
              mVertexVelocitiesBuffer(GL_ARRAY_BUFFER, GL_DYNAMIC_DRAW),
              mVertexClothData(clothVertexData),
              mTriangleClothData(clothTriangleData) {}

    ClothMesh::ClothMesh(Mesh &&mesh,
                         std::vector<ClothVertexData> &&clothVertexData,
                         std::vector<ClothTriangleData> &&clothTriangleData)
            : ClothMesh(std::move(mesh.mVertices),
                        std::move(clothVertexData),
                        std::move(mesh.mTriangles),
                        std::move(clothTriangleData)) {
        mTexDiffuse = mesh.mTexDiffuse;
        mTexSpecular = mesh.mTexSpecular;
        mTexBump = mesh.mTexBump;
    }

    void ClothMesh::uploadHostData() {
        if (mHasUploadedHostData) return;

        Mesh::uploadHostData();

        mVAO.bind();

        mVertexClothBuffer.bind();
        mVertexClothBuffer.bufferData(numVertices() * sizeof(ClothVertexData), mVertexClothData.data());

        OGL_CALL(glEnableVertexAttribArray(VertexAttributes::MASS));
        OGL_CALL(glVertexAttribPointer(VertexAttributes::MASS, 1, GL_FLOAT, GL_FALSE, sizeof(ClothVertexData),
                                       (GLvoid *) offsetof(ClothVertexData, mass)));

        mVertexVelocitiesBuffer.bind();
        mVertexVelocitiesBuffer.bufferData(numVertices() * sizeof(glm::vec4), 0);

        OGL_CALL(glEnableVertexAttribArray(VertexAttributes::VELOCITY));
        OGL_CALL(glVertexAttribPointer(VertexAttributes::VELOCITY, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec3),
                                       (GLvoid *) 0));

        mVAO.unbind();
    }

    void ClothMesh::generateBuffersCL(cl::Context &context) {
        Mesh::generateBuffersCL(context);
        OCL_ERROR;

        OCL_CHECK(mVertexClothBufferCL = cl::BufferGL(context, CL_MEM_READ_ONLY, mVertexClothBuffer.ID()));
        OCL_CHECK(mVertexVelocitiesBufferCL = cl::BufferGL(context, CL_MEM_READ_ONLY, mVertexVelocitiesBuffer.ID()));
        OCL_CHECK(mTriangleClothBufferCL = cl::Buffer(context, CL_MEM_READ_ONLY,
                                                      sizeof(ClothTriangleData) * numTriangles()));
        OCL_CHECK(mVertexPredictedPositionsBufferCL = cl::Buffer(context, CL_MEM_READ_ONLY,
                                                                 sizeof(cl_float3) * numVertices()));

    }

    void ClothMesh::clearHostData() {
        Mesh::clearHostData();
        mVertexClothData.clear();
        mTriangleClothData.clear();
    }

    void ClothMesh::render(clgl::BaseShader &shader, const glm::mat4 &VP, const glm::mat4 &M) {
        OGL_CALL(glDisable(GL_CULL_FACE));
        Mesh::render(shader, VP, M);
        OGL_CALL(glEnable(GL_CULL_FACE));
    }
}
