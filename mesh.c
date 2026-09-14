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

static GrVertex tempVtx[MAX_VERTICES];
    //GrVertex screen[3]; // Экранный треугольник
static GrVertex screen[3]; // Только для одного треугольника
int triangles_drawn;

inline float eval_plane_value(GrVertex v, float* plane) {
    return plane[0] * v.x + plane[1] * v.y + plane[2] * v.z + plane[3];
}

// Клиппинг треугольника по произвольной плоскости
int ClipTriangleByPlane(
    GrVertex* output,                    // массив для выходных треугольников (макс 8*3 вершин)
    GrVertex* v1,                  // вершина 1
    GrVertex* v2,                  // вершина 2
    GrVertex* v3,                  // вершина 3
    float* plane,                  // [a, b, c, d]
    int max_output_vertices              // размер выходного буфера (должен быть >= 24)
) {
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

/*  Дроп треугольника по произвольной плоскости,
    возвращает 1, если возвращается исхожный,
    0 если дропается */
int DropTriangleByPlane(
    GrVertex* v1,                  // вершина 1
    GrVertex* v2,                  // вершина 2
    GrVertex* v3,                  // вершина 3
    float* plane
) {
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

// Полный клиппинг треугольника по frustum
// Возвращает количество треугольников (0 - до 8)
int ClipTriangleByFrustum(
    GrVertex* output,                    // буфер для выходных треугольников
    GrVertex* v1,                  // вершина 1
    GrVertex* v2,                  // вершина 2
    GrVertex* v3,                  // вершина 3
    FrustumPlanes* planes,         // плоскости frustum
    int max_output_vertices              // размер буфера (должен быть >= 24)
) {
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
        //if (p == 5) {
        if (1) {
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
                num_clipped = ClipTriangleByPlane(
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

// Полный дроп треугольника по frustum
// Возвращает количество треугольников (0 - до 8)
int DropTriangleByFrustum(
    GrVertex* v1,                  // вершина 1
    GrVertex* v2,                  // вершина 2
    GrVertex* v3,                  // вершина 3
    FrustumPlanes* planes
) {
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


// Копирование вершины
static void CopyVertex3D(GrVertex* dest, GrVertex* src) {
    memcpy(dest, src, sizeof(GrVertex));
}

// Интерполяция вершины для клиппинга
static void InterpolateVertex3D(GrVertex* out, 
                                GrVertex* v1, 
                                GrVertex* v2, 
                                float t) {
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

    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = 0;
        if (line[0] == '#' || line[0] == '\0') continue;

        // Парсинг вертексов - первый проход, просто считаем
        // Пример строки: "v 1.000000 1.000000 -1.000000"
        if (strncmp(line, "v ", 2) == 0) {
            float x, y, z;
            if (sscanf(line+2, "%f %f %f", &x, &y, &z) == 3) { num_vertices++; }
        }
        // Парсинг нормалей - первый проход, просто считаем
        // Пример строки: "vn 0.000000 0.000000 0.11000"
        else if (strncmp(line, "vn ", 3) == 0) {
            float x, y, z;
            if (sscanf(line+3, "%f %f %f", &x, &y, &z) == 3) { num_normals++; }
        }
        // Парсинг текстурных координат - первый проход, просто считаем
        // Пример строки: "vt 0.000000 0.000000"
        else if (strncmp(line, "vt ", 3) == 0) {
            float u, v;
            if (sscanf(line+3, "%f %f", &u, &v) == 2) { num_texture_coordinates++; }
        }
        // Парсинг фейсов - первый проход, просто считаем
        // Пример строки: "f 2 3 1" - только если нет UV-координат
        // Если есть, то
        // f 4/1 17/2 5/3
        // 4, 17, 5 — это индексы вершин (из списка v). Они нумеруются с 1.
        // 1, 2, 3 — это индексы текстурных координат (из списка vt). Они тоже нумеруются с 1.
        else if (strncmp(line, "f ", 2) == 0) {
            int v1, v2, v3, vt1, vt2, vt3, vn1, vn2, vn3;
            // Пытаемся прочитать тройку вертексов для построения треугольника
            if (sscanf(line+2, "%d %d %d",
                    &v1, &v2, &v3) == 3) { num_faces++; }
            else if (sscanf(line+2, "%d/%d %d/%d %d/%d",
                    &v1, &vt1, &v2, &vt2, &v3, &vt3) == 6) { num_faces++; }
            else if (sscanf(line+2, "%d/%d/%d %d/%d/%d %d/%d/%d",
                    &v1, &vt1, &vn1, &v2, &vt2, &vn2, &v3, &vt3, &vn3) == 9) { num_faces++; }
            else {
                // Не удалось прочитать 9 чисел – пропускаем или сообщаем об ошибке
                printf("WARNING: unsupported face format: %s\n", line);
            }
        }
    }
    fclose(f);

    printf("[FILE] Preload %s : %d vertices, %d faces, %d texture coords, %d normals\n", 
        filename, num_vertices, num_faces, num_texture_coordinates, num_normals);


    // Теперь выделяем ровно столько памяти, сколько нужно
    // Откроем файл ещё раз и заполним массивы
    // По размеру структуры GrVertex
    //mesh->grVertices = malloc(num_vertices * sizeof(GrVertex));
    mesh->grVertices = malloc((num_vertices * sizeof(GrVertex) + 15) & ~15);

    // Неужели в этом была причина?!
    // что код не работал на железке
    // ... нет
    // for (i = 0; i < num_verts; i++)
    // {
    //     GrVertex *grVertex = mesh->grVertices[i];
    //     grVertex->tmuvtx = malloc(GLIDE_NUM_TMU * sizeof(GrTmuVertex)); 
    // }

    // Инициализируем все вершины
    for (i = 0; i < num_vertices; i++) {
        memset(&mesh->grVertices[i], 0, sizeof(GrVertex));
        mesh->grVertices[i].oow = 1.0f;  // Важно!
        // tmuvtx уже встроен в структуру, не нужно выделять отдельно
    }

    // По 3 int на один треугольник
    mesh->indices_vertices =            malloc(num_faces * 3 * sizeof(int));
    if (num_normals > 0)
    {
        mesh->indices_normals =             malloc(num_faces * 3 * sizeof(int));
    }

    if (num_texture_coordinates > 0)
    {
        mesh->indices_texture_coordinates = malloc(num_faces * 3 * sizeof(int));
    }
    
    // По 2 float на одну координату (vt 0.001 0.001)
    if (num_texture_coordinates > 0)
    {
        mesh->texture_coordinates = malloc(num_texture_coordinates * 2 * sizeof(float));
    }

    // По 3 float на одну нормаль (vn 0.001 0.001 0.001)
    if (num_normals > 0)
    {
        mesh->normals = malloc(num_normals * 3 * sizeof(float));
    }

    // printf("After many malloc\n");
    // return mesh;

    f = fopen(filename, "r");
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = 0;
        if (line[0] == '#' || line[0] == '\0') continue;

        // Парсинг вертексов
        // Пример строки: "v 1.000000 1.000000 -1.000000"
        if (num_vertices > 0 && strncmp(line, "v ", 2) == 0) {
            float x, y, z;
            if (sscanf(line+2, "%f %f %f", &x, &y, &z) == 3) {
                
                mesh->grVertices[counter_v].r = 1;
                mesh->grVertices[counter_v].g = 1;
                mesh->grVertices[counter_v].b = 1;
                mesh->grVertices[counter_v].a = 1;

                mesh->grVertices[counter_v].oow = 1;
                mesh->grVertices[counter_v].ooz = 1;

                mesh->grVertices[counter_v].tmuvtx[0].oow = 1;
                mesh->grVertices[counter_v].tmuvtx[0].sow = 1;
                mesh->grVertices[counter_v].tmuvtx[0].tow = 1;

                mesh->grVertices[counter_v].tmuvtx[1].oow = 1;
                mesh->grVertices[counter_v].tmuvtx[1].sow = 1;
                mesh->grVertices[counter_v].tmuvtx[1].tow = 1;

                mesh->grVertices[counter_v].x = x;
                mesh->grVertices[counter_v].y = y;
                mesh->grVertices[counter_v].z = z;

                // printf("File: %f %f %f\n", x, y, z);
                // printf("Mesh: %f %f %f\n", mesh->grVertices[counter].x,
                //     mesh->grVertices[counter].y, mesh->grVertices[counter].z);

                counter_v++;
            }
        }
        // Парсинг нормалей - второй проход, заполняем массивы
        // Пример строки: "vn 0.000000 0.000000 0.11000"
        else if (num_normals > 0 && strncmp(line, "vn ", 3) == 0) {
            float x, y, z;
            if (sscanf(line+3, "%f %f %f", &x, &y, &z) == 3) {
                mesh->normals[counter_n] = x;
                counter_n++;

                mesh->normals[counter_n] = y;
                counter_n++;

                mesh->normals[counter_n] = z;
                counter_n++;
            }
        }
        // Парсинг текстурных координат
        // Пример строки: "vt 0.000000 0.000000"
        else if (num_texture_coordinates > 0 && strncmp(line, "vt ", 3) == 0) {
            float u, v;
            if (sscanf(line+3, "%f %f", &u, &v) == 2) {
                mesh->texture_coordinates[counter_vt] = u;
                counter_vt++;

                mesh->texture_coordinates[counter_vt] = v;
                counter_vt++;
            }
        }
        // Парсинг фейсов
        // Пример строки: "f 2 3 1" - только если нет UV-координат
        // Если есть, то
        // f 4/1 17/2 5/3
        // 4, 17, 5 — это индексы вершин (из списка v). Они нумеруются с 1.
        // 1, 2, 3 — это индексы текстурных координат (из списка vt). Они тоже нумеруются с 1.
        else if (num_faces > 0 && strncmp(line, "f ", 2) == 0) {
            int v1, v2, v3, vt1, vt2, vt3, vn1, vn2, vn3;
            // Пытаемся прочитать тройку вертексов для построения треугольника
            if (sscanf(line+2, "%d %d %d",
                    &v1, &v2, &v3) == 3)
            {
                mesh->indices_vertices[counter_f] = v1 - 1;
                // printf("Index %d added: %d\n", counter2, mesh->indices[counter2]);
                counter_f++;

                mesh->indices_vertices[counter_f] = v2 - 1;
                // printf("Index %d added: %d\n", counter2, mesh->indices[counter2]);
                counter_f++;

                mesh->indices_vertices[counter_f] = v3 - 1;
                // printf("Index %d added: %d\n", counter2, mesh->indices[counter2]);
                counter_f++;
            }
            else if (sscanf(line+2, "%d/%d %d/%d %d/%d",
                    &v1, &vt1, &v2, &vt2, &v3, &vt3) == 6)
            {
                mesh->indices_vertices[counter_f] = v1 - 1;
                mesh->indices_texture_coordinates[counter_f] = vt1 - 1;
                // printf("Index %d added: %d\n", counter2, mesh->indices[counter2]);
                counter_f++;

                mesh->indices_vertices[counter_f] = v2 - 1;
                mesh->indices_texture_coordinates[counter_f] = vt2 - 1;
                // printf("Index %d added: %d\n", counter2, mesh->indices[counter2]);
                counter_f++;

                mesh->indices_vertices[counter_f] = v3 - 1;
                mesh->indices_texture_coordinates[counter_f] = vt3 - 1;
                // printf("Index %d added: %d\n", counter2, mesh->indices[counter2]);
                counter_f++;
            }
            else if (sscanf(line+2, "%d/%d/%d %d/%d/%d %d/%d/%d",
                    &v1, &vt1, &vn1, &v2, &vt2, &vn2, &v3, &vt3, &vn3) == 9)
            {
                mesh->indices_vertices[counter_f] = v1 - 1;
                mesh->indices_normals[counter_f] = vn1 - 1;
                mesh->indices_texture_coordinates[counter_f] = vt1 - 1;
                // printf("Index %d added: %d\n", counter2, mesh->indices[counter2]);
                counter_f++;

                mesh->indices_vertices[counter_f] = v2 - 1;
                mesh->indices_normals[counter_f] = vn2 - 1;
                mesh->indices_texture_coordinates[counter_f] = vt2 - 1;
                // printf("Index %d added: %d\n", counter2, mesh->indices[counter2]);
                counter_f++;

                mesh->indices_vertices[counter_f] = v3 - 1;
                mesh->indices_normals[counter_f] = vn3 - 1;
                mesh->indices_texture_coordinates[counter_f] = vt3 - 1;
                // printf("Index %d added: %d\n", counter2, mesh->indices[counter2]);
                counter_f++;
            }
            else {
                // Не удалось прочитать 9 чисел – пропускаем или сообщаем об ошибке
                printf("WARNING: unsupported face format: %s\n", line);
            }
        }
    }
    fclose(f);

    mesh->num_vertices = num_vertices;
    mesh->num_faces = num_faces;
    mesh->num_texture_coordinates = num_texture_coordinates;
    mesh->num_normals = num_normals;

    // Тут надо обработать массив GrVertex,
    // чтобы добавить текстурные координаты
    // Перебор по фейсам, то есть по тройкам
    if (num_texture_coordinates > 0) {
        for (i = 0; i < mesh->num_faces * 3; i += 3) {
            float u, v;
            int index_tc1, index_tc2, index_tc3;
            int index_v1, index_v2, index_v3;

            // Поскольку у нас совпадают индексы indices_vertices и indices_texture_coordinates
            // то можно так найти искомый вертекс
            // Индексы текстурных координат
            index_tc1 = mesh->indices_texture_coordinates[i];
            index_tc2 = mesh->indices_texture_coordinates[i+1];
            index_tc3 = mesh->indices_texture_coordinates[i+2];

            // Соответствующие индексы вершин
            index_v1 = mesh->indices_vertices[i];
            index_v2 = mesh->indices_vertices[i+1];
            index_v3 = mesh->indices_vertices[i+2];
            
            // Поскольку текстурные координаты идут парами,
            // то удваиваем
            
            // Первая вершина треугольника
            u = mesh->texture_coordinates[index_tc1 * 2];
            v = mesh->texture_coordinates[index_tc1 * 2 + 1];

            mesh->grVertices[index_v1].tmuvtx[0].sow = u;
            mesh->grVertices[index_v1].tmuvtx[0].tow = 1 - v;
            mesh->grVertices[index_v1].tmuvtx[0].oow = 1.0f;

            // Вторая вершина треугольника
            u = mesh->texture_coordinates[index_tc2 * 2];
            v = mesh->texture_coordinates[index_tc2 * 2 + 1];

            mesh->grVertices[index_v2].tmuvtx[0].sow = u;
            mesh->grVertices[index_v2].tmuvtx[0].tow = 1 - v;
            mesh->grVertices[index_v2].tmuvtx[0].oow = 1.0f;

            // Третья вершина треугольника
            u = mesh->texture_coordinates[index_tc3 * 2];
            v = mesh->texture_coordinates[index_tc3 * 2 + 1];

            mesh->grVertices[index_v3].tmuvtx[0].sow = u;
            mesh->grVertices[index_v3].tmuvtx[0].tow = 1 - v;
            mesh->grVertices[index_v3].tmuvtx[0].oow = 1.0f;
        }
    }

    // Попробуем перебрать по самим вертексам
    // if (num_texture_coordinates > 0) {
    //     for (i = 0; i < num_vertices; i++) {
    //         float u = 0, v = 0;
    //         int index_tc = 0;

    //         index_tc = mesh->indices_texture_coordinates[i];

    //         u = mesh->texture_coordinates[index_tc * 2];
    //         v = mesh->texture_coordinates[index_tc * 2 + 1];

    //         mesh->grVertices[i].tmuvtx[0].sow = u;
    //         mesh->grVertices[i].tmuvtx[0].tow = v;
    //         mesh->grVertices[i].tmuvtx[0].oow = 1.0f;

    //         //printf("DEBUG vertex %d index tex coord %d u %f v %f\n", i, index_tc, u, v);
    //     }
    // }

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

// void DrawMesh2(Mesh* mesh, TextureSlot* texture,
//     float pos_x, float pos_y, float pos_z,
//     float rot_x, float rot_y, float rot_z,
//     float scale_x, float scale_y, float scale_z) {
//     GrState grState;
//     int i, j, t;
//     float model[16];
//     float proj[16];
//     float mvp[16];
//     GrVertex tempVtx[MAX_VERTICES];
//     GrVertex clipSpaceVerts[MAX_VERTICES];  // Вершины в Clip Space (до деления)
//     FrustumPlanes frustum;
//     GrVertex clipped_verts[24];
//     int num_tris;
//     if (!mesh || !mesh->grVertices) return;
//     grGlideGetState(&grState);
//     // Настройка текстурного состояния (как было)
//     if (camera.wireframe_mode == 1) {
//         grTexCombine(GR_TMU0,
//              GR_COMBINE_FUNCTION_LOCAL,
//              GR_COMBINE_FACTOR_LOCAL,
//              GR_COMBINE_FUNCTION_LOCAL,
//              GR_COMBINE_OTHER_NONE,
//              FXFALSE, FXFALSE);
//         grColorCombine(
//             GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_ONE,
//             GR_COMBINE_LOCAL_NONE, GR_COMBINE_OTHER_ITERATED,
//             FXFALSE );
//     }
//     else {
//         grTexCombine(
//             GR_TMU0,
//             GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE,
//             GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE,
//             FXFALSE, FXFALSE);
//         grColorCombine(
//             GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_LOCAL,
//             GR_COMBINE_LOCAL_CONSTANT, GR_COMBINE_OTHER_TEXTURE,
//             FXFALSE );
//         grTexSource(texture->tmu,
//             texture->baseAddr,
//             GR_MIPMAPLEVELMASK_BOTH,
//             &texture->grTexInfo);
//     }
//     // Строим матрицы
//     MatrixIdentity(model);
//     MatrixEulerRotation(model, rot_x, rot_y, rot_z);
//     MatrixTranslation(model, pos_x, pos_y, pos_z);
//     MatrixScale(model, scale_x, scale_y, scale_z);
//     MatrixIdentity(proj);
//     MatrixProjection(proj,
//         camera.fov,
//         camera.aspect,
//         camera.near_clip,
//         camera.far_clip);
//     // Извлекаем плоскости frustum из матрицы проекции
//     ExtractFrustumPlanes(&frustum, proj);
//     MatrixIdentity(mvp);
//     MatrixMultiply(mvp, mvp, model);
//     MatrixMultiply(mvp, mvp, view);
//     MatrixMultiply(mvp, mvp, proj);
//     // Копируем вершины модели
//     memcpy(tempVtx, mesh->grVertices, mesh->num_vertices * sizeof(GrVertex));
//     // ПРИМЕНЯЕМ MVP, НО НЕ ДЕЛАЕМ ПЕРСПЕКТИВНОЕ ДЕЛЕНИЕ!
//     for (i = 0; i < mesh->num_vertices; i++) {
//         // Сохраняем оригинальные текстурные координаты
//         float orig_s = tempVtx[i].tmuvtx[0].sow;
//         float orig_t = tempVtx[i].tmuvtx[0].tow;
//         // Применяем матрицу (это даст нам Clip Space координаты)
//         ApplyMatrix(&tempVtx[i], mvp);
//         // Сохраняем в отдельный буфер для клиппинга
//         memcpy(&clipSpaceVerts[i], &tempVtx[i], sizeof(GrVertex));
//         // Восстанавливаем оригинальные текстурные координаты (до умножения на oow)
//         // Они нам понадобятся для клиппинга
//         clipSpaceVerts[i].tmuvtx[0].sow = orig_s;
//         clipSpaceVerts[i].tmuvtx[0].tow = orig_t;
//     }
//     // Рисуем треугольники с клиппингом в Clip Space
//     for (i = 0; i < mesh->num_faces * 3; i += 3) {
//         int idx1 = mesh->indices_vertices[i];
//         int idx2 = mesh->indices_vertices[i+1];
//         int idx3 = mesh->indices_vertices[i+2];
//         GrVertex* v1 = &clipSpaceVerts[idx1];
//         GrVertex* v2 = &clipSpaceVerts[idx2];
//         GrVertex* v3 = &clipSpaceVerts[idx3];
//         // Проверка: все ли вершины за near plane?
//         if (v1->z < camera.near_clip && 
//             v2->z < camera.near_clip && 
//             v3->z < camera.near_clip) {
//             continue; // Полностью невидим
//         }
//         // Клиппинг по всем плоскостям frustum (в Clip Space)
//         num_tris = ClipTriangleByFrustum(
//             clipped_verts,
//             v1, v2, v3,
//             &frustum,
//             24
//         );
//         if (num_tris == 0) {
//     // Проверяем, почему треугольник отброшен
//     printf("Tri clipped: v1=(%.2f,%.2f,%.2f,%.2f) v2=(%.2f,%.2f,%.2f,%.2f) v3=(%.2f,%.2f,%.2f,%.2f)\n",
//         v1->x, v1->y, v1->z, v1->oow,
//         v2->x, v2->y, v2->z, v2->oow,
//         v3->x, v3->y, v3->z, v3->oow);
// }
// return ;
//         // Рендерим полученные треугольники
//         for (t = 0; t < num_tris; t++) {
//             GrVertex* tri = &clipped_verts[t * 3];
//             GrVertex screen[3];
//             // Для каждой вершины делаем перспективное деление и переход в экранные координаты
//             for (j = 0; j < 3; j++) {
//                 // Сначала умножаем текстурные координаты на oow и размер текстуры
//                 tri[j].tmuvtx[0].sow *= tri[j].oow * texture->width;
//                 tri[j].tmuvtx[0].tow *= tri[j].oow * texture->height;
//                 // Теперь перспективное деление и экранные координаты
//                 VertexToScreen(&tri[j], &screen[j]);
//             }
//             // Проверяем валидность и рисуем
//             if (screen[0].oow > 0 && screen[1].oow > 0 && screen[2].oow > 0) {
//                 if (camera.wireframe_mode == 0) {
//                     guAADrawTriangleWithClip(&screen[0], &screen[1], &screen[2]);
//                 } else {
//                     grConstantColorValue(0xFFFFFFFF);
//                     grAADrawLine(&screen[0], &screen[1]);
//                     grAADrawLine(&screen[1], &screen[2]);
//                     grAADrawLine(&screen[2], &screen[0]);
//                 }
//             }
//         }
//     }
//     grGlideSetState(&grState);
// }

void DrawMesh(Mesh* mesh, TextureSlot* texture,
    float pos_x, float pos_y, float pos_z,
    float rot_x, float rot_y, float rot_z,
    float scale_x, float scale_y, float scale_z) {
    
    GrState grState;
    int i, j;
    float model[16];      // Матрица модели (положение, поворот, масштаб объекта)
    //float view[16];       // Матрица вида (камера) теперь в camera.c - UpdateView()
    //float proj[16];       // Матрица проекции теперь в camera.c - UpdateView()
    float mvp[16];        // Итоговая MVP матрица
    
    //GrVertex* tempVtx;
    //GrVertex screen[3]; // Экранный треугольник
    //GrVertex* screen;
    
    int dropped;
    int valid;

    if (!mesh || !mesh->grVertices) return;
   
    // Копируем состояние и настраиваем текстуру (как в DrawTexturedCubeAt)
    grGlideGetState(&grState);
   
    if (camera.wireframe_mode == 1) {
        grTexCombine(GR_TMU0, // Используем первый TMU
            //  GR_COMBINE_FUNCTION_LOCAL, // Использовать тексель из текстуры как локальный цвет
            //  GR_COMBINE_FACTOR_LOCAL,
            //  GR_COMBINE_FUNCTION_LOCAL, // Совместить локальный цвет с вершинным
            //  GR_COMBINE_OTHER_NONE,
            GR_COMBINE_FUNCTION_ZERO,   // ARG1: игнорируем текстуру
            GR_COMBINE_FACTOR_ZERO,
            GR_COMBINE_FUNCTION_ZERO,   // ARG2: игнорируем текстуру
            GR_COMBINE_OTHER_NONE,
             FXFALSE, FXFALSE);
        grColorCombine(
            // GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_ONE,
            // GR_COMBINE_LOCAL_CONSTANT, GR_COMBINE_OTHER_ITERATED,
            GR_COMBINE_FUNCTION_SCALE_OTHER,
            GR_COMBINE_FACTOR_ONE,
            GR_COMBINE_LOCAL_NONE,      // Нет локального цвета
            GR_COMBINE_OTHER_ITERATED,  // Используем вершинный цвет
            FXFALSE );
    }
    else {
        // Только текстура
        grTexCombine(
            GR_TMU0,
            GR_COMBINE_FUNCTION_LOCAL,
            GR_COMBINE_FACTOR_NONE,
            GR_COMBINE_FUNCTION_LOCAL,
            GR_COMBINE_FACTOR_NONE,
            FXFALSE, FXFALSE);
        
        grColorCombine(
            GR_COMBINE_FUNCTION_SCALE_OTHER,
            GR_COMBINE_FACTOR_LOCAL,
            GR_COMBINE_LOCAL_CONSTANT,
            GR_COMBINE_OTHER_TEXTURE,
            FXFALSE );
        grTexSource(texture->tmu,
            texture->baseAddr,
            GR_MIPMAPLEVELMASK_BOTH,
            &texture->grTexInfo);
    }

    MatrixIdentity(      model);
    MatrixEulerRotation( model, rot_x,   rot_y,   rot_z);
    MatrixTranslation(   model, pos_x,   pos_y,   pos_z);
    MatrixScale(         model, scale_x, scale_y, scale_z);
    
    // Матрица View обрабатывается в UpdateView (camera.c)
    // MatrixLookAt(view,
    //     0.0f, 0.0f, 0.0f,      // позиция глаза (камеры)
    //     0.0f, 0.0f, 50.0f,      // куда смотрит камера
    //     0.0f, 1.0f, 0.0f);     // направление "вверх"

    // MatrixIdentity(proj);
    // MatrixProjection(proj,
    //     camera.fov,          // FOV 60 градусов
    //     camera.aspect,       // aspect ratio 4:3
    //     camera.near_clip,    // near plane
    //     camera.far_clip);    // far plane

    // Проверка на некорректные значения
    // for (i = 0; i < 16; i++) {
    //     if (isnan(proj[i]) || isinf(proj[i])) {
    //         printf("ERROR: Invalid matrix value at index %d\n", i);
    //         MatrixIdentity(mvp);
    //         return;
    //     }
    // }

    // printf("Camera: %f %f %f %f\n", camera.fov,          // FOV 60 градусов
    //     camera.aspect,       // aspect ratio 4:3
    //     camera.near_clip,    // near plane
    //     camera.far_clip);

    MatrixIdentity(mvp);
    
    // Порядок MVP = Model x View x Projection,
    // потому что в проекте используется порядок Row Major
    // то есть строки матрицы хранятся последовательно
    MatrixMultiply(mvp, mvp, model);
    MatrixMultiply(mvp, mvp, view);
    MatrixMultiply(mvp, mvp, proj);

    // Проверка на некорректные значения
    // for (i = 0; i < 16; i++) {
    //     if (isnan(mvp[i]) || isinf(mvp[i])) {
    //         printf("ERROR: Invalid matrix value at index %d\n", i);
    //         MatrixIdentity(mvp);
    //         return;
    //     }
    // }

    // Извлекаем плоскости frustum из матрицы проекции
    //ExtractFrustumPlanes(&frustum, proj);
    
    //ExtractFrustumPlanes(&frustum, view);
    //ExtractFrustumPlanes(&frustum, mvp);
    // Near plane: z = near_clip (смотрит в +Z)
    // frustum.near_plane[0] = 0.0f;
    // frustum.near_plane[1] = 0.0f;
    //frustum.near_plane[2] = 1.0f;
    // //frustum.near_plane[3] = -camera.near_clip;  // например, -0.1f
    //frustum.near_plane[3] = -1;  // например, -0.1f

    // Far plane: z = far_clip (смотрит в -Z)
    // frustum.far_plane[0] = 0.0f;
    // frustum.far_plane[1] = 0.0f;
    //frustum.far_plane[2] = -1.0f;
    //frustum.far_plane[2] = 0.0f;
    // //frustum.far_plane[3] = camera.far_clip;  // например, 100.0f
    //frustum.far_plane[3] = 20.0f;  // например, 100.0f
        
    // Отладочный вывод плоскостей
    // printf("Near plane: %.3f, %.3f, %.3f, %.3f\n", 
    //     frustum.near_plane[0], frustum.near_plane[1], 
    //     frustum.near_plane[2], frustum.near_plane[3]);
    // printf("Far plane: %.3f, %.3f, %.3f, %.3f\n", 
    //     frustum.far_plane[0], frustum.far_plane[1], 
    //     frustum.far_plane[2], frustum.far_plane[3]);
    // printf("Left plane: %.3f, %.3f, %.3f, %.3f\n", 
    //     frustum.left_plane[0], frustum.left_plane[1], 
    //     frustum.left_plane[2], frustum.left_plane[3]);
    // printf("Right plane: %.3f, %.3f, %.3f, %.3f\n", 
    //     frustum.right_plane[0], frustum.right_plane[1], 
    //     frustum.right_plane[2], frustum.right_plane[3]);

    // Crash
    // Победил краш уменьшением MAX_VERTICES до 512
    //tempVtx = malloc(mesh->num_vertices * sizeof(GrVertex));
    // Невыровненные данные приводят к зависанию на реальной железке?..
    //tempVtx = (GrVertex*)malloc((mesh->num_vertices * sizeof(GrVertex) + 15) & ~15);
    memcpy(tempVtx, mesh->grVertices, mesh->num_vertices * sizeof(GrVertex));

    //screen = (GrVertex*)malloc((3 * sizeof(GrVertex) + 15) & ~15);
    
    // Трансформируем и рисуем
    for (i = 0; i < mesh->num_vertices; i++) {
        ApplyMatrix(&tempVtx[i], mvp);
        
        // масштабируем текстурные координаты
        tempVtx[i].tmuvtx[0].sow *= tempVtx[i].oow * (unsigned long)texture->width;
        tempVtx[i].tmuvtx[0].tow *= tempVtx[i].oow * (unsigned long)texture->height;

        // tempVtx[i].tmuvtx[0].sow = 1.0f;
        // tempVtx[i].tmuvtx[0].tow = 1.0f;
        // tempVtx[i].tmuvtx[1].sow = 1.0f;
        // tempVtx[i].tmuvtx[1].tow = 1.0f;

        // tempVtx[i].tmuvtx[0].sow *= (unsigned long)texture->width;
        // tempVtx[i].tmuvtx[0].tow *= (unsigned long)texture->height;
        
        //VertexToScreen2(&tempVtx[i]);
    }

    // Рисуем треугольники
    // Здесь неправильно:
    // вертексы расположены не по индексам,
    // а нам нужно итерировать индексы и выбирать нужные вертексы из &tempVtx[i]
    for (i = 0; i < mesh->num_faces * 3; i+=3) {
        GrVertex* a = &tempVtx[mesh->indices_vertices[i]];
        GrVertex* b = &tempVtx[mesh->indices_vertices[i+1]];
        GrVertex* c = &tempVtx[mesh->indices_vertices[i+2]];

        dropped = DropTriangleByFrustum(
            a, b, c,
            &frustum
        );
        
        // Пропускаем треугольники за пределами пространства камеры
        if (dropped > 0) continue;

        // Преобразуем в экранные координаты
        for (j = 0; j < 3; j++) {
            VertexToScreen(&tempVtx[mesh->indices_vertices[i+j]], &screen[j]);
        }
        
        // Проверяем на inf/nan во всех полях вершин
        // valid = 1;
        // for (j = 0; j < 3; j++) {
        //     if (isnan(screen[j].x) || 
        //         isnan(screen[j].y) || 
        //         isnan(screen[j].z) ||
        //         isnan(screen[j].oow) || 
        //         isnan(screen[j].tmuvtx[0].sow) || 
        //         isnan(screen[j].tmuvtx[0].tow) ||
        //         isnan(screen[j].tmuvtx[1].sow) || 
        //         isnan(screen[j].tmuvtx[1].tow) ||
        //         isinf(screen[j].x) || 
        //         isinf(screen[j].y) || 
        //         isinf(screen[j].z) ||
        //         isinf(screen[j].oow) || 
        //         isinf(screen[j].tmuvtx[0].sow) || 
        //         isinf(screen[j].tmuvtx[0].tow) ||
        //         isinf(screen[j].tmuvtx[1].sow) || 
        //         isinf(screen[j].tmuvtx[1].tow)
        //     ) {
        //         valid = 0;
        //         break;
        //     }
        // }
        // if (!valid) continue;

        // Проверяем валидность и рисуем
        if (screen[0].oow > 0 && screen[1].oow > 0 && screen[2].oow > 0) {
            if (camera.wireframe_mode == 0) {
                //guAADrawTriangleWithClip(&screen[0], &screen[1], &screen[2]);
                grDrawTriangle(&screen[0], &screen[1], &screen[2]);
                triangles_drawn++;
            }
            else {
                grConstantColorValue(0xFFFFFFFF);   // белый (ABGR)

                grAADrawPoint(&screen[0]);
                grAADrawPoint(&screen[1]);
                grAADrawPoint(&screen[2]);

                grAADrawLine(&screen[0], &screen[1]);
                grAADrawLine(&screen[1], &screen[2]);
                grAADrawLine(&screen[2], &screen[0]);
            }
        }
    }

    grGlideSetState(&grState);

    //free(tempVtx);
    //free(screen);
}
