// MATRIX.C - операции с матрицами
// Все преобразования применяются через пост-умножение (A = A × B).
// Зачем вообще матрицы?
// Точка в 3D-пространстве — это вектор (x, y, z).
// Чтобы её сдвинуть, повернуть, масштабировать, спроецировать на экран,
// мы используем одну и ту же операцию — умножение вектора на матрицу 4×4.
// Во всём проекте используется Row Major порядок,
// когда строки хранятся последовательно.

#include "matrix.h"
#include <stdio.h>
#include <string.h>
#include <math.h>


#include "fastmath.h"

#pragma pack(push, 4)
#include "glide.h"

// Единичная матрица
void MatrixIdentity(float* matrix) {
    // Все элементы = 0, кроме диагонали (1,1,1,1)
    // 1 0 0 0
    // 0 1 0 0
    // 0 0 1 0
    // 0 0 0 1
    memset(matrix, 0, sizeof(float) * 16);
    SET_MATRIX_VALUE(matrix, 0, 0, 1);
    SET_MATRIX_VALUE(matrix, 1, 1, 1);
    SET_MATRIX_VALUE(matrix, 2, 2, 1);
    SET_MATRIX_VALUE(matrix, 3, 3, 1);
}

// Умножение двух матриц: out = a × b
void MatrixMultiply(float* out, const float* a, const float* b)
{
    float temp[16]; // временный буфер, чтобы можно было умножать на себя
    float sum;
    int i, j, k;

    // Проверка на некорректные значения
    // for (i = 0; i < 16; i++) {
    //     if (isnan(a[i]) || isinf(a[i]) || isnan(b[i]) || isinf(b[i])) {
    //         printf("ERROR: Invalid matrix value at index %d\n", i);
    //         MatrixIdentity(out);
    //         return;
    //     }
    // }

    for (i = 0; i < 4; i++) {            // по строкам результата
        for (j = 0; j < 4; j++) {         // по столбцам результата
            sum = 0.0f;
            for (k = 0; k < 4; k++) {     // скалярное произведение
                sum += GET_MATRIX_VALUE(a, i, k) * GET_MATRIX_VALUE(b, k, j);
            }
            SET_MATRIX_VALUE(temp, i, j, sum);
        }
    }

    // Копируем результат обратно (работает даже если out == a или out == b)
    memcpy(out, temp, sizeof(float) * 16);
}

// Передвижение
void MatrixTranslation(float* m, float x, float y, float z)
{
    float t[16];
    MatrixIdentity(t);

    // В row-major матрице перенос записывается в последнюю строку
    SET_MATRIX_VALUE(t, 3, 0, x);
    SET_MATRIX_VALUE(t, 3, 1, y);
    SET_MATRIX_VALUE(t, 3, 2, z);

    MatrixMultiply(m, m, t);        // m = m × T
}

// Повороты
void MatrixRotationX(float* m, float angle_rad)
{
    float r[16];
    float s, c;

    MatrixIdentity(r);

    c = fast_cos(angle_rad);
    s = fast_sin(angle_rad);

    SET_MATRIX_VALUE(r, 1, 1,  c);
    SET_MATRIX_VALUE(r, 1, 2, -s);
    SET_MATRIX_VALUE(r, 2, 1,  s);
    SET_MATRIX_VALUE(r, 2, 2,  c);

    MatrixMultiply(m, m, r);
}

void MatrixRotationY(float* m, float angle_rad)
{
    float r[16];
    float s, c;

    MatrixIdentity(r);

    c = fast_cos(angle_rad);
    s = fast_sin(angle_rad);

    SET_MATRIX_VALUE(r, 0, 0,  c);
    SET_MATRIX_VALUE(r, 0, 2,  s);
    SET_MATRIX_VALUE(r, 2, 0, -s);
    SET_MATRIX_VALUE(r, 2, 2,  c);

    MatrixMultiply(m, m, r);
}

