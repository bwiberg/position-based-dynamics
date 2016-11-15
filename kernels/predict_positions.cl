/**
 * OpenCL representation of a vertex. Matches the memory layout
 * of the Vertex struct in src/geometry/geometry.hpp
 */
typedef struct def_Vertex {
    float position[3];
    float normal[3];
    float texCoord[2];
    float color[4];
} Vertex;

/**
 * OpenCL representation of extra cloth vertex data. Matches the memory
 * layout of the ClothVertexData struct in src/geometry/geometry.hpp
 */
typedef struct def_ClothVertexData {
    uint vertexID; // don't know if this is needed
    float mass;
    float invmass;
} ClothVertexData;

#define ID get_global_id(0)

inline float3 Float3(float x, float y, float z) {
    float3 vec;
    vec.x = x;
    vec.y = y;
    vec.z = z;
    return vec;
}

#define POSITION(vertex) Float3(vertex.position[0], vertex.position[1], vertex.position[2])

__kernel void predict_positions(__global float3         *predictedPositions, // 0
                                __global float3         *velocities,         // 1
                                __global const Vertex   *vertices,           // 2
                                const float             deltaTime) {         // 3

    float3 velocity = velocities[ID];
    velocity.y = velocity.y - deltaTime * 9.82f;
    velocities[ID] = velocity;

    float3 position = POSITION(vertices[ID]);
    position = position + deltaTime * velocity;

    predictedPositions[ID] = position;
}

__kernel void set_positions_to_predicted(__global const float3  *predictedPositions,
                                         __global Vertex        *vertices) {
    vertices[ID].position[0] = predictedPositions[ID].x;
    vertices[ID].position[1] = predictedPositions[ID].y;
    vertices[ID].position[2] = predictedPositions[ID].z;
}
