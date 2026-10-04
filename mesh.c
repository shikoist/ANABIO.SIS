// mesh.c - загрузка массивов вертексов из файлов

#define MAX_VERTICES 512
#define MAX_FACES 512   // тройки индексов, т.е. треугольники * 3

#define MAX_CLIP_VERTICES 16 * 3     // для клипинга
#define MAX_OUTPUT_TRIANGLES 8 * 3

// Функция оценки расстояния до плоскости
// #define EVAL_PLANE(v) (plane[0] * (v).x + plane[1] * (v).y + plane[2] * (v).z + plane[3])

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <MATH.H>
#include "matrix.h"
#include "mesh.h"
#include "glide.h"

static GrVertex viewVerts[MAX_VERTICES];
static GrVertex tempVtx[MAX_VERTICES];
static GrVertex screen[3]; // Buffer for only one triangle
int triangles_drawn;

// On which side is the vertex located
float eval_plane_value(GrVertex v, float* plane) {
    return plane[0] * v.x + plane[1] * v.y + plane[2] * v.z + plane[3];
}

// Clipping triangle against a generic plane
int ClipTriangleByPlaneOld(GrVertex* output, GrVertex* v1, GrVertex* v2, GrVertex* v3, float* plane, int max_output_vertices) {
    GrVertex verts[MAX_CLIP_VERTICES];
    GrVertex input[3];
    GrVertex outputVerts[MAX_CLIP_VERTICES];
    int num_verts = 3;
    int num_output = 0;
    int tri_count = 0;

    // Для каждой вершины запоминаем, с какой стороны плоскости она находится
    float dist[3];
    int inside[3];
    int inside_count = 0;

    int i = 0;
    
    // Конвертируем входные вершины
    memcpy(&input[0], v1, sizeof(GrVertex));
    memcpy(&input[1], v2, sizeof(GrVertex));
    memcpy(&input[2], v3, sizeof(GrVertex));
    
    // Копируем во временный массив
    memcpy(verts, input, 3 * sizeof(GrVertex));
    
    // Вычисляем вертиксы снаружи и внутри плоскости
    for (i = 0; i < 3; i++) {
        dist[i] = eval_plane_value(input[i], plane);
        inside[i] = (dist[i] >= 0);  // >= 0 значит внутри
        if (inside[i]) inside_count++;
    }
    
    // Если все вершины внутри - возвращаем исходный треугольник
    if (inside_count == 3) {
        if (max_output_vertices >= 3) {
            memcpy(output, v1, sizeof(GrVertex));
            memcpy(output + 1, v2, sizeof(GrVertex));
            memcpy(output + 2, v3, sizeof(GrVertex));
            return 1;
        }
        return 0;
    }
    
    // Если все вершины снаружи - треугольник невидим
    if (inside_count == 0) {
        return 0;
    }
    
    // Одна вершина внутри, две снаружи
    //      . |
    //        |  .
    //      . |
    if (inside_count == 1) {
        GrVertex* v_inside = NULL;
        GrVertex* v_out1 = NULL;
        GrVertex* v_out2 = NULL;

        GrVertex i1, i2;

        float t1, t2;
        float t_out1, t_out2;

        int out_idx[2] = {-1, -1};
        int in_idx = -1;
        
        for (i = 0; i < 3; i++) {
            if (inside[i]) {
                in_idx = i;
            } else {
                if (out_idx[0] == -1) out_idx[0] = i;
                else out_idx[1] = i;
            }
        }
        
        // Вычисляем точки пересечения ребер с плоскостью
        t1 = dist[in_idx] / (dist[in_idx] - dist[out_idx[0]]);
        t2 = dist[in_idx] / (dist[in_idx] - dist[out_idx[1]]);
        
        // Вычисляем точки пересечения
        InterpolateVertex3D(&i1, &input[in_idx], &input[out_idx[0]], t1);
        InterpolateVertex3D(&i2, &input[in_idx], &input[out_idx[1]], t2);
        
        // Создаем один треугольник: v_inside, i1, i2
        if (max_output_vertices >= 3) {
            memcpy(output, &input[in_idx], sizeof(GrVertex));
            memcpy(output + 1, &i1, sizeof(GrVertex));
            memcpy(output + 2, &i2, sizeof(GrVertex));
            return 1;
        }
        return 0;
    }
    
    // Две вершины внутри, одна снаружи
    if (inside_count == 2) {
        GrVertex* v_in1 = NULL;
        GrVertex* v_in2 = NULL;
        GrVertex* v_out = NULL;
        GrVertex i1, i2;
        int out_idx = -1;
        int in_idx[2] = {-1, -1};
        float t1, t2;
        
        for (i = 0; i < 3; i++) {
            if (inside[i]) {
                if (v_in1 == NULL) v_in1 = &input[i];
                else v_in2 = &input[i];
            } else {
                v_out = &input[i];
            }
        }
        
        // Находим индексы
        for (i = 0; i < 3; i++) {
            if (inside[i]) {
                if (in_idx[0] == -1) in_idx[0] = i;
                else in_idx[1] = i;
            } else {
                out_idx = i;
            }
        }
        
        // Вычисляем точки пересечения
        t1 = dist[in_idx[0]] / (dist[in_idx[0]] - dist[out_idx]);
        t2 = dist[in_idx[1]] / (dist[in_idx[1]] - dist[out_idx]);
        
        InterpolateVertex3D(&i1, &input[in_idx[0]], &input[out_idx], t1);
        InterpolateVertex3D(&i2, &input[in_idx[1]], &input[out_idx], t2);
        
        // Создаем два треугольника
        if (max_output_vertices >= 6) {
            // Треугольник 1: v_in1, v_in2, i1
            memcpy(output, &input[in_idx[0]], sizeof(GrVertex));
            memcpy(output + 1, &input[in_idx[1]], sizeof(GrVertex));
            memcpy(output + 2, &i1, sizeof(GrVertex));
            
            // Треугольник 2: v_in2, i2, i1
            memcpy(output + 3, &input[in_idx[1]], sizeof(GrVertex));
            memcpy(output + 4, &i2, sizeof(GrVertex));
            memcpy(output + 5, &i1, sizeof(GrVertex));
            
            return 2;
        }
        return 0;
    }
    
    return 0;
}

