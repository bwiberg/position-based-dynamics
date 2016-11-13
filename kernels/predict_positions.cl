/**
 * OpenCL representation of a vertex. Matches the memory layout
 * of the Vertex struct in src/geometry/geometry.hpp
 */
typedef struct def_Vertex {
//    float3 position;
//    float3 normal;
//    float2 texCoord;
//    float4 color;
//    float data[12];
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

__kernel void predict_positions(__global const ClothVertexData  *clothVertexData,
                                __global float3                 *predictedPositions,
                                __global float3                 *velocities,
                                __global Vertex                 *vertices,
                                const float                     deltaTime) {
    Vertex vertex = vertices[ID];
    float3 velocity = velocities[ID];
    velocity.y = velocity.y - deltaTime * 9.82f;

    velocities[ID] = velocity;

    float3 position = float3(vertex.position[0], vertex.position[1], vertex.position[2]);

    predictedPositions[ID] = position + deltaTime * velocity;
}

__kernel void set_positions_to_predicted(__global const float3  *predictedPositions,
                                         __global Vertex        *vertices) {
    //vertices[ID].position = predictedPositions[ID];
    //vertices[ID].data[1] = 1.0f;
    vertices[ID].position[0] = predictedPositions[ID].x;
    vertices[ID].position[1] = predictedPositions[ID].y;
    //vertices[ID].data[2] = predictedPositions[ID].z;
}
