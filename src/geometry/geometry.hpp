#pragma once

#include <vector>
#include <CL/cl.hpp>
#include <glm/glm.hpp>

#define ATTR_PACKED __attribute__ ((__packed__))

namespace pbd {
    enum VertexAttributes {
        POSITION = 0,
        NORMAL = 1,
        TEX_COORD = 2,
        COLOR = 3,
        VELOCITY = 4,
        MASS = 5
    };

    /**
     * Host (CPU) representation of a vertex.
     * Matches the memory layout of the Vertex struct
     * in kernels/common/Mesh.cl
     *
     * The __padding elements are needed to match vec3's
     * with OpenCL's cl_float3, which * are 4-component
     * vectors.
     */
    struct ATTR_PACKED Vertex {
        glm::vec3 position;
        float __padding0;
        glm::vec3 normal;
        float __padding1;
        glm::vec2 texCoord;
        glm::vec4 color;
    };

    /**
     * Host (CPU) representation of a triangle.
     * Matches the memory layout of the Triangle struct
     * in kernels/common/Mesh.cl
     */
    struct ATTR_PACKED Triangle {
        glm::uvec3 vertices;
    };

    /**
      * Host (CPU) representation of a extra cloth vertex data.
      * Matches the memory layout of the ClothVertex struct
      * in kernels/common/ClothMesh.cl
      */
    struct ATTR_PACKED ClothVertexData {
        unsigned int vertexID;
        float mass;
    };

    /**
     * Host (CPU) representation of cloth triangle data.
     * Matches the memory layout of the ClothVertexData
     * struct in kernels/common/ClothMesh.cl
     */
    struct ATTR_PACKED ClothTriangleData {
        unsigned int triangleID;

        /**
         * Every triangle has 0-3 neighbours.
         * Neighbour 0 shares vertices [0, 1]
         * Neighbour 1 shares vertices [0, 2]
         * Neighbour 2 shares vertices [1, 2]
         *
         * If a neighbourID is equal to -1 it means that edge is not shared.
         */
        int neighbourIDs[3];
    };
}