// Clip any polygon to a plane
int ClipPolygonByPlane(GrVertex* output, GrVertex* input, int num_verts, float* plane, int max_output) {
    int i, out_n = 0;
    float d_prev, d_cur;

    // проверка входа и выхода
    // если меньше трёх (не треугольник), возвращаем 0
    if (num_verts < 3 || max_output < 3) return 0; 

    // Знак говорит, с какой стороны плоскости вершина:
    // >= 0 — внутри (та сторона, куда смотрит нормаль),
    // < 0 — снаружи.
    d_prev = eval_plane_value(input[num_verts - 1], plane);

    // Идём по вершинам полигона. 
    // На каждой итерации обрабатываем ребро 
    // (input[i-1], input[i]), где input[i-1] — 
    // это та вершина, на которой мы остановились в прошлый раз 
    // (её расстояние хранится в d_prev). 
    // После обработки сдвигаем d_prev = d_cur, 
    // чтобы следующая итерация работала с новым ребром.
    for (i = 0; i < num_verts; i++) {
        d_cur = eval_plane_value(input[i], plane);

        // Если вершина внутри
        if (d_cur >= 0.0f) {
            
            if (d_prev < 0.0f) {
                // ...и предыдущая снаружи — добавляем пересечение
                float t = d_prev / (d_prev - d_cur);
                if (out_n >= max_output) return 0;
                InterpolateVertex3D(&output[out_n++], 
                                    &input[(i + num_verts - 1) % num_verts],
                                    &input[i], t);
            }
            if (out_n >= max_output) return 0;
            memcpy(&output[out_n++], &input[i], sizeof(GrVertex));
        } else {
            // Текущая снаружи
            if (d_prev >= 0.0f) {
                // ...а предыдущая внутри — добавляем пересечение
                float t = d_prev / (d_prev - d_cur);
                if (out_n >= max_output) return 0;
                InterpolateVertex3D(&output[out_n++],
                                    &input[(i + num_verts - 1) % num_verts],
                                    &input[i], t);
            }
        }
        d_prev = d_cur;
    }
    return out_n;
}

// Drop of the triangle on an arbitrary plane
int DropTriangleByPlane(GrVertex* v1, GrVertex* v2, GrVertex* v3, float* plane) {
    // Собираем массив из входных вершин
    GrVertex input[3];
    
    // Для каждой вершины запоминаем, с какой стороны плоскости она находится
    float dist[3];
    int inside[3];
    int inside_count = 0;

    int i = 0;
    
    // Конвертируем входные вершины
    memcpy(&input[0], v1, sizeof(GrVertex));
    memcpy(&input[1], v2, sizeof(GrVertex));
    memcpy(&input[2], v3, sizeof(GrVertex));
    
    // Вычисляем вертиксы снаружи и внутри плоскости
    for (i = 0; i < 3; i++) {
        dist[i] = eval_plane_value(input[i], plane);
        inside[i] = (dist[i] >= 0);  // >= 0 значит внутри
        if (inside[i]) inside_count++;
    }
    
    // Если все вершины внутри - возвращаем исходный треугольник
    if (inside_count == 3) {
        return 1;
    }
    
    // Если все вершины снаружи - треугольник невидим
    if (inside_count >= 0 && inside_count <= 2) {
        return 0;
    }
    
    return 0;
}

