// mesh.h
#ifndef MESH_H
#define MESH_H

#include "primitiv.h"   // GrVertex, MAX_CUBE_VERTICES, и т.п.

extern int triangles_drawn;

typedef struct {
    int num_vertices;
    int num_faces;
    int num_texture_coordinates;
    int num_normals;
    
    int* indices_vertices;
    int* indices_texture_coordinates;
    int* indices_normals;
    
    GrVertex* grVertices;
    float* normals;
    float* texture_coordinates;
} Mesh;

static inline float eval_plane_value(GrVertex v, float* plane);
int ClipTriangleByPlane(
    GrVertex* output,                    // массив для выходных треугольников (макс 8*3 вершин)
    GrVertex* v1,                  // вершина 1
    GrVertex* v2,                  // вершина 2
    GrVertex* v3,                  // вершина 3
    float* plane,                  // [a, b, c, d]
    int max_output_vertices              // размер выходного буфера (должен быть >= 24)
);
void ExtractFrustumPlanes(FrustumPlanes* planes, float* proj_matrix);
int ClipTriangleByFrustum(
    GrVertex* output,                    // буфер для выходных треугольников
    GrVertex* v1,                  // вершина 1
    GrVertex* v2,                  // вершина 2
    GrVertex* v3,                  // вершина 3
    FrustumPlanes* planes,         // плоскости frustum
    int max_output_vertices              // размер буфера (должен быть >= 24)
);
static void CopyVertex3D(GrVertex* dest, GrVertex* src);
static void InterpolateVertex3D(GrVertex* out, 
                                GrVertex* v1, 
                                GrVertex* v2, 
                                float t);

Mesh* LoadMeshFromOBJ(const char* filename);
void UnloadMesh(Mesh* mesh);
void DrawMesh(
    Mesh* mesh, TextureSlot* texture,
    float pos_x, float pos_y, float pos_z,
    float rot_x, float rot_y, float rot_z,
    float scale_x, float scale_y, float scale_z
);


#endif