void MatrixRotationZ(float* m, float angle_rad)
{
    float r[16];
    float s, c;

    MatrixIdentity(r);

    c = fast_cos(angle_rad);
    s = fast_sin(angle_rad);

    SET_MATRIX_VALUE(r, 0, 0,  c);
    SET_MATRIX_VALUE(r, 0, 1, -s);
    SET_MATRIX_VALUE(r, 1, 0,  s);
    SET_MATRIX_VALUE(r, 1, 1,  c);

    MatrixMultiply(m, m, r);
}

// Функция для поворота по трём осям (Euler angles, порядок XYZ)
void MatrixEulerRotation(float* m, float x, float y, float z)
{
    MatrixRotationX(m, x * DEGTORAD);
    MatrixRotationY(m, y * DEGTORAD);
    MatrixRotationZ(m, z * DEGTORAD);
}

// Масштабирование
void MatrixScale(float* matrix, float sx, float sy, float sz)
{
    float tmp[16];
    MatrixIdentity(tmp);
    SET_MATRIX_VALUE(tmp, 0, 0, sx);
    SET_MATRIX_VALUE(tmp, 1, 1, sy);
    SET_MATRIX_VALUE(tmp, 2, 2, sz);
    MatrixMultiply(matrix, matrix, tmp);
}

// Перспективная проекция
void MatrixProjection(float* matrix, float fov_deg, float aspect,
    float near_clip, float far_clip)
{
	float projection[16];
    float fov_rad, tan_half_fov, y_scale, x_scale;

    MatrixIdentity(projection);

    fov_rad = fov_deg * DEGTORAD;
    
    // \  |  /
    //  \ | /   поэтому делим пополам
    //   \|/
    // Криво написанная мной функция fast_tan убила мне целый день
    // из-за этого движок зависал на реальном железе
    //tan_half_fov = fast_tan(fov_rad * 0.5f);
    tan_half_fov = tan(fov_rad * 0.5f);
    
    //if (tan_half_fov <= 0.01f) return;

    //tan_half_fov = fast_tan(fov_rad * 1.5f);

    // Переводим из -1, 1 в ширину и высоту экрана
    //y_scale = 1.0f / tan_half_fov;
    // y_scale = tan_half_fov;
    // x_scale = y_scale / aspect;

    x_scale = 1.0f / tan_half_fov;
    y_scale = x_scale * aspect;
    
    // Помним, что порядок Row Major
    SET_MATRIX_VALUE(projection, 0, 0, x_scale);
    SET_MATRIX_VALUE(projection, 1, 1, y_scale);
    SET_MATRIX_VALUE(projection, 2, 2, far_clip / (far_clip - near_clip));
    SET_MATRIX_VALUE(projection, 2, 3, 1.0f);
    SET_MATRIX_VALUE(projection, 3, 2, -near_clip * far_clip / (far_clip - near_clip));
    SET_MATRIX_VALUE(projection, 3, 3, 0.0f);

    MatrixMultiply(matrix, matrix, projection);
}

