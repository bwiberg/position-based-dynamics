#include "Mesh.hpp"

#include <util/OCL_CALL.hpp>

namespace pbd {
    Mesh::Mesh(std::vector<Vertex> &&vertices, std::vector<Triangle> &&triangles, GLenum usage)
            : mVertexBuffer(GL_ARRAY_BUFFER, usage),
              mTriangleBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW),
              mVertices(vertices),
              mTriangles(triangles),
              mHasUploadedHostData(false) {}


    void Mesh::render(clgl::BaseShader &shader, const glm::mat4 &VP, const glm::mat4 &M) {
        shader.use();
        shader.uniform("VP", VP);
        shader.uniform("MVP", VP * M);
        shader.uniform("M", M);

        mVAO.bind();
        OGL_CALL(glDrawElements(GL_TRIANGLES, 3 * static_cast<GLsizei>(numTriangles()), GL_UNSIGNED_INT, 0));
        mVAO.unbind();
    }

    void Mesh::flipNormals() {
        // Flip the direction of each geometric normal
        for (auto &vertex : mVertices) {
            vertex.normal = -vertex.normal;
        }

        // Swap the indices of each triangle/face, so that OpenGL
        // correctly culls faces based on index ordering.
        for (auto &triangle : mTriangles) {
            std::swap(triangle.vertices[1], triangle.vertices[2]);
        }
    }

    void Mesh::uploadHostData() {
        if (mHasUploadedHostData) return;

        mHasUploadedHostData = true;
        mNumVertices = mVertices.size();
        mNumTriangles = mTriangles.size();

        mVAO.bind();

        mVertexBuffer.bind();
        mVertexBuffer.bufferData(numVertices() * sizeof(Vertex), mVertices.data());

        OGL_CALL(glEnableVertexAttribArray(VertexAttributes::POSITION));
        OGL_CALL(glVertexAttribPointer(VertexAttributes::POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                       (GLvoid *) offsetof(Vertex, position)));

        OGL_CALL(glEnableVertexAttribArray(VertexAttributes::NORMAL));
        OGL_CALL(glVertexAttribPointer(VertexAttributes::NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                       (GLvoid *) offsetof(Vertex, normal)));

        OGL_CALL(glEnableVertexAttribArray(VertexAttributes::TEX_COORD));
        OGL_CALL(glVertexAttribPointer(VertexAttributes::TEX_COORD, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                       (GLvoid *) offsetof(Vertex, texCoord)));

        OGL_CALL(glEnableVertexAttribArray(VertexAttributes::COLOR));
        OGL_CALL(glVertexAttribPointer(VertexAttributes::COLOR, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                       (GLvoid *) offsetof(Vertex, color)));

        mTriangleBuffer.bind();
        mTriangleBuffer.bufferData(numTriangles() * sizeof(Triangle), mTriangles.data());

        mVAO.unbind();
    }

    void Mesh::generateBuffersCL(cl::Context &context) {
        OCL_ERROR;
        OCL_CHECK(mVertexBufferCL = cl::BufferGL(context, CL_MEM_READ_WRITE, mVertexBuffer.ID(), CL_ERROR));
        OCL_CHECK(mTriangleBufferCL = cl::BufferGL(context, CL_MEM_READ_ONLY, mTriangleBuffer.ID(), CL_ERROR));
    }

    void Mesh::clearHostData() {
        mVertices.clear();
        mTriangles.clear();
    }

    unsigned long Mesh::numVertices() {
        return mHasUploadedHostData ? mNumVertices : mVertices.size();
    }

    unsigned long Mesh::numTriangles() {
        return mHasUploadedHostData ? mNumTriangles : mTriangles.size();
    }

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
                        std::move(clothTriangleData)) {}

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
        mVertexVelocitiesBuffer.bufferData(numVertices() * sizeof(glm::vec3), 0);

        OGL_CALL(glEnableVertexAttribArray(VertexAttributes::VELOCITY));
        OGL_CALL(glVertexAttribPointer(VertexAttributes::VELOCITY, 1, GL_FLOAT, GL_FALSE, sizeof(glm::vec3),
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
    }

    void ClothMesh::clearHostData() {
        Mesh::clearHostData();
        mVertexClothData.clear();
        mTriangleClothData.clear();
    }
}