// Full frustum clipping triangle
int ClipTriangleByFrustumOld(GrVertex* output, GrVertex* v1, GrVertex* v2, GrVertex* v3, FrustumPlanes* planes, int max_output_vertices) {
    // Применяем клиппинг по каждой плоскости
    float planes_list[6][4];
    GrVertex temp_buffer1[24];  // достаточно для 8 треугольников
    GrVertex temp_buffer2[24];
    int num_tris = 1;
    int current_buffer = 0;
    int i, p;
    memcpy(planes_list[0], planes->near_plane, 4 * sizeof(float));
    memcpy(planes_list[1], planes->left_plane, 4 * sizeof(float));
    memcpy(planes_list[2], planes->right_plane, 4 * sizeof(float));
    memcpy(planes_list[3], planes->bottom_plane, 4 * sizeof(float));
    memcpy(planes_list[4], planes->top_plane, 4 * sizeof(float));
    memcpy(planes_list[5], planes->far_plane, 4 * sizeof(float));
    // Начинаем с одного треугольника
    memcpy(temp_buffer1, v1, sizeof(GrVertex));
    memcpy(temp_buffer1 + 1, v2, sizeof(GrVertex));
    memcpy(temp_buffer1 + 2, v3, sizeof(GrVertex));
    for (p = 0; p < 6; p++) {
    //for (p = 0; p < 1; p++) {
        //if (p != 5) {
        if (p == 5) {
        //if (1) {
            int input_tris = num_tris;
            int out_count = 0;
            int t;
            //num_tris = 0;
            for (t = 0; t < input_tris; t++) {
                GrVertex* src = (current_buffer == 0) ? temp_buffer1 : temp_buffer2;
                GrVertex* dst = (current_buffer == 0) ? temp_buffer2 : temp_buffer1;
                int idx = t * 3;
                int num_clipped;
                // Клиппим треугольник по текущей плоскости
                num_clipped = ClipTriangleByPlaneOld(
                    //dst + num_tris * 3,
                    dst + out_count * 3,
                    &src[idx], &src[idx+1], &src[idx+2],
                    planes_list[p],
                    //24 - num_tris * 3
                    24 - out_count * 3
                );
                out_count += num_clipped;
                // Если слишком много треугольников - прерываем
                if (out_count > MAX_OUTPUT_TRIANGLES) {
                    out_count = MAX_OUTPUT_TRIANGLES;
                    break;
                }
            }
            // Переключаем буфер для следующей плоскости
            num_tris = out_count;
            current_buffer = 1 - current_buffer;
            // Если нет треугольников - выходим
            if (num_tris == 0) break;
        }
    }
    //return 0;
    // Копируем результат в выходной буфер
    if (num_tris > 0) {
        GrVertex* src = (current_buffer == 0) ? temp_buffer1 : temp_buffer2;
        int total_verts = num_tris * 3;
        if (total_verts <= max_output_vertices) {
            memcpy(output, src, total_verts * sizeof(GrVertex));
        } else {
            num_tris = 0;
        }
    }
    return num_tris;
}

int ClipTriangleByFrustum(GrVertex* output, GrVertex* v1, GrVertex* v2, GrVertex* v3, FrustumPlanes* planes, int max_output_vertices) {
    // Достаточно буфера на 3 вершины на входе
    // и 4 на выходе за одну плоскость (треугольник + 1 точка = 4)
    // Но для 6 плоскостей накапливается — держим 3+3+1 с запасом.
    GrVertex buf_a[8], buf_b[8];
    GrVertex* src = buf_a;
    GrVertex* dst = buf_b;
    GrVertex* tmp;
    int num = 3;
    float* planes_list[6];
    int p;
    int num_tris;

    // near, left, right, bottom, top, far
    planes_list[0] = planes->near_plane;
    planes_list[1] = planes->left_plane;
    planes_list[2] = planes->right_plane;
    planes_list[3] = planes->bottom_plane;
    planes_list[4] = planes->top_plane;
    planes_list[5] = planes->far_plane;

    memcpy(buf_a, v1, sizeof(GrVertex));
    memcpy(buf_a + 1, v2, sizeof(GrVertex));
    memcpy(buf_a + 2, v3, sizeof(GrVertex));

    for (p = 0; p < 6; p++) {
        //if (p == 0)
        if (1)
        {
            num = ClipPolygonByPlane(dst, src, num, planes_list[p], 8);
            if (num < 3) return 0;      // всё снаружи — треугольник невидим
            // swap
            tmp = src; src = dst; dst = tmp;
        }
    }

    // После 6 итераций результат лежит в src (мы свапнули в конце)
    // Триангуляция веером: (0,1,2), (0,2,3), ...
    num_tris = num - 2;
    if (num_tris * 3 > max_output_vertices) return 0;

    for (p = 0; p < num_tris; p++) {
        memcpy(output + p * 3 + 0, &src[0],   sizeof(GrVertex));
        memcpy(output + p * 3 + 1, &src[p+1], sizeof(GrVertex));
        memcpy(output + p * 3 + 2, &src[p+2], sizeof(GrVertex));
    }
    return num_tris;
}

// Triangle drop by frustum
int DropTriangleByFrustum(GrVertex* v1, GrVertex* v2, GrVertex* v3, FrustumPlanes* planes) {
    // Применяем клиппинг по каждой плоскости
    float planes_list[6][4];

    int p;
    int dropped = 0;
    
    memcpy(planes_list[0], planes->near_plane, 4 * sizeof(float));
    memcpy(planes_list[1], planes->left_plane, 4 * sizeof(float));
    memcpy(planes_list[2], planes->right_plane, 4 * sizeof(float));
    memcpy(planes_list[3], planes->bottom_plane, 4 * sizeof(float));
    memcpy(planes_list[4], planes->top_plane, 4 * sizeof(float));
    memcpy(planes_list[5], planes->far_plane, 4 * sizeof(float));
    
    for (p = 0; p < 6; p++) {
        dropped += DropTriangleByPlane(
            v1, v2, v3,
            planes_list[p]
        );
    }
    
    return dropped;
}