// Матрица вида (LookAt) — камера
void MatrixLookAt(float* m,
                  float camera_pos_x, float camera_pos_y, float camera_pos_z,
                  float look_at_x,  float look_at_y,  float look_at_z,
                  float up_direction_x,  float up_direction_y,  float up_direction_z)
{
    float forward[3], side[3], up[3];
    float len;

    // Заполняем матрицу вида
    MatrixIdentity(m);
    //MatrixTranslation(m, -camera_pos_x, -camera_pos_y, -camera_pos_z);
    //MatrixTranslation(m, camera_pos_x, camera_pos_y, camera_pos_z);

    // Вычисляем направление взгляда (forward = normalize(at - eye))
    forward[0] = look_at_x - camera_pos_x;
    forward[1] = look_at_y - camera_pos_y;
    forward[2] = look_at_z - camera_pos_z;

    len = sqrt( forward[0]*forward[0] 
              + forward[1]*forward[1] 
              + forward[2]*forward[2]);
    if (len > 0.00001f) {
        forward[0] /= len;
        forward[1] /= len;
        forward[2] /= len;
    }

    // Вычисляем правую ось (side = normalize(cross(forward, up)))
    side[0] = forward[1] * up_direction_z - forward[2] * up_direction_y;
    side[1] = forward[2] * up_direction_x - forward[0] * up_direction_z;
    side[2] = forward[0] * up_direction_y - forward[1] * up_direction_x;

    len = sqrt(side[0]*side[0] 
             + side[1]*side[1] 
             + side[2]*side[2]);
    if (len > 0.00001f) {
        side[0] /= len;
        side[1] /= len;
        side[2] /= len;
    }

    // Пересчитываем up = cross(side, forward)
    up[0] = side[1] * forward[2] - side[2] * forward[1];
    up[1] = side[2] * forward[0] - side[0] * forward[2];
    up[2] = side[0] * forward[1] - side[1] * forward[0];

    

    SET_MATRIX_VALUE(m, 0, 0, side[0]);
    SET_MATRIX_VALUE(m, 0, 1, side[1]);
    SET_MATRIX_VALUE(m, 0, 2, side[2]);

    SET_MATRIX_VALUE(m, 1, 0, up[0]);
    SET_MATRIX_VALUE(m, 1, 1, up[1]);
    SET_MATRIX_VALUE(m, 1, 2, up[2]);

    SET_MATRIX_VALUE(m, 2, 0, -forward[0]);
    SET_MATRIX_VALUE(m, 2, 1, -forward[1]);
    SET_MATRIX_VALUE(m, 2, 2, -forward[2]);
    // SET_MATRIX_VALUE(m, 2, 0, forward[0]);
    // SET_MATRIX_VALUE(m, 2, 1, forward[1]);
    // SET_MATRIX_VALUE(m, 2, 2, forward[2]);

    // Перенос камеры в начало координат
    MatrixTranslation(m, -camera_pos_x, -camera_pos_y, -camera_pos_z);
    //MatrixTranslation(m, camera_pos_x, camera_pos_y, camera_pos_z);

    // === ТРАНСЛЯЦИЯ: view = Translate(-eye) * Rotation ===
    // last row = [ -eye·side, -eye·up, eye·forward, 1 ]
    // SET_MATRIX_VALUE(m, 0, 3, 0.0f);
    // SET_MATRIX_VALUE(m, 1, 3, 0.0f);
    // SET_MATRIX_VALUE(m, 2, 3, 0.0f);
    // SET_MATRIX_VALUE(m, 3, 0, -(side[0]    * camera_pos_x   
    //                           + side[1]    * camera_pos_y 
    //                           + side[2]    * camera_pos_z));
    // SET_MATRIX_VALUE(m, 3, 1, -(up[0]      * camera_pos_x   
    //                           + up[1]      * camera_pos_y 
    //                           + up[2]      * camera_pos_z));
    // SET_MATRIX_VALUE(m, 3, 2,  (forward[0] * camera_pos_x   
    //                           + forward[1] * camera_pos_y 
    //                           + forward[2] * camera_pos_z));
    // SET_MATRIX_VALUE(m, 3, 3, 1.0f);
    
    // SET_MATRIX_VALUE(m, 3, 0, -camera_pos_x);
    // SET_MATRIX_VALUE(m, 3, 1, -camera_pos_y);
    // SET_MATRIX_VALUE(m, 3, 2, -camera_pos_z);
    // SET_MATRIX_VALUE(m, 3, 3, 1.0f);
}

