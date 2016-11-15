#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable

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


typedef struct def_ClothSimParams {
    uint numSubSteps;   // Number of PBD constraint projection steps
    float deltaTime;    // Time step (dt):
    float k_stretch;    // PBD stiffness constant for cloth stretch constraint
    float k_bend;       // PBD stiffness constant for cloth bending constraint
} ClothSimParams;

float calc_dihedral_angle(const float3 p1,
                          const float3 p2,
                          const float3 p3,
                          const float3 p4);



float3 Float3(float x, float y, float z);

float3 Cross(float3 u, float3 v);

/**
 * Atomic
 */
void atomic_add_global_float(volatile __global float *addr, float val);

/**
 * Component-wise atomic addition of a float3 vector
 */
void cmpwise_atomic_add_global_float3(volatile __global float3 *addr, float3 val);

#define POSITION(vertex) Float3(vertex.position[0], vertex.position[1], vertex.position[2])
#define ID get_global_id(0)

#define PI 3.1415926535f
#define ONE_THIRD 0.33333f
#define EDGE_NO_TRIANGLE -1

#define clamped_acos(x) acos(clamp(x, -0.99999999f, 0.99999999f))


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

__kernel void fix_vertex(__global ClothVertexData *clothVertices,
                         const int idToFix) {
    if (ID != idToFix) return;
    
    clothVertices[ID].invmass = 0.0f;
}

/**
 * For every edge
 */