// Copying the vertex
void CopyVertex3D(GrVertex* dest, GrVertex* src) {
    memcpy(dest, src, sizeof(GrVertex));
}

// Interpolation of the vertex for clipping
void InterpolateVertex3D(GrVertex* out, GrVertex* v1, GrVertex* v2, float t) {
    int i;

    out->x = v1->x + t * (v2->x - v1->x);
    out->y = v1->y + t * (v2->y - v1->y);
    out->z = v1->z + t * (v2->z - v1->z);
    out->r = v1->r + t * (v2->r - v1->r);
    out->g = v1->g + t * (v2->g - v1->g);
    out->b = v1->b + t * (v2->b - v1->b);
    out->a = v1->a + t * (v2->a - v1->a);
    out->oow = v1->oow + t * (v2->oow - v1->oow);
    out->ooz = v1->ooz + t * (v2->ooz - v1->ooz);
    
    for (i = 0; i < GLIDE_NUM_TMU; i++) {
        out->tmuvtx[i].sow = v1->tmuvtx[i].sow + t * (v2->tmuvtx[i].sow - v1->tmuvtx[i].sow);
        out->tmuvtx[i].tow = v1->tmuvtx[i].tow + t * (v2->tmuvtx[i].tow - v1->tmuvtx[i].tow);
    }
}

