typedef struct def_Vertex {
    float position[3];
    float normal[3];
    float texCoord[2];
    float color[4];
} Vertex;

typedef struct def_Edge {
    int triangles[2];
    int vertices[4]; // [p1, p2, p3, p4] from the PBD paper
} Edge;

typedef struct def_Triangle {
    uint vertices[3];
} Triangle;

typedef struct def_ClothVertexData {
    uint vertexID; // don't know if this is needed
    float mass;
    float invmass;
} ClothVertexData;

typedef struct def_ClothEdge {
    uint edgeID;
    float initialDihedralAngle;
    float initialLength;
} ClothEdgeData;

typedef struct def_ClothTriangleData {
    uint triangleID; // don't know if this is needed
    int neighbourIDs[3];
    float mass;
} ClothTriangleData;

float calc_dihedral_angle(const float3 p1,
                          const float3 p2,
                          const float3 p3,
                          const float3 p4);



float3 Float3(float x, float y, float z);

float3 Cross(float3 u, float3 v);

void atomic_add_global_float(volatile __global float *addr, float val);

#define POSITION(vertex) Float3(vertex.position[0], vertex.position[1], vertex.position[2])
#define ID get_global_id(0)

#define ONE_THIRD 0.33333f


////////////  ///////////////////////////////////////  ////////////
////////////  //////////// SETUP KERNELS ////////////  ////////////
////////////  ///////////////////////////////////////  ////////////

/**
 * For every triangle
 */
__kernel void calc_cloth_mass(__global const Vertex                 *vertices,          // 0
                              volatile __global ClothVertexData     *clothVertices,     // 1
                              __global const Triangle               *triangles,         // 2
                              __global ClothTriangleData            *clothTriangles) {  // 3

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

    const float triangleMass = 0.5f * length(Cross(AB, AC));

    if (ID < 6) {
        printf("calc_cloth_mass, ID=%i, triangleMass=%f\n", ID, triangleMass);
    }

    // save this mass into the triangle's memory
    clothTriangles[ID].mass = triangleMass;

    // Store one third of the triangle's mass into each of its vertices.
    // Needs to use atomic operations since multiple threads might try
    // to add to the mass at the same time.
    atomic_add_global_float(&clothVertices[triangle.vertices[0]].mass,
                            ONE_THIRD * triangleMass);

    atomic_add_global_float(&clothVertices[triangle.vertices[1]].mass,
                            ONE_THIRD * triangleMass);

    atomic_add_global_float(&clothVertices[triangle.vertices[2]].mass,
                            ONE_THIRD * triangleMass);

}

/**
 * For every vertex
 */
__kernel void calc_inverse_mass(__global ClothVertexData *clothVertices) {
    clothVertices[ID].invmass = 1.0f / clothVertices[ID].mass;

    if (ID < 6) {
    printf("calc_inverse_mass, ID=%i, mass=%f, invmass=%f\n", ID,
            clothVertices[ID].mass, clothVertices[ID].invmass);
    }
}

/**
 * For every edge
 */
__kernel void calc_edge_properties(__global const Vertex        *vertices,          // 0
                                   __global const Triangle      *triangles,         // 1
                                   __global Edge                *edges,             // 2
                                   __global ClothEdgeData       *clothEdges,        // 3
                                   __global ClothTriangleData   *clothTriangles) {  // 4

    const Edge thisedge = edges[ID];

    const int p1ID = thisedge.vertices[0];
    const int p2ID = thisedge.vertices[1];

    const float3 p1 = POSITION(vertices[p1ID]);
    const float3 p2 = POSITION(vertices[p2ID]);

    /// calc edge length
    clothEdges[ID].initialLength = length(p1 - p2);

    if (thisedge.triangles[1] == -1) {
        clothEdges[ID].initialDihedralAngle = 0.0f;
        if (ID < 6) {
            printf("calc_edge_properties, ID=%i, initialLength=%f, dihedralAngle=%f\n",
                    ID, clothEdges[ID].initialLength, clothEdges[ID].initialDihedralAngle);
        }

        return;
    };

    const int p3ID = thisedge.vertices[2];
    const int p4ID = thisedge.vertices[3];
    
    const float3 p3 = POSITION(vertices[p3ID]);
    const float3 p4 = POSITION(vertices[p4ID]);

    clothEdges[ID].initialDihedralAngle = calc_dihedral_angle(p1, p2, p3, p4);

    if (ID < 6) {
        printf("calc_edge_properties, ID=%i, initialLength=%f, dihedralAngle=%f\n",
        ID, clothEdges[ID].initialLength, clothEdges[ID].initialDihedralAngle);
    }
}

////////////  /////////////////////////////////////////////////////  ////////////
////////////  //////////// POSITION CORRECTION KERNELS ////////////  ////////////
////////////  /////////////////////////////////////////////////////  ////////////

__kernel void clip_to_planes(__global Vertex *vertices) {
    vertices[ID].position[1] = max(vertices[ID].position[1], 0.0f);
}

/**
 * For every edge
 */
__kernel void calc_position_corrections(__global const Vertex               *vertices,              // 0
                                     __global const ClothVertexData      *clothVertices,         // 1
                                     __global const Edge                 *edges,                 // 2
                                     __global const ClothEdgeData        *clothEdges,            // 3
                                     __global const Triangle             *triangles,             // 4
                                     __global const ClothTriangleData    *clothTriangles,        // 5
                                     __global const float3               *predictedPositions,    // 6
                                     __global float3                     *positionCorrections) { // 7
    
    const Edge edge                 = edges[ID];
    const ClothEdgeData clothEdge   = clothEdges[ID];
    
    //const Vertex v1 = edge.vertices[0];
    //const Vertex v2 = edge.vertices[1];
    
    //const float3 p1 = POSITION(v1);
    //const float3 p2 = POSITION(v2);
    
    /// stretch constraint
    //const float Cstretch = length(p1 - p2) - clothEdge.initialLength;
    
    
    
}

float calc_dihedral_angle(const float3 p1,
                          const float3 p2,
                          const float3 p3,
                          const float3 p4) {
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
    result.x = u.y * v.z - u.z * v.y;
    result.y = u.z * v.x - u.x * v.z;
    result.z = u.x * v.y - u.y * v.x;
    return result;
}

// source: https://streamcomputing.eu/blog/2016-02-09/atomic-operations-for-floats-in-opencl-improved/
void atomic_add_global_float(volatile __global float *addr, float val) {
    union{
        unsigned int u32;
        float        f32;
    } next, expected, current;
    current.f32    = *addr;
    do{
        expected.f32 = current.f32;
        next.f32     = expected.f32 + val;
        current.u32  = atomic_cmpxchg( (volatile __global unsigned int *)addr,
                                       expected.u32, next.u32);
    } while( current.u32 != expected.u32 );
}
