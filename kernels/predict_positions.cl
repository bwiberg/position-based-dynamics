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
                                __global const ClothVertexData *clothVertices, // 3
                                const float             deltaTime) {         // 4

    const float3 origposition = POSITION(vertices[ID]);
    
    const float factor = clothVertices[ID].invmass * clothVertices[ID].mass;
    
    float3 velocity = velocities[ID];
    if (ID == 1) {
        printf("pre-g  velocity=[%f, %f, %f]\n", velocity.x, velocity.y, velocity.z);
    }
    velocity.y -= factor * deltaTime * 9.82f;
    if (ID == 1) {
        printf("post-g velocity=[%f, %f, %f]\n", velocity.x, velocity.y, velocity.z);
    }
        
    const float3 newposition = origposition + factor * deltaTime * velocity;
    
//    printf("ID=%i\nposition0=[%f, %f, %f]\nposition1=[%f, %f, %f]\nvelocity0=[%f, %f, %f]\nvelocity1=[%f, %f, %f]\n",
//           ID,
//           POSITION(vertices[ID]).x, POSITION(vertices[ID]).y, POSITION(vertices[ID]).z,
//           position.x, position.y, position.z,
//           velocities[ID].x, velocities[ID].y, velocities[ID].z,
//           velocity.x, velocity.y, velocity.z);
//    velocities[ID] = velocity;
    predictedPositions[ID] = newposition;
}

__kernel void set_positions_to_predicted(__global const float3  *predictedPositions,        // 0
                                         __global Vertex        *vertices,          // 1
                                         __global float3        *velocities,        // 2
                                         const float            deltaTime) {      // 3
    
    const float3 origposition = POSITION(vertices[ID]);
    const float3 newposition = predictedPositions[ID];
    const float3 velocity = (newposition - origposition) / deltaTime;
    
    if (ID == 1) {
        //printf("origposition=[%f, %f, %f]\n", origposition.x, origposition.y, origposition.z);
        //printf("newposition =[%f, %f, %f]\n", newposition.x, newposition.y, newposition.z);
        printf("velocity    =[%f, %f, %f]\n", velocity.x, velocity.y, velocity.z);
    }

    velocities[ID] = velocity;
    
    vertices[ID].position[0] = newposition.x;
    vertices[ID].position[1] = newposition.y;
    vertices[ID].position[2] = newposition.z;
}