Mesh* LoadMeshFromOBJ(const char* filename) {
    Mesh* mesh;
    // счётчики для статических буферов
    int num_vertices = 0, num_faces = 0, num_texture_coordinates = 0, num_normals = 0;
    int counter_v = 0, counter_f = 0, counter_vt = 0, counter_n = 0;
    char line[256];
    int i;
    int max_final_vertices;

    int counter_pos = 0;
    int counter_uv  = 0;
    int counter_nrm = 0;
    int final_idx = 0;

    float* obj_positions = NULL;   // 3 float on vertex
    float* obj_uvs       = NULL;   // 2 float on UV
    float* obj_normals   = NULL;   // 3 float on normal
    
    FILE* f = fopen(filename, "r");
    if (!f) {
        printf("ERROR: Cannot open file %s\n", filename);
        free(mesh);
        return NULL;
    }

    mesh = (Mesh*)malloc(sizeof(Mesh));
    if (!mesh) {
        printf("ERROR: Failed to allocate Mesh structure\n");
        return NULL;
    }
    memset(mesh, 0, sizeof(Mesh));

    // Parsing file - first pass, just counting
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = 0;
        if (line[0] == '#' || line[0] == '\0') continue;

        // Example string: "v 1.000000 1.000000 -1.000000"
        if (strncmp(line, "v ", 2) == 0) {
            float x, y, z;
            if (sscanf(line+2, "%f %f %f", &x, &y, &z) == 3) { num_vertices++; }
        }
        // Parsing normals
        // Example string: "vn 0.000000 0.000000 0.11000"
        else if (strncmp(line, "vn ", 3) == 0) {
            float x, y, z;
            if (sscanf(line+3, "%f %f %f", &x, &y, &z) == 3) { num_normals++; }
        }
        // Parsing texture coordinates
        // Example string: "vt 0.000000 0.000000"
        else if (strncmp(line, "vt ", 3) == 0) {
            float u, v;
            if (sscanf(line+3, "%f %f", &u, &v) == 2) { num_texture_coordinates++; }
        }
        // Parsing faces
        // Example string: "f 2 3 1" - only if no UV coordinates
        // If there is, then
        // f 4/1 17/2 5/3
        // 4, 17, 5 — these are vertex indices (from list v). The yare numbered from 1.
        // 1, 2, 3 — these are texture coordinate indices (from the list vt).
        //  They are also numbered starting from 1.
        else if (strncmp(line, "f ", 2) == 0) {
            int v1, v2, v3, vt1, vt2, vt3, vn1, vn2, vn3;
            // Trying to read a triplet of vertices for triangle construction
            if (sscanf(line+2, "%d %d %d",
                    &v1, &v2, &v3) == 3) { num_faces++; }
            else if (sscanf(line+2, "%d/%d %d/%d %d/%d",
                    &v1, &vt1, &v2, &vt2, &v3, &vt3) == 6) { num_faces++; }
            else if (sscanf(line+2, "%d/%d/%d %d/%d/%d %d/%d/%d",
                    &v1, &vt1, &vn1, &v2, &vt2, &vn2, &v3, &vt3, &vn3) == 9) { num_faces++; }
            else {
                // Failed to read 9 numbers – skip or report error
                printf("WARNING: unsupported face format: %s\n", line);
            }
        }
    }
    fclose(f);

    printf("[FILE] Calculating %s : %d vertices, %d faces, %d texture coords, %d normals\n", 
        filename, num_vertices, num_faces, num_texture_coordinates, num_normals);

    // Now we allocate exactly the memory that is needed
    // Open the file again and fill the arrays
    // According to the size of the GrVertex structure
    //mesh->grVertices = malloc(num_vertices * sizeof(GrVertex));
    //mesh->grVertices = malloc((num_vertices * sizeof(GrVertex) + 15) & ~15);

    // Now each triangle is a separate entity
    mesh->grVertices = malloc(num_faces * 3 * sizeof(GrVertex));

    // Allocating memory
    if (num_vertices > 0)
        obj_positions = malloc(num_vertices * 3 * sizeof(float));
    if (num_texture_coordinates > 0)
        obj_uvs = malloc(num_texture_coordinates * 2 * sizeof(float));
    if (num_normals > 0)
        obj_normals = malloc(num_normals * 3 * sizeof(float));

    // Final vertex count = face count * 3
    max_final_vertices = num_faces * 3;

    // Initialize all vertices
    for (i = 0; i < max_final_vertices; i++) {
        memset(&mesh->grVertices[i], 0, sizeof(GrVertex));
        mesh->grVertices[i].oow = 1.0f;  // Important!
        // tmuvtx is already built into the structure, no need to allocate separately
    }

    // By 3 int for each triangle
    mesh->indices_vertices = malloc(max_final_vertices * sizeof(int));
    if (num_normals > 0) mesh->indices_normals = malloc(max_final_vertices * sizeof(int));
    if (num_texture_coordinates > 0) mesh->indices_texture_coordinates = malloc(max_final_vertices * sizeof(int));
    if (num_texture_coordinates > 0) mesh->texture_coordinates = malloc(num_texture_coordinates * 2 * sizeof(float));
    if (num_normals > 0) mesh->normals = malloc(num_normals * 3 * sizeof(float));

    // Second pass - filling arrays
    counter_pos = 0;
    counter_uv  = 0;
    counter_nrm = 0;
    final_idx = 0;

    f = fopen(filename, "r");
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = 0;
        if (line[0] == '#' || line[0] == '\0') continue;

        // --- v ---
        if (strncmp(line, "v ", 2) == 0) {
            float x, y, z;
            if (sscanf(line + 2, "%f %f %f", &x, &y, &z) == 3) {
                obj_positions[counter_pos * 3 + 0] = x;
                obj_positions[counter_pos * 3 + 1] = y;
                obj_positions[counter_pos * 3 + 2] = z;
                counter_pos++;
            }
        }
        // --- vn ---
        else if (strncmp(line, "vn ", 3) == 0) {
            float x, y, z;
            if (sscanf(line + 3, "%f %f %f", &x, &y, &z) == 3) {
                obj_normals[counter_nrm * 3 + 0] = x;
                obj_normals[counter_nrm * 3 + 1] = y;
                obj_normals[counter_nrm * 3 + 2] = z;
                counter_nrm++;
            }
        }
        // --- vt ---
        else if (strncmp(line, "vt ", 3) == 0) {
            float u, v;
            if (sscanf(line + 3, "%f %f", &u, &v) == 2) {
                obj_uvs[counter_uv * 2 + 0] = u;
                obj_uvs[counter_uv * 2 + 1] = v;
                counter_uv++;
            }
        }
        // --- f ---
        else if (strncmp(line, "f ", 2) == 0) {
            int v1, v2, v3;
            int vt1, vt2, vt3;
            int vn1, vn2, vn3;
            int v_idx[3];
            int vt_idx[3];
            int vn_idx[3];
            int k;

            int has_uv = 0;
            int has_n  = 0;

            // Three possible face formats
            if (sscanf(line + 2, "%d %d %d", &v1, &v2, &v3) == 3) {
                // format "f v v v"
                has_uv = 0;
                has_n  = 0;
            }
            else if (sscanf(line + 2, "%d/%d %d/%d %d/%d",
                            &v1, &vt1, &v2, &vt2, &v3, &vt3) == 6) {
                // format "f v/vt v/vt v/vt"
                has_uv = 1;
                has_n  = 0;
            }
            else if (sscanf(line + 2, "%d/%d/%d %d/%d/%d %d/%d/%d",
                            &v1, &vt1, &vn1, &v2, &vt2, &vn2, &v3, &vt3, &vn3) == 9) {
                // format "f v/vt/vn v/vt/vn v/vt/vn"
                has_uv = 1;
                has_n  = 1;
            }
            else {
                printf("WARNING: unsupported face format: %s\n", line);
                continue;
            }

            // Массив из трёх вершин текущего фейса
            v_idx[0] = v1;
            v_idx[1] = v2;
            v_idx[2] = v3;
            vt_idx[0] = vt1;
            vt_idx[1] = vt2;
            vt_idx[2] = vt3;
            vn_idx[0] = vn1;
            vn_idx[1] = vn2;
            vn_idx[2] = vn3;

            for (k = 0; k < 3; k++) {
                int vi = v_idx[k];
                GrVertex* nv;

                // Index position check
                if (vi < 1 || vi > num_vertices) {
                    printf("WARNING: face references v[%d], but only %d vertices exist\n",
                        vi, num_vertices);
                    continue;
                }

                // Create a new vertex
                nv = &mesh->grVertices[final_idx];
                memset(nv, 0, sizeof(GrVertex));

                // --- position ---
                nv->x = obj_positions[(vi - 1) * 3 + 0];
                nv->y = obj_positions[(vi - 1) * 3 + 1];
                nv->z = obj_positions[(vi - 1) * 3 + 2];

                // --- default color ---
                nv->r = 1.0f;
                nv->g = 1.0f;
                nv->b = 1.0f;
                nv->a = 1.0f;

                // --- W-coordinates ---
                nv->oow = 1.0f;
                nv->ooz = 1.0f;

                // --- UV ---
                if (has_uv) {
                    int ti = vt_idx[k];
                    if (ti >= 1 && ti <= num_texture_coordinates) {
                        float u = obj_uvs[(ti - 1) * 2 + 0];
                        float v = obj_uvs[(ti - 1) * 2 + 1];
                        nv->tmuvtx[0].sow = u; // Blender flips UV fix
                        nv->tmuvtx[0].tow = 1 - v; // still mirrored on axe X
                        nv->tmuvtx[0].oow = 1.0f;
                    } else {
                        printf("WARNING: face references vt[%d], but only %d exist\n",
                            ti, num_texture_coordinates);
                    }
                }

                // --- We write the index in indices_vertices ---
                mesh->indices_vertices[final_idx] = final_idx;
                final_idx++;
            }
        }
    }
    fclose(f);

    mesh->num_vertices = final_idx;
    mesh->num_faces = num_faces;
    mesh->num_texture_coordinates = num_texture_coordinates;
    mesh->num_normals = num_normals;

    if (obj_positions) free(obj_positions);
    if (obj_uvs)       free(obj_uvs);
    if (obj_normals)   free(obj_normals);

    if (max_final_vertices > MAX_VERTICES) {
        printf("ERROR: %s has %d faces (= %d vertices), MAX_VERTICES is %d\n",
            filename, num_faces, max_final_vertices, MAX_VERTICES);
        free(mesh);
        return NULL;
    }

    printf("[FILE] Model %s is loaded: %d vertices, %d faces, %d texture coords, %d normals\n", 
        filename, mesh->num_vertices, mesh->num_faces, mesh->num_texture_coordinates, mesh->num_normals);

    // Память освобождаем в UnloadMesh()
    //free(mesh->grVertices);

    return mesh;
}

