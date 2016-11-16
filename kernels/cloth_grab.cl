typedef struct def_Vertex {
    float position[3];
    float normal[3];
    float texCoord[2];
    float color[4];
} Vertex;

#define ID get_global_id(0)

inline float3 Float3(float x, float y, float z) {
    float3 vec;
    vec.x = x;
    vec.y = y;
    vec.z = z;
    return vec;
}

#define POSITION(vertex) Float3(vertex.position[0], vertex.position[1], vertex.position[2])


