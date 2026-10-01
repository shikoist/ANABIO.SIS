// mesh.h
#ifndef MESH_H
#define MESH_H

#include "primitiv.h"   // GrVertex, MAX_CUBE_VERTICES, and so on

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

float eval_plane_value(GrVertex v, float* plane);
int ClipTriangleByPlaneOld(GrVertex* output, GrVertex* v1, GrVertex* v2, GrVertex* v3, float* plane, int max_output_vertices);
int ClipPolygonByPlane(GrVertex* output, GrVertex* input, int num_verts, float* plane, int max_output);
void ExtractFrustumPlanes(FrustumPlanes* planes, float* proj_matrix);
int ClipTriangleByFrustum(GrVertex* output, GrVertex* v1, GrVertex* v2, GrVertex* v3, FrustumPlanes* planes, int max_output_vertices);
void CopyVertex3D(GrVertex* dest, GrVertex* src);
void InterpolateVertex3D(GrVertex* out, GrVertex* v1, GrVertex* v2, float t);
Mesh* LoadMeshFromOBJ(const char* filename);
void UnloadMesh(Mesh* mesh);
void DrawMeshWithClip(Mesh* mesh, Texture *texture, float pos_x, float pos_y, float pos_z, float rot_x, float rot_y, float rot_z, float scale_x, float scale_y, float scale_z);
void DrawMeshWithDrop(Mesh* mesh, Texture *texture, float pos_x, float pos_y, float pos_z, float rot_x, float rot_y, float rot_z, float scale_x, float scale_y, float scale_z);

#endif
