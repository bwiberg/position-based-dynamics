/**
 * OpenCL representation of extra cloth vertex data. Matches the memory
 * layout of the ClothVertexData struct in src/geometry/geometry.hpp
 */
typedef struct def_ClothVertexData {
    uint vertexID; // don't know if this is needed
    float mass;
    float invmass;
} ClothVertexData;

/**
 * OpenCL representation of cloth triangle data. Matches the memory
 * layout of the ClothTriangleData struct in src/geometry/geometry.hpp
 */
typedef struct def_ClothTriangleData {
    uint triangleID; // don't know if this is needed
    int neighbourIDs[3];
    float mass;
} ClothTriangleData;

/**
 * OpenCL representation of a cloth mesh. Does NOT match the memory
 * layout of the ClothMesh struct in src/rendering/ClothMesh.hpp
 */
typedef struct def_ClothMesh {
    Vertex *vertices;
    ClothVertexData *vertexData;

    Triangle *triangles;
    ClothTriangleData *triangleData;
} ClothMesh;