void UnloadMesh(Mesh* mesh)
{
    if (mesh->grVertices)
    {
        free(mesh->grVertices);
        mesh->grVertices = NULL;
    }
    
    if (mesh->indices_vertices) {
        free(mesh->indices_vertices);
        mesh->indices_vertices = NULL;
    }
    
    if (mesh->indices_normals) {
        free(mesh->indices_normals);
        mesh->indices_normals = NULL;
    }
    
    if (mesh->indices_texture_coordinates) {
        free(mesh->indices_texture_coordinates);
        mesh->indices_texture_coordinates = NULL;
    }
    
    if (mesh->normals) {
        free(mesh->normals);
        mesh->normals = NULL;
    }
    
    if (mesh->texture_coordinates) {
        free(mesh->texture_coordinates);
        mesh->texture_coordinates = NULL;
    }

    free(mesh);
    mesh = NULL;
}

// Old version
void DrawMeshWithClipOld(Mesh* mesh, Texture* texture, float pos_x, float pos_y, float pos_z, float rot_x, float rot_y, float rot_z, float scale_x, float scale_y, float scale_z) {
    GrState grState;
    int i, j, t;
    float model[16];
    float proj[16];
    float mvp[16];
    GrVertex tempVtx[MAX_VERTICES];
    GrVertex clipSpaceVerts[MAX_VERTICES];  // Вершины в Clip Space (до деления)
    GrVertex clipped_verts[24];
    int num_tris;
    if (!mesh || !mesh->grVertices) return;
    grGlideGetState(&grState);
    // Настройка текстурного состояния (как было)
    if (camera.wireframe_mode == 1) {
        grTexCombine(GR_TMU0,
             GR_COMBINE_FUNCTION_LOCAL,
             GR_COMBINE_FACTOR_LOCAL,
             GR_COMBINE_FUNCTION_LOCAL,
             GR_COMBINE_OTHER_NONE,
             FXFALSE, FXFALSE);
        grColorCombine(
            GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_ONE,
            GR_COMBINE_LOCAL_NONE, GR_COMBINE_OTHER_ITERATED,
            FXFALSE );
    }
    else {
        grTexCombine(
            GR_TMU0,
            GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE,
            GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE,
            FXFALSE, FXFALSE);
        grColorCombine(
            GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_LOCAL,
            GR_COMBINE_LOCAL_CONSTANT, GR_COMBINE_OTHER_TEXTURE,
            FXFALSE );
        grTexSource(texture->tmu,
            texture->baseAddr,
            GR_MIPMAPLEVELMASK_BOTH,
            &texture->grTexInfo);
    }
    // Строим матрицы
    MatrixIdentity(model);
    MatrixEulerRotation(model, rot_x, rot_y, rot_z);
    MatrixTranslation(model, pos_x, pos_y, pos_z);
    MatrixScale(model, scale_x, scale_y, scale_z);
    //return;
    //MatrixIdentity(proj);
    // MatrixProjection(proj,
    //     camera.fov,
    //     camera.aspect,
    //     camera.near_clip,
    //     camera.far_clip);
    // Извлекаем плоскости frustum из матрицы проекции
    //ExtractFrustumPlanes(&frustum, proj);
    MatrixIdentity(mvp);
    MatrixMultiply(mvp, mvp, model);
    MatrixMultiply(mvp, mvp, view);
    MatrixMultiply(mvp, mvp, proj);
    // Копируем вершины модели
    memcpy(tempVtx, mesh->grVertices, mesh->num_vertices * sizeof(GrVertex));
    // ПРИМЕНЯЕМ MVP, НО НЕ ДЕЛАЕМ ПЕРСПЕКТИВНОЕ ДЕЛЕНИЕ!
    for (i = 0; i < mesh->num_vertices; i++) {
        // Сохраняем оригинальные текстурные координаты
        float orig_s = tempVtx[i].tmuvtx[0].sow;
        float orig_t = tempVtx[i].tmuvtx[0].tow;
        // Применяем матрицу (это даст нам Clip Space координаты)
        ApplyMatrix(&tempVtx[i], mvp);
        // Сохраняем в отдельный буфер для клиппинга
        memcpy(&clipSpaceVerts[i], &tempVtx[i], sizeof(GrVertex));
        // Восстанавливаем оригинальные текстурные координаты (до умножения на oow)
        // Они нам понадобятся для клиппинга
        clipSpaceVerts[i].tmuvtx[0].sow = orig_s;
        clipSpaceVerts[i].tmuvtx[0].tow = orig_t;
    }
    //return;
    // Рисуем треугольники с клиппингом в Clip Space
    for (i = 0; i < mesh->num_faces * 3; i += 3) {
        int idx1 = mesh->indices_vertices[i];
        int idx2 = mesh->indices_vertices[i+1];
        int idx3 = mesh->indices_vertices[i+2];
        GrVertex* v1 = &clipSpaceVerts[idx1];
        GrVertex* v2 = &clipSpaceVerts[idx2];
        GrVertex* v3 = &clipSpaceVerts[idx3];
        // Проверка: все ли вершины за near plane?
        if (v1->z < camera.near_clip && 
            v2->z < camera.near_clip && 
            v3->z < camera.near_clip) {
            continue; // Полностью невидим
        }
        //return;
        // Клиппинг по всем плоскостям frustum (в Clip Space)
        num_tris = ClipTriangleByFrustum(
            clipped_verts,
            v1, v2, v3,
            &frustum,
            24
        );
        //return;
        if (num_tris > 0) {
            // Проверяем, почему треугольник отброшен
            printf("Tri clipped: v1=(%.2f,%.2f,%.2f,%.2f) v2=(%.2f,%.2f,%.2f,%.2f) v3=(%.2f,%.2f,%.2f,%.2f)\n",
                v1->x, v1->y, v1->z, v1->oow,
                v2->x, v2->y, v2->z, v2->oow,
                v3->x, v3->y, v3->z, v3->oow);
        }
        //return;
        // Рендерим полученные треугольники
        for (t = 0; t < num_tris; t++) {
            GrVertex* tri = &clipped_verts[t * 3];
            GrVertex screen[3];
            // Для каждой вершины делаем перспективное деление и переход в экранные координаты
            for (j = 0; j < 3; j++) {
                // Сначала умножаем текстурные координаты на oow и размер текстуры
                tri[j].tmuvtx[0].sow *= tri[j].oow * texture->width;
                tri[j].tmuvtx[0].tow *= tri[j].oow * texture->height;
                // Теперь перспективное деление и экранные координаты
                VertexToScreen(&tri[j], &screen[j]);
            }
            // Проверяем валидность и рисуем
            if (screen[0].oow > 0 && screen[1].oow > 0 && screen[2].oow > 0) {
                if (camera.wireframe_mode == 0) {
                    //guAADrawTriangleWithClip(&screen[0], &screen[1], &screen[2]);
                    grDrawTriangle(&screen[0], &screen[1], &screen[2]);
                    triangles_drawn++;
                } else {
                    grConstantColorValue(0xFFFFFFFF);
                    grAADrawLine(&screen[0], &screen[1]);
                    grAADrawLine(&screen[1], &screen[2]);
                    grAADrawLine(&screen[2], &screen[0]);
                }
            }
        }
    }
    grGlideSetState(&grState);
}

