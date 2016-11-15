/**
 * OpenCL representation of extra cloth vertex data. Matches the memory
 * layout of the ClothVertexData struct in src/simulation/geometry.hpp
 */
typedef struct def_ClothVertexData {
    uint vertexID; // don't know if this is needed
    float mass;
    float invmass;
} ClothVertexData;

/**
 * OpenCL representation of extra cloth edge data. Matches the memory
 * layout of the ClothEdgeData struct in src/simulation/geometry.hpp
 */
typedef struct def_ClothEdge {
    uint edgeID;
    float initialDihedralAngle;
    float initialLength;
};

/**
 * OpenCL representation of cloth triangle data. Matches the memory
 * layout of the ClothTriangleData struct in src/simulation/geometry.hpp
 */
typedef struct def_ClothTriangleData {
    uint triangleID; // don't know if this is needed
    int neighbourIDs[3];
    float mass;
} ClothTriangleData;
