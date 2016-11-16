#include "Mesh.hpp"

#include <util/OCL_CALL.hpp>

namespace pbd {
    Mesh::Mesh(std::vector<Vertex>    && vertices,
               std::vector<Edge>      && edges,
               std::vector<Triangle>  && triangles,
               GLenum usage)
            : mVertexBuffer(GL_ARRAY_BUFFER, usage),
              mEdgeBuffer(GL_ARRAY_BUFFER, usage),
              mTriangleBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW),
              mVertices(vertices),
              mEdges(edges),
              mTriangles(triangles),
              mHasUploadedHostData(false) {
        mTexDiffuse.ID = 0;
        mTexSpecular.ID = 0;
        mTexBump.ID = 0;
    }

    void Mesh::render(clgl::BaseShader &shader, const glm::mat4 &VP, const glm::mat4 &M) {
        shader.use();
        shader.uniform("VP", VP);
        shader.uniform("MVP", VP * M);
        shader.uniform("M", M);
        shader.uniform("normalMultiplier", 1.0f);

        shader.uniform("HasDiffuseTexture", static_cast<int>(mTexDiffuse.ID != 0));
        if (mTexDiffuse.ID != 0) {
            shader.uniform("diffuseTexture", static_cast<int>(Texture::Type::DIFFUSE));
            OGL_CALL(glActiveTexture(GL_TEXTURE0 + Texture::Type::DIFFUSE));
            OGL_CALL(glBindTexture(GL_TEXTURE_2D, mTexDiffuse.ID));
        }

        shader.uniform("HasSpecularTexture", static_cast<int>(mTexSpecular.ID != 0));
        if (mTexSpecular.ID != 0) {
            shader.uniform("specularTexture", static_cast<int>(Texture::Type::SPECULAR));
            OGL_CALL(glActiveTexture(GL_TEXTURE0 + Texture::Type::SPECULAR));
            OGL_CALL(glBindTexture(GL_TEXTURE_2D, mTexSpecular.ID));
        }

        shader.uniform("HasBumpTexture", static_cast<int>(mTexBump.ID != 0));
        if (mTexBump.ID != 0) {
            shader.uniform("bumpTexture", static_cast<int>(Texture::Type::BUMP));
            OGL_CALL(glActiveTexture(GL_TEXTURE0 + Texture::Type::BUMP));
            OGL_CALL(glBindTexture(GL_TEXTURE_2D, mTexBump.ID));
        }

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
        mNumEdges = mEdges.size();
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

        mEdgeBuffer.bind();
        mEdgeBuffer.bufferData(numEdges() * sizeof(Edge), mEdges.data());

        mTriangleBuffer.bind();
        mTriangleBuffer.bufferData(numTriangles() * sizeof(Triangle), mTriangles.data());

        mVAO.unbind();
    }

    void Mesh::generateBuffersCL(cl::Context &context) {
        OCL_ERROR;
        OCL_CHECK(mVertexBufferCL = cl::BufferGL(context, CL_MEM_READ_WRITE, mVertexBuffer.ID(), CL_ERROR));
        OCL_CHECK(mEdgeBufferCL = cl::BufferGL(context, CL_MEM_READ_WRITE, mEdgeBuffer.ID(), CL_ERROR));
        OCL_CHECK(mTriangleBufferCL = cl::BufferGL(context, CL_MEM_READ_ONLY, mTriangleBuffer.ID(), CL_ERROR));
    }

    void Mesh::clearHostData() {
        mVertices.clear();
        mEdges.clear();
        mTriangles.clear();
    }

    std::vector<cl::Memory> Mesh::getMemoryCL() {
        std::vector<cl::Memory> memory;
        memory.push_back(mVertexBufferCL);
        memory.push_back(mEdgeBufferCL);
        memory.push_back(mTriangleBufferCL);
        return memory;
    }

    unsigned long Mesh::numVertices() {
        return mHasUploadedHostData ? mNumVertices : mVertices.size();
    }

    unsigned long Mesh::numEdges() {
        return mHasUploadedHostData ? mNumEdges : mEdges.size();
    }

    unsigned long Mesh::numTriangles() {
        return mHasUploadedHostData ? mNumTriangles : mTriangles.size();
    }
}
