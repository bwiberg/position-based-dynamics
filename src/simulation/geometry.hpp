#pragma once

#include <geometry/geometry.hpp>

namespace pbd {
    /**
      * Host (CPU) representation of a extra cloth vertex data.
      * Matches the memory layout of the ClothVertex struct
      * in kernels/common/ClothMesh.cl
      */
    struct ATTR_PACKED ClothVertexData {
        unsigned int vertexID;
        float mass;
        float invmass;
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
        float mass;
    };
}