// Новая, исправленная версия MatrixLookAt для Row-Major и левосторонней системы координат
void MatrixLookAt2(float* m,
                  float eye_x, float eye_y, float eye_z,
                  float at_x,  float at_y,  float at_z,
                  float up_x,  float up_y,  float up_z)
{
    float forward[3], side[3], up[3];
    float len;

    // 1. Вычисляем forward (направление взгляда)
    // В левосторонней системе камера смотрит вдоль +forward
    forward[0] = at_x - eye_x;
    forward[1] = at_y - eye_y;
    forward[2] = at_z - eye_z;
    
    len = sqrt(forward[0]*forward[0] + forward[1]*forward[1] + forward[2]*forward[2]);
    if (len > 0.00001f) {
        forward[0] /= len; forward[1] /= len; forward[2] /= len;
    }

    // 2. Вычисляем side (право) как cross(up, forward)
    side[0] = up_y * forward[2] - up_z * forward[1];
    side[1] = up_z * forward[0] - up_x * forward[2];
    side[2] = up_x * forward[1] - up_y * forward[0];
    
    len = sqrt(side[0]*side[0] + side[1]*side[1] + side[2]*side[2]);
    if (len > 0.00001f) {
        side[0] /= len; side[1] /= len; side[2] /= len;
    }

    // 3. Пересчитываем up как cross(forward, side)
    up[0] = forward[1] * side[2] - forward[2] * side[1];
    up[1] = forward[2] * side[0] - forward[0] * side[2];
    up[2] = forward[0] * side[1] - forward[1] * side[0];

    // 4. Строим матрицу вида (Row-Major, Left-Handed)
    MatrixIdentity(m);
    
    // Ось X (side)
    SET_MATRIX_VALUE(m, 0, 0, side[0]);
    SET_MATRIX_VALUE(m, 1, 0, side[1]);
    SET_MATRIX_VALUE(m, 2, 0, side[2]);
    
    // Ось Y (up)
    SET_MATRIX_VALUE(m, 0, 1, up[0]);
    SET_MATRIX_VALUE(m, 1, 1, up[1]);
    SET_MATRIX_VALUE(m, 2, 1, up[2]);
    
    // Ось Z (forward) - ВАЖНО: знак "+" для левосторонней системы
    SET_MATRIX_VALUE(m, 0, 2, forward[0]);
    SET_MATRIX_VALUE(m, 1, 2, forward[1]);
    SET_MATRIX_VALUE(m, 2, 2, forward[2]);
    
    // 5. Перенос (Translate)
    // Вычисляем перенос как отрицательное скалярное произведение позиции на оси
    SET_MATRIX_VALUE(m, 3, 0, -(side[0]    * eye_x + side[1]    * eye_y + side[2]    * eye_z));
    SET_MATRIX_VALUE(m, 3, 1, -(up[0]      * eye_x + up[1]      * eye_y + up[2]      * eye_z));
    SET_MATRIX_VALUE(m, 3, 2, -(forward[0] * eye_x + forward[1] * eye_y + forward[2] * eye_z));
    SET_MATRIX_VALUE(m, 3, 3, 1.0f);
}

// Отладочный вывод матрицы
void MatrixPrint(const float* m)
{
    int i, j;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            printf("%8.4f ", GET_MATRIX_VALUE(m, i, j));
        }
        printf("\n");
    }
    printf("\n");
}

// Применение матрицы на вертекс
void ApplyMatrix(GrVertex* v, const float* mat)
{
    float x, y, z, w;
    float new_x, new_y, new_z, new_w;
    float prevZ = v->z; // Сохраняем, чтобы пересчитать в конце
    
    x = v->x;
    y = v->y;
    z = v->z;
    w = 1.0f;
    //w = 1.0f / v->oow;
    //w = v->w;
    
    // row-major
    new_x = GET_MATRIX_VALUE(mat, 0, 0)*x + GET_MATRIX_VALUE(mat, 1, 0)*y + GET_MATRIX_VALUE(mat, 2, 0)*z + GET_MATRIX_VALUE(mat, 3, 0)*w;
    new_y = GET_MATRIX_VALUE(mat, 0, 1)*x + GET_MATRIX_VALUE(mat, 1, 1)*y + GET_MATRIX_VALUE(mat, 2, 1)*z + GET_MATRIX_VALUE(mat, 3, 1)*w;
    new_z = GET_MATRIX_VALUE(mat, 0, 2)*x + GET_MATRIX_VALUE(mat, 1, 2)*y + GET_MATRIX_VALUE(mat, 2, 2)*z + GET_MATRIX_VALUE(mat, 3, 2)*w;
    new_w = GET_MATRIX_VALUE(mat, 0, 3)*x + GET_MATRIX_VALUE(mat, 1, 3)*y + GET_MATRIX_VALUE(mat, 2, 3)*z + GET_MATRIX_VALUE(mat, 3, 3)*w;

    //v->oow = (new_w != 0.0f) ? 1.0f / new_w : 65535.0f;   // защита от деления на 0

    // Для экранных координат — делим только если w > 0 (иначе клиппинг)
    // if (new_w > 0.0001f) {
    //     v->x = new_x / new_w;
    //     v->y = new_y / new_w;
    //     v->z = new_z / new_w;          // это уже не используется hardware, можно оставить
    // } else {
    //     // вершина за камерой — помечаем как невидимую
    //     v->x = 0.0f;
    //     v->y = 0.0f;
    //     v->z = 0.0f;
    //     v->oow = -1.0f;                // специальный маркер
    // }

    //if (new_w < 0) new_w = -new_w;

    if (new_w <= 0.0001f) {
        // Вершина за камерой — пропускаем
        v->oow = -1.0f;
        return;
    }

    v->x = new_x / new_w;
    v->y = new_y / new_w;
    v->z = new_z / new_w;
    v->oow = 1.0f / new_w;

    // //v->oow = new_w;
    // v->oow = 1.0f;
    // //v->ooz = (prevZ > 0.0f) ? 65535.0f / prevZ : 65535.0f;
    //v->ooz = 1.0f / new_w;
    // v->x = new_x;
    // v->y = new_y;
    // v->z = new_z;
    //v->oow = 1.0f / new_w;
    //v->z = 1.0f / v->ooz;
}

