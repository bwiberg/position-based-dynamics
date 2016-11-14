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
 * OpenCL representation of a mesh. Does NOT match the memory
 * layout of the Mesh struct in src/rendering/Mesh.hpp
 */
typedef struct def_Mesh {
    Vertex *vertices;
    Triangle *triangles;
} Mesh;
