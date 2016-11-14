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
 * OpenCL representation of a triangle. Matches the memory layout
 * of the Triangle struct in src/geometry/geometry.hpp
 */
typedef struct def_Triangle {
    uint vertices[3];
} Triangle;

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

    // initial dihedral angles to each neighbour
    float initialAngles[3];
} ClothTriangleData;

float calc_dihedral_angle(const Vertex v1,
                          const Vertex v2,
                          const Vertex v3,
                          const Vertex v4);

float3 Float3(float x, float y, float z);

float3 Cross(float3 u, float3 v);

#define POSITION(vertex) Float3(vertex.position[0], vertex.position[1], vertex.position[2])

#define ID get_global_id(0)

/**
 * For every triangle
 */
__kernel void calc_triangle_mass(__global const Vertex      *vertices,          // 0
                                 __global const Triangle    *triangles,         // 1
                                 __global ClothTriangleData *clothTriangles) {  // 2

    // get the vertices that make up the triangle
    const Vertex A = vertices[triangles[ID].vertices[0]];
    const Vertex B = vertices[triangles[ID].vertices[1]];
    const Vertex C = vertices[triangles[ID].vertices[2]];

    const float3 Apos = POSITION(A);
    const float3 Bpos = POSITION(B);
    const float3 Cpos = POSITION(C);

    const float3 AB = Bpos - Apos;
    const float3 AC = Cpos - Apos;

    const float trianglesMass = 0.5 * length(Cross(AB, AC));

    clothTriangles[ID].mass = trianglesMass;
}

#define ONE_THIRD 0.33333f

/**
 * For every triangle
 */
__kernel void calc_vertex_mass(__global ClothVertexData         *clothVertices,     // 0
                               __global const Triangle          *triangles,         // 1
                               __global const ClothTriangleData *clothTriangles) {  // 2

    const float triangleMass = clothTriangles[ID].mass;

    clothVertices[triangles[ID].vertices[0]].mass += ONE_THIRD * triangleMass;
    clothVertices[triangles[ID].vertices[1]].mass += ONE_THIRD * triangleMass;
    clothVertices[triangles[ID].vertices[2]].mass += ONE_THIRD * triangleMass;
}

/**
 * For every vertex
 */
__kernel void calc_inverse_mass(__global ClothVertexData *clothVertices) {
    clothVertices[ID].invmass = 1.0f / clothVertices[ID].mass;
}

/**
 * For every triangle
 */
__kernel void calc_initial_dihedral_angles(__global const Vertex        *vertices,          // 0
                                           __global const Triangle      *triangles,         // 1
                                           __global ClothTriangleData   *clothTriangles) {  // 2
    const ClothTriangleData clothTriangle = clothTriangles[ID];

    for (uint i = 0; i < 3; ++i) {
        if (clothTriangle.neighbourIDs[i] < 0) continue;


    }
}

float calc_dihedral_angle(const Vertex v1,
                          const Vertex v2,
                          const Vertex v3,
                          const Vertex v4) {
    const float3 p1 = POSITION(v1);
    const float3 p2 = POSITION(v2);
    const float3 p3 = POSITION(v3);
    const float3 p4 = POSITION(v4);

    const float3 n1 = normalize(Cross(p2 - p1, p3 - p1));
    const float3 n2 = normalize(Cross(p2 - p1, p4 - p1));

    return acos(dot(n1, n2));
}

float3 Float3(float x, float y, float z) {
    float3 vec;
    vec.x = x;
    vec.y = y;
    vec.z = z;
    return vec;
}

float3 Cross(float3 u, float3 v) {
    float3 result;
    result.x = u_.y * v_.z - u_.z * v_.y;
    result.y = u_.z * v_.x - u_.x * v_.z;
    result.z = u_.x * v_.y - u_.y * v_.x;
    return result;
}