void VertexToScreen(GrVertex* inputVertex, GrVertex* outputVertex)
{
    GrVertex tmp;
    int i;

    memcpy(&tmp, inputVertex, sizeof(GrVertex));

//    outputVertex->x = X_TO_SCREEN(tmp.x / tmp.z, SCREEN_WIDTH);
//    outputVertex->y = Y_TO_SCREEN(tmp.y / tmp.z, SCREEN_HEIGHT);

    // У нас уже есть деление на W, на Z делить уже не надо
    outputVertex->x = X_TO_SCREEN(tmp.x, SCREEN_WIDTH);
    outputVertex->y = Y_TO_SCREEN(tmp.y, SCREEN_HEIGHT);
    outputVertex->z = tmp.z;
    //outputVertex->ooz = (tmp.z > 0.1f) ? 65535.0f / tmp.z : 65535.0f;
    outputVertex->ooz = tmp.ooz;
    outputVertex->oow = tmp.oow;
    //outputVertex->oow = 1.0f;

    // Копируем текстурные координаты (уже корректные sow/tow)
    // for (i = 0; i < GLIDE_NUM_TMU; i++) {
    //     outputVertex->tmuvtx[i].sow = tmp.tmuvtx[i].sow;
    //     outputVertex->tmuvtx[i].tow = tmp.tmuvtx[i].tow;
    // }
    outputVertex->tmuvtx[0].sow = tmp.tmuvtx[0].sow;
    outputVertex->tmuvtx[0].tow = tmp.tmuvtx[0].tow;
}

void VertexToScreen2(GrVertex* outputVertex)
{
    GrVertex tmp;
    int i;

    memcpy(&tmp, outputVertex, sizeof(GrVertex));

//    outputVertex->x = X_TO_SCREEN(tmp.x / tmp.z, SCREEN_WIDTH);
//    outputVertex->y = Y_TO_SCREEN(tmp.y / tmp.z, SCREEN_HEIGHT);

    // У нас уже есть деление на W, на Z делить уже не надо
    outputVertex->x = X_TO_SCREEN(tmp.x, SCREEN_WIDTH);
    outputVertex->y = Y_TO_SCREEN(tmp.y, SCREEN_HEIGHT);
    outputVertex->z = tmp.z;
    //outputVertex->ooz = (tmp.z > 0.1f) ? 65535.0f / tmp.z : 65535.0f;
    outputVertex->ooz = tmp.ooz;
    outputVertex->oow = tmp.oow;

    // Копируем текстурные координаты (уже корректные sow/tow)
    // for (i = 0; i < GLIDE_NUM_TMU; i++) {
    //     outputVertex->tmuvtx[i].sow = tmp.tmuvtx[i].sow;
    //     outputVertex->tmuvtx[i].tow = tmp.tmuvtx[i].tow;
    // }
    outputVertex->tmuvtx[0].sow = tmp.tmuvtx[0].sow;
    outputVertex->tmuvtx[0].tow = tmp.tmuvtx[0].tow;
}

