// // // // // // // // // // // /// // // // // // //
/// /// /// /// /// /// UNUSED /// /// /// /// /// ///
// // // /// // // // // // // // // // // // // // //

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


float3 Float3(float x, float y, float z);

float3 Cross(float3 u, float3 v);

#define POSITION(vertex) Float3(vertex.position[0], vertex.position[1], vertex.position[2])
#define NORMAL(vertex) Float3(vertex.normal[0], vertex.normal[1], vertex.normal[2])
#define ID get_global_id(0)

__kernel void calc_vertex_normals(__global Vertex           *vertices,      // 0
                                  __global const Triangle   *triangles) {   // 1

    const Triangle triangle = triangles[ID];

    // get the vertices that make up the triangle
    const Vertex A = vertices[triangle.vertices[0]];
    const Vertex B = vertices[triangle.vertices[1]];
    const Vertex C = vertices[triangle.vertices[2]];

    const float3 Apos = POSITION(A);
    const float3 Bpos = POSITION(B);
    const float3 Cpos = POSITION(C);

    const float3 AB = Bpos - Apos;
    const float3 AC = Cpos - Apos;

    float3 normal = normalize(Cross(AB, AC)); // check order

    // "add" the triangle's normal to each of its vertices' normals
    for (uint vertID = 0; vertID < 3; ++vertID) {
        vertices[triangle.vertices[vertID]].normal.x += normal.x;
        vertices[triangle.vertices[vertID]].normal.y += normal.y;
        vertices[triangle.vertices[vertID]].normal.z += normal.z;
    }
}

/**
 * For every vertex
 */
__kernel void normalize_vertex_normals(__global Vertex *vertices) {
    float3 normal = NORMAL(vertices[ID]);
    normal = normalize(normal);

    vertices[ID].normal[0] = normal.x;
    vertices[ID].normal[1] = normal.y;
    vertices[ID].normal[2] = normal.z;
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