__kernel void calc_edge_properties(__global const Vertex        *vertices,          // 0
                                __global const Triangle      *triangles,         // 1
                                __global Edge               *edges,             // 2
                                __global ClothEdgeData      *clothEdges,        // 3
                                __global ClothTriangleData  *clothTriangles) {  // 4

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

__kernel void clip_to_planes(__global float3 *predictedPositions) {
    predictedPositions[ID].y = max(predictedPositions[ID].y, 0.02f);
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
                                     __global const float3                *predictedPositions,    // 6
                                     volatile __global float3              *positionCorrections,   // 7
                                     const ClothSimParams              params) {               // 8
    
    const Edge edge                 = edges[ID];
    const ClothEdgeData clothEdge   = clothEdges[ID];
    
    const int v1ID = edge.vertices[0];
    const int v2ID = edge.vertices[1];
    
    const ClothVertexData cv1   = clothVertices[v1ID];
    const ClothVertexData cv2   = clothVertices[v2ID];
    
    float3 p1 = predictedPositions[v1ID];
    float3 p2 = predictedPositions[v2ID];
    
    //////////////////////////
    /// stretch constraint ///
    //////////////////////////
    
    const float3 p2p1 = p1 - p2;
    const float p2p1length = length(p2p1);
    
    const float Cstretch = p2p1length - clothEdge.initialLength;
    const float3 gradCstretch = params.k_stretch * p2p1 / max(p2p1length, 0.1f);
    
    const float tmp = 1 / (cv1.invmass + cv2.invmass);
    
    float3 deltaP1 = -(cv1.invmass * tmp) * Cstretch * gradCstretch;
    float3 deltaP2 =  (cv2.invmass * tmp) * Cstretch * gradCstretch;
    
#define DBG_FLOAT3(vec) printf("[%f, %f, %f]\n", vec.x, vec.y, vec.z)
    
//    if (ID == 2) {
//        printf("v1ID=%i\n", v1ID);
//        printf("p1="); DBG_FLOAT3(p1);
//        printf("p2="); DBG_FLOAT3(p2);
//        printf("p2p1="); DBG_FLOAT3(p2p1);
//        printf("p2p1length=%f\n", p2p1length);
//        printf("Cstretch=%f\n", Cstretch);
//        printf("gradCstretch="); DBG_FLOAT3(gradCstretch);
//        printf("tmp=%f\n", tmp);
//    }
    
    ///////////////////////
    /// bend constraint ///
    ///////////////////////
    
    if (edge.triangles[1] != EDGE_NO_TRIANGLE) {
        const int v3ID = edge.vertices[2];
        const int v4ID = edge.vertices[3];
        
        const ClothVertexData cv3   = clothVertices[v3ID];
        const ClothVertexData cv4   = clothVertices[v4ID];
        
        // subtract p1 from all positions to get simpler expressions
        p2 -= p1;
        
        const float3 p3 = predictedPositions[v3ID] - p1;
        const float3 p4 = predictedPositions[v4ID] - p1;
        p1 = Float3(0.0f, 0.0f, 0.0f);
        
        const float3 n1 = normalize(Cross(p2, p3));
        const float3 n2 = normalize(Cross(p2, p4));
        const float d = dot(n1, n2);
        
        const float3 q3 = (Cross(p2, n2) + Cross(n1, p2) * d) / length(Cross(p2, p3));
        const float3 q4 = (Cross(p2, n1) + Cross(n2, p2) * d) / length(Cross(p2, p4));
        const float3 q2 = -(Cross(p3, n2) + Cross(n1, p3) * d) / length(Cross(p2, p3)) - (Cross(p4, n1) + Cross(n2, p4) * d) / length(Cross(p2, p4));
        const float3 q1 = - q2 - q3 - q4;
        
        const float denom = (cv1.invmass * pow(length(q1),2) +
                           cv2.invmass * pow(length(q2),2) +
                           cv3.invmass * pow(length(q3),2) +
                           cv4.invmass * pow(length(q4),2));
        const float nom = -sqrt(clamp(1 - pow(d, 2), 0.0f, 1.0f)) * (clamped_acos(d) - clothEdge.initialDihedralAngle);
        const float factor = nom / denom;

        deltaP1 += params.k_bend * cv1.invmass * factor * q1;
        deltaP2 += params.k_bend * cv2.invmass * factor * q2;
        const float3 deltaP3 = params.k_bend * cv3.invmass * factor * q3;
        const float3 deltaP4 = params.k_bend * cv4.invmass * factor * q4;

//        if (ID == 2) {
//            printf("verts=[%i, %i, %i, %i]", v1ID, v2ID, v3ID, v4ID);
//            printf("\n");
//            printf("initialDihedralAngle=%f", clothEdge.initialDihedralAngle);
//            printf("       dihedralAngle=%f", clamped_acos(d));
//            printf("\n");
//            printf("p1="); DBG_FLOAT3(p1);
//            printf("p2="); DBG_FLOAT3(p2);
//            printf("p3="); DBG_FLOAT3(p3);
//            printf("p4="); DBG_FLOAT3(p4);
//            printf("\n");
//            printf("n1="); DBG_FLOAT3(n1);
//            printf("n2="); DBG_FLOAT3(n2);
//            printf("\n");
//            printf("q1="); DBG_FLOAT3(q1);
//            printf("q2="); DBG_FLOAT3(q2);
//            printf("q3="); DBG_FLOAT3(q3);
//            printf("\n");
//            printf("d=%f, nom=%f, denom=%f, factor=%f", d, nom, denom, factor);
//            printf("\n");
//            printf("deltaP1="); DBG_FLOAT3(deltaP1);
//            printf("deltaP2="); DBG_FLOAT3(deltaP2);
//            printf("deltaP3="); DBG_FLOAT3(deltaP3);
//            printf("deltaP4="); DBG_FLOAT3(deltaP4);
//            printf("\n\n");
//        }
        cmpwise_atomic_add_global_float3(&(positionCorrections[v3ID]), deltaP3);
        cmpwise_atomic_add_global_float3(&(positionCorrections[v4ID]), deltaP4);
    }
    
    cmpwise_atomic_add_global_float3(&(positionCorrections[v1ID]), deltaP1);
    cmpwise_atomic_add_global_float3(&(positionCorrections[v2ID]), deltaP2);
}

/**
 *  For every vertex
 */
__kernel void correct_predictions(__global float3      *positionCorrections,   // 0
                               __global float3      *predictedPositions) {  // 1
//    printf("position=[%f, %f, %f]\n correction=[%f, %f, %f]\n",
//           positionCorrections[ID].x, positionCorrections[ID].y, positionCorrections[ID].z,
//           predictedPositions[ID].x, predictedPositions[ID].y, predictedPositions[ID].z);
    
    predictedPositions[ID] += positionCorrections[ID];
    
    // don't forget to reset position correction when we're done!
    positionCorrections[ID] = Float3(0.0f, 0.0f, 0.0f);
}

float calc_dihedral_angle(const float3 p1,
                          const float3 p2,
                          const float3 p3,
                          const float3 p4) {
    const float3 n1 = normalize(Cross(p2 - p1, p3 - p1));
    const float3 n2 = normalize(Cross(p2 - p1, p4 - p1));
    return clamped_acos(dot(n1, n2));
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

void cmpwise_atomic_add_global_float3(volatile __global float3 *addr, float3 val) {
    volatile __global float *faddr = (__global float *) addr;
    
    atomic_add_global_float(&(faddr[0]), val.x);
    atomic_add_global_float(&(faddr[1]), val.y);
    atomic_add_global_float(&(faddr[2]), val.z);
}
