#include "Mesh.hpp"

#include <util/OCL_CALL.hpp>

namespace pbd {
    ClothMesh::ClothMesh(std::vector<Vertex>               && vertices,
                         std::vector<ClothVertexData>      && clothVertexData,
                         std::vector<Edge>                 && edges,
                         std::vector<ClothEdgeData>        && clothEdgeData,
                         std::vector<Triangle>             && triangles,
                         std::vector<ClothTriangleData>    && clothTriangleData)
            : Mesh(std::move(vertices),
                   std::move(edges),
                   std::move(triangles),
                   GL_DYNAMIC_DRAW),
              mVertexClothBuffer(GL_ARRAY_BUFFER, GL_DYNAMIC_DRAW),
              mVertexVelocitiesBuffer(GL_ARRAY_BUFFER, GL_DYNAMIC_DRAW),
              mVertexPredictedPositionsBuffer(GL_ARRAY_BUFFER, GL_DYNAMIC_DRAW),
              mVertexPositionCorrectionsBuffer(GL_ARRAY_BUFFER, GL_DYNAMIC_DRAW),
              mVertexClothData(clothVertexData),
              mEdgeClothData(clothEdgeData),
              mTriangleClothData(clothTriangleData) {}

    ClothMesh::ClothMesh(Mesh && mesh,
                         std::vector<ClothVertexData>      && clothVertexData,
                         std::vector<ClothEdgeData>        && clothEdgeData,
                         std::vector<ClothTriangleData>    && clothTriangleData)
            : ClothMesh(std::move(mesh.mVertices),
                        std::move(clothVertexData),
                        std::move(mesh.mEdges),
                        std::move(clothEdgeData),
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

        std::vector<glm::vec4> velocities;
        velocities.resize(numVertices());
        std::fill(velocities.begin(), velocities.end(), glm::vec4(0.0f));
        mVertexVelocitiesBuffer.bind();
        mVertexVelocitiesBuffer.bufferData(numVertices() * sizeof(glm::vec4), velocities.data());

        OGL_CALL(glEnableVertexAttribArray(VertexAttributes::VELOCITY));
        OGL_CALL(glVertexAttribPointer(VertexAttributes::VELOCITY, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4),
                                       (GLvoid *) 0));

        mVAO.unbind();

        mVertexPredictedPositionsBuffer.bind();
        mVertexPredictedPositionsBuffer.bufferData(numVertices() * sizeof(glm::vec4), velocities.data());

        mVertexPositionCorrectionsBuffer.bind();
        mVertexPositionCorrectionsBuffer.bufferData(numVertices() * sizeof(glm::vec4), velocities.data());
    }

    void ClothMesh::generateBuffersCL(cl::Context &context) {
        Mesh::generateBuffersCL(context);
        OCL_ERROR;

        OCL_CHECK(mVertexClothBufferCL = cl::BufferGL(context, CL_MEM_READ_ONLY, mVertexClothBuffer.ID()));
        OCL_CHECK(mVertexVelocitiesBufferCL = cl::BufferGL(context, CL_MEM_READ_WRITE, mVertexVelocitiesBuffer.ID()));
        OCL_CHECK(mEdgeClothBufferCL = cl::Buffer(context, CL_MEM_READ_ONLY,
                                                  sizeof(ClothEdgeData) * numEdges()));
        OCL_CHECK(mTriangleClothBufferCL = cl::Buffer(context, CL_MEM_READ_ONLY,
                                                      sizeof(ClothTriangleData) * numTriangles()));
        OCL_CHECK(mVertexPredictedPositionsBufferCL = cl::BufferGL(context, CL_MEM_READ_WRITE,
                                                                   mVertexPredictedPositionsBuffer.ID()));
        OCL_CHECK(mVertexPositionCorrectionsBufferCL = cl::BufferGL(context, CL_MEM_READ_WRITE,
                                                                   mVertexPositionCorrectionsBuffer.ID()));
        OCL_CHECK(mDistToLineBufferCL = cl::Buffer(context, CL_MEM_READ_WRITE,
                                                   sizeof(cl_float) * numVertices(),
                                                   (void*)0, CL_ERROR));
        OCL_CHECK(mVertexInBinPosCL = cl::Buffer(context, CL_MEM_READ_WRITE,
                                                 sizeof(cl_uint) * numVertices(),
                                                 (void*)0, CL_ERROR));
    }

    void ClothMesh::clearHostData() {
        Mesh::clearHostData();
        mVertexClothData.clear();
        mEdgeClothData.clear();
        mTriangleClothData.clear();
    }

    std::vector<cl::Memory> ClothMesh::getMemoryCL() {
        auto memory = Mesh::getMemoryCL();
        memory.push_back(mVertexPredictedPositionsBufferCL);
        memory.push_back(mVertexVelocitiesBufferCL);
        return memory;
    }

    void ClothMesh::render(clgl::BaseShader &shader, const glm::mat4 &VP, const glm::mat4 &M) {
        // render front-side of cloth
        OGL_CALL(glCullFace(GL_BACK));
        shader.uniform("normalMultiplier", 1.0f);
        Mesh::render(shader, VP, M);

        // render back-side of cloth
        OGL_CALL(glCullFace(GL_FRONT));
        shader.uniform("normalMultiplier", -1.0f);
        Mesh::render(shader, VP, M);

        OGL_CALL(glCullFace(GL_BACK));
    }
}
