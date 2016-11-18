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

//#define __DEBUG__

#ifdef __DEBUG__
#define DBG(prefix, _float) printf("%s%f\n", prefix, _float)
#define DBG3(prefix, vec3) printf("%s[%f, %f, %f]\n", prefix, vec3.x, vec3.y, vec3.z)
#define DBG_IF_ID(id, prefix, _float) if (get_global_id(0) == id) printf("%s%f\n", prefix, _float)
#define DBG3_IF_ID(id, prefix, vec3) if (get_global_id(0) == id) printf("%s[%f, %f, %f]\n", prefix, vec3.x, vec3.y, vec3.z)
#else
#define DBG(prefix, _float) //
#define DBG3(prefix, vec3) //
#define DBG_IF_ID(id, prefix, _float) //
#define DBG3_IF_ID(id, prefix, vec3) //
#endif

/**
 * (runs for every vertex)
 *
 * Calculates the distance of each vertex to a world-space line/ray.
 */
__kernel void calc_dist_to_line(__global const Vertex   *vertices,          // 0
                                __global float          *distances,         // 1
                                const float3            lineOrigin,         // 2
                                const float3            lineDirection) {    // 3

    float3 position = POSITION(vertices[ID]);
    position.y = -position.y;

    const float3 relposition = position - lineOrigin;

    // projects the vertex onto the line
    const float3 relposition_parallel_line = lineDirection * dot(lineDirection, relposition);
    const float3 relposition_orthogonal_line = relposition - relposition_parallel_line;

    distances[ID] = length(relposition_orthogonal_line);
}

/**
 * (runs for every vertex)
 *
 * Calculates the distance of each vertex to a world-space line/ray.
 */
__kernel void predict_positions(__global float3         *predictedPositions, // 0
                                __global float3         *velocities,         // 1
                                __global const Vertex   *vertices,           // 2
                                __global const ClothVertexData *clothVertices, // 3
                                const float             deltaTime) {         // 4

    const float3 origposition = POSITION(vertices[ID]);
    DBG3_IF_ID(3, "origposition=", origposition);
    
    const float factor = clothVertices[ID].invmass * clothVertices[ID].mass;
    DBG_IF_ID(3, "factor=", factor);
    
    float3 velocity = velocities[ID];
    DBG3_IF_ID(3, "origvelocity=", velocity);
    
    velocity.y -= factor * deltaTime * 9.82f;
    DBG3_IF_ID(3, "newvelocity =", velocity);
    
    velocity = 0.99f * velocity;
        
    const float3 newposition = origposition + factor * deltaTime * velocity;
    
    predictedPositions[ID] = newposition;
}

/**
 * (runs for every vertex)
 *
 * Applies gravity to velocities and predicts positions based on an Euler timestep
 */
__kernel void set_positions_to_predicted(__global const float3  *predictedPositions,        // 0
                                         __global Vertex        *vertices,          // 1
                                         __global float3        *velocities,        // 2
                                         const float            deltaTime) {      // 3
    
    const float3 origposition = POSITION(vertices[ID]);
    const float3 newposition = predictedPositions[ID];
    const float3 velocity = (newposition - origposition) / deltaTime;
    
    DBG3_IF_ID(3, "origposition=", origposition);
    DBG3_IF_ID(3, "velocity    =", velocity);
    DBG3_IF_ID(3, "newposition =", newposition);
    
    velocities[ID] = velocity;
    
    vertices[ID].position[0] = newposition.x;
    vertices[ID].position[1] = newposition.y;
    vertices[ID].position[2] = newposition.z;
}