void DrawMeshWithClip(Mesh* mesh, Texture* texture, float pos_x, float pos_y, float pos_z, float rot_x, float rot_y, float rot_z, float scale_x, float scale_y, float scale_z) {
    GrState grState;
    int i, j, t;
    float model[16];
    float mv[16];              // Model * View — БЕЗ проекции
    //GrVertex viewVerts[MAX_VERTICES];   // вершины в view-space
    GrVertex clipped[24];
    int num_tris;

    if (!mesh || !mesh->grVertices) return;

    grGlideGetState(&grState);

    // --- текстурное состояние (без изменений) ---
    if (camera.wireframe_mode == 1) {
        grTexCombine(GR_TMU0,
            GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_LOCAL,
            GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_OTHER_NONE,
            FXFALSE, FXFALSE);
        grColorCombine(
            GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_ONE,
            GR_COMBINE_LOCAL_NONE, GR_COMBINE_OTHER_ITERATED,
            FXFALSE);
    } else {
        grTexCombine(GR_TMU0,
            GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE,
            GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE,
            FXFALSE, FXFALSE);
        grColorCombine(
            GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_LOCAL,
            GR_COMBINE_LOCAL_CONSTANT, GR_COMBINE_OTHER_TEXTURE,
            FXFALSE);
        grTexSource(texture->tmu, texture->baseAddr,
                    GR_MIPMAPLEVELMASK_BOTH, &texture->grTexInfo);
    }

    // --- матрицы: только Model * View ---
    MatrixIdentity(model);
    MatrixEulerRotation(model, rot_x, rot_y, rot_z);
    MatrixTranslation(model, pos_x, pos_y, pos_z);
    MatrixScale(model, scale_x, scale_y, scale_z);

    MatrixIdentity(mv);
    MatrixMultiply(mv, mv, model);
    MatrixMultiply(mv, mv, view);

    // --- трансформация вершин в view-space ---
    memcpy(viewVerts, mesh->grVertices,
           mesh->num_vertices * sizeof(GrVertex));

    for (i = 0; i < mesh->num_vertices; i++) {
        // ApplyMatrix делит на W. В view-space W=1, так что деления нет.
        ApplyMatrix(&viewVerts[i], mv);
        // ApplyMatrix выставит oow = 1.0 для view-space — это нормально,
        // нам нужен oow только для текстурных координат позже.
        viewVerts[i].oow = 1.0f;
    }

    // --- клиппинг и отрисовка ---
    for (i = 0; i < mesh->num_faces * 3; i += 3) {
        int idx1 = mesh->indices_vertices[i];
        int idx2 = mesh->indices_vertices[i+1];
        int idx3 = mesh->indices_vertices[i+2];
        GrVertex* v1 = &viewVerts[idx1];
        GrVertex* v2 = &viewVerts[idx2];
        GrVertex* v3 = &viewVerts[idx3];
        
        // Быстрый reject: все три вершины за near-плоскостью
        // (в view-space near_plane имеет нормаль (0,0,1) и d=-near)
        float d1 = eval_plane_value(*v1, frustum.near_plane);
        float d2 = eval_plane_value(*v2, frustum.near_plane);
        float d3 = eval_plane_value(*v3, frustum.near_plane);
        if (d1 < 0.0f && d2 < 0.0f && d3 < 0.0f) continue;

        num_tris = ClipTriangleByFrustum(clipped, v1, v2, v3,
                                         &frustum, 24);
        if (num_tris <= 0) continue;

        // --- отрисовка каждого клипленного треугольника ---
        for (t = 0; t < num_tris; t++) {
            GrVertex* tri = &clipped[t * 3];
            GrVertex screen[3];
            int ok = 1;

            for (j = 0; j < 3; j++) {

                GrVertex clip;

                // NaN / Inf фильтр ДО любых преобразований
                if (!(tri[j].x == tri[j].x) ||   // isnan
                    !(tri[j].y == tri[j].y) ||
                    !(tri[j].z == tri[j].z) ||
                    !(tri[j].oow == tri[j].oow) ||
                    !(tri[j].tmuvtx[0].sow == tri[j].tmuvtx[0].sow) ||
                    !(tri[j].tmuvtx[0].tow == tri[j].tmuvtx[0].tow)) {
                    ok = 0; break;
                }

                // Проекция view-space -> clip-space -> screen
                // Применяем только projection, потом перспективное деление
                clip = tri[j];
                //CopyVertex3D(&clip, &tri[j]);
                ApplyMatrix(&clip, proj);   // даст x/w, y/w, z/w и oow=1/w

                // Теперь clip.x и clip.y уже поделены на w.
                // Для Glide нужны экранные координаты + oow для W-буфера.
                screen[j].x    = X_TO_SCREEN(clip.x, SCREEN_WIDTH);
                screen[j].y    = Y_TO_SCREEN(clip.y, SCREEN_HEIGHT);
                screen[j].z    = clip.z;
                screen[j].oow  = clip.oow;   // 1/w для Glide
                screen[j].ooz  = clip.ooz;
                screen[j].r    = tri[j].r;
                screen[j].g    = tri[j].g;
                screen[j].b    = tri[j].b;
                screen[j].a    = tri[j].a;

                // Текстурные координаты: s/w, t/w, умноженные на размер текстуры
                screen[j].tmuvtx[0].sow =
                    tri[j].tmuvtx[0].sow * clip.oow * texture->width;
                screen[j].tmuvtx[0].tow =
                    tri[j].tmuvtx[0].tow * clip.oow * texture->height;
                screen[j].tmuvtx[0].oow = clip.oow;

                if (screen[j].oow <= 0.0f) { ok = 0; break; }
            }

            if (!ok) continue;

            if (camera.wireframe_mode == 0) {
                grDrawTriangle(&screen[0], &screen[1], &screen[2]);
                triangles_drawn++;
            } else {
                grConstantColorValue(0xFFFFFFFF);
                grAADrawLine(&screen[0], &screen[1]);
                grAADrawLine(&screen[1], &screen[2]);
                grAADrawLine(&screen[2], &screen[0]);
            }
        }
    }

    grGlideSetState(&grState);
}
