// camera.c - концепция игровой камеры в 3д-мире
#include <math.h>

#include "matrix.h"
#include "fastmath.h"
#include "camera.h"

// Камера одна и глобальная
Camera camera;
FrustumPlanes frustum;

// Матрица вида тоже одна и тоже глобальная
float view[16];
float proj[16];

// Задаём начальные параметры камеры
void SetupCamera(
    float pos_x, float pos_y, float pos_z,
    float pitch, float yaw, float roll,
    float fov, float aspect,
    float near_clip, float far_clip
) {
    
    // float default_camera_yaw   = 180.0f;  // я так и не понял, почему только при этих
    // float default_camera_pitch = -4.0f;   // значениях камера смотрит на +Z
    // camera.yaw   = 180.0f;   // поворот по горизонтали (влево-вправо)
    // camera.pitch = -4.0f;   // поворот по вертикали (вверх-вниз)
    // camera.translation_speed = 40.0f;
    // camera.rotation_speed = 90.0f;
    camera.pos_x = pos_x;
    camera.pos_y = pos_y;
    camera.pos_z = pos_z;
    camera.pitch = pitch;
    camera.yaw = yaw;
    camera.roll = roll;
    camera.fov = fov;
    camera.aspect = aspect;
    camera.near_clip = near_clip;
    camera.far_clip = far_clip;

    quat_from_euler(&camera.orientation,
        pitch * DEGTORAD,
        yaw   * DEGTORAD,
        roll  * DEGTORAD);
}

// Вычисление плоскостей frustum из матрицы проекции
void ExtractFrustumPlanes(FrustumPlanes* planes, float* proj_matrix) {
    const float* m = proj_matrix;
    int i;
    float far_len;
    
    // Для Row-Major матрицы, индексы: m[строка * 4 + столбец]
    // Извлекаем плоскости из матрицы проекции
    
    // Near plane: строка 3 + строка 2
    planes->near_plane[0] = m[12] + m[8];   // m[3][0] + m[2][0]
    planes->near_plane[1] = m[13] + m[9];   // m[3][1] + m[2][1]
    planes->near_plane[2] = m[14] + m[10];  // m[3][2] + m[2][2]
    planes->near_plane[3] = m[15] + m[11];  // m[3][3] + m[2][3]
    
    // Far plane: строка 3 - строка 2
    planes->far_plane[0] = m[12] - m[8];
    planes->far_plane[1] = m[13] - m[9];
    planes->far_plane[2] = m[14] - m[10];
    planes->far_plane[3] = m[15] - m[11];

    // Left plane: строка 3 + строка 0
    planes->left_plane[0] = m[12] + m[0];
    planes->left_plane[1] = m[13] + m[1];
    planes->left_plane[2] = m[14] + m[2];
    planes->left_plane[3] = m[15] + m[3];
    
    // Right plane: строка 3 - строка 0
    planes->right_plane[0] = m[12] - m[0];
    planes->right_plane[1] = m[13] - m[1];
    planes->right_plane[2] = m[14] - m[2];
    planes->right_plane[3] = m[15] - m[3];
    
    // Bottom plane: строка 3 + строка 1
    planes->bottom_plane[0] = m[12] + m[4];
    planes->bottom_plane[1] = m[13] + m[5];
    planes->bottom_plane[2] = m[14] + m[6];
    planes->bottom_plane[3] = m[15] + m[7];
    
    // Top plane: строка 3 - строка 1
    planes->top_plane[0] = m[12] - m[4];
    planes->top_plane[1] = m[13] - m[5];
    planes->top_plane[2] = m[14] - m[6];
    planes->top_plane[3] = m[15] - m[7];
    


    // Нормализуем плоскости
    for (i = 0; i < 6; i++) {
        float* p = (float*)planes + i * 4;
        float len = sqrt(p[0]*p[0] + p[1]*p[1] + p[2]*p[2]);
        if (len > 0.0001f) {
            p[0] /= len; p[1] /= len; p[2] /= len; p[3] /= len;
        }
    }
}

// void UpdateView() {
//     // Матрица камеры
//     //float view[16];
//     //float camera_pos_x, camera_pos_y, camera_pos_z;
//     float yaw_rad, pitch_rad, roll_rad;
//     float at_x, at_y, at_z;
//     float dir_x, dir_y, dir_z;   // направление взгляда
//     //float world_up_x, world_up_y, world_up_z;
//     float cos_theta, sin_theta;
//     float dot;
//     float up_x, up_y, up_z;
//     float cross_x, cross_y, cross_z;
//     // Ограничиваем pitch, чтобы камера не перевернулась
//     //if (camera.pitch > 89.0f)  camera.pitch = -89.0f;
//     //if (camera.pitch < -89.0f) camera.pitch = 89.0f;
//     // Перевод в радианы
//     yaw_rad   = camera.yaw   * DEGTORAD;
//     pitch_rad = camera.pitch * DEGTORAD;
//     roll_rad  = camera.roll  * DEGTORAD;
//     // Вычисляем направление взгляда
//     dir_x = fast_cos(pitch_rad) * fast_sin(yaw_rad);
//     dir_y = fast_sin(pitch_rad);
//     dir_z = fast_cos(pitch_rad) * fast_cos(yaw_rad);
//     // Точка, куда смотрит камера = позиция + направление * дистанция
//     at_x = camera.pos_x + dir_x;
//     at_y = camera.pos_y + dir_y;
//     at_z = camera.pos_z + dir_z;
//     // at_x = dir_x;
//     // at_y = dir_y;
//     // at_z = dir_z;
//     // world_up — это то, что было раньше (0, 1, 0)
//     up_x = 0.0f;
//     up_y = 1.0f;
//     up_z = 0.0f;
//     // Rodrigues' rotation formula: поворачиваем world_up вокруг dir на угол roll
//     cos_theta = fast_cos(roll_rad);
//     sin_theta = fast_sin(roll_rad);
//     // dot = world_up • dir
//     dot = dir_x * up_x + dir_y * up_y + dir_z * up_z;  // = dir_y
//     // cross = dir × world_up
//     cross_x = dir_y * up_z - dir_z * up_y;
//     cross_y = dir_z * up_x - dir_x * up_z;
//     cross_z = dir_x * up_y - dir_y * up_x;
//     // rotated_up = world_up * cos + (dir × world_up) * sin + dir * dot * (1 - cos)
//     up_x = up_x * cos_theta + cross_x * sin_theta + dir_x * dot * (1.0f - cos_theta);
//     up_y = up_y * cos_theta + cross_y * sin_theta + dir_y * dot * (1.0f - cos_theta);
//     up_z = up_z * cos_theta + cross_z * sin_theta + dir_z * dot * (1.0f - cos_theta);
//     // Строим View-матрицу
//     //MatrixIdentity(view);
//     // MatrixTranslation(view,                    // результат
//     //     camera.pos_x, camera.pos_y, camera.pos_z);
//     //     MatrixTranslation(view,                    // результат
//     //     camera.pos_x, camera.pos_y, camera.pos_z);
//     MatrixLookAt2(view,                    // результат
//         camera.pos_x, camera.pos_y, camera.pos_z,   // eye
//         at_x, at_y, at_z,                              // at
//         up_x, up_y, up_z);
// }

// Обновляем матрицу вида с учётом текущего положения и поворота камеры
void UpdateMatricesViewProj() {
    // Из кватерниона получаем три локальные оси (левосторонняя система)
    // Начинаем с базовых осей
    float forward_x = 0.0f, forward_y = 0.0f, forward_z = 1.0f;   // +Z вперёд
    float right_x   = 1.0f, right_y   = 0.0f, right_z   = 0.0f;   // +X вправо
    float up_x      = 0.0f, up_y      = 1.0f, up_z      = 0.0f;   // +Y вверх
    
    quat_rotate_vector(&forward_x, &forward_y, &forward_z, &camera.orientation);
    quat_rotate_vector(&right_x,   &right_y,   &right_z,   &camera.orientation);
    quat_rotate_vector(&up_x,      &up_y,      &up_z,      &camera.orientation);
    
    // Строим матрицу вида (Row-major, Left-Handed)
    // Оси камеры записываются в столбцы (т.к. row-major)
    MatrixIdentity(view);
    
    SET_MATRIX_VALUE(view, 0, 0, right_x);
    SET_MATRIX_VALUE(view, 1, 0, right_y);
    SET_MATRIX_VALUE(view, 2, 0, right_z);
    
    SET_MATRIX_VALUE(view, 0, 1, up_x);
    SET_MATRIX_VALUE(view, 1, 1, up_y);
    SET_MATRIX_VALUE(view, 2, 1, up_z);
    
    // Для левосторонней системы вперёд = +forward
    SET_MATRIX_VALUE(view, 0, 2, forward_x);
    SET_MATRIX_VALUE(view, 1, 2, forward_y);
    SET_MATRIX_VALUE(view, 2, 2, forward_z);
    
    // Перенос = -dot(eye, axis)
    SET_MATRIX_VALUE(view, 3, 0, -(right_x   * camera.pos_x + right_y   * camera.pos_y + right_z   * camera.pos_z));
    SET_MATRIX_VALUE(view, 3, 1, -(up_x      * camera.pos_x + up_y      * camera.pos_y + up_z      * camera.pos_z));
    SET_MATRIX_VALUE(view, 3, 2, -(forward_x * camera.pos_x + forward_y * camera.pos_y + forward_z * camera.pos_z));
    SET_MATRIX_VALUE(view, 3, 3, 1.0f);

    // Просчитаем матрицу проекции тоже
    MatrixIdentity(proj);
    MatrixProjection(proj,
        camera.fov,          // FOV 60 градусов
        camera.aspect,       // aspect ratio 4:3
        camera.near_clip,    // near plane
        camera.far_clip);    // far plane

    ExtractFrustumPlanes(&frustum, proj);
}

void MoveCameraWorld(float move_delta_x, float move_delta_y, float move_delta_z) {
    camera.pos_x += move_delta_x;
    camera.pos_y += move_delta_y;
    camera.pos_z += move_delta_z;
}

// void MoveCameraLocal(float forwardDelta, float rightDelta, float upDelta) {
//     float yaw_rad, pitch_rad, len;
//     float forward_x, forward_y, forward_z;
//     float right_x, right_y, right_z;
//     float up_x, up_y, up_z;
//     yaw_rad   = camera.yaw   * DEGTORAD;
//     pitch_rad = camera.pitch * DEGTORAD;
//     // Направление "вперёд" в горизонтальной плоскости (игнорируем roll)
//     forward_x = fast_sin(yaw_rad) * fast_cos(pitch_rad);
//     forward_y = fast_sin(pitch_rad);
//     forward_z = fast_cos(yaw_rad) * fast_cos(pitch_rad);
//     // Вектор "вправо" = cross(forward, world_up) = cross(forward, (0,1,0))
//     right_x =  forward_z;               // cross product with (0,1,0) simplified
//     right_y =  0.0f;
//     right_z = -forward_x;
//     // Нормализуем right (он уже почти единичный, если forward единичный)
//     len = sqrt(right_x*right_x + right_y*right_y + right_z*right_z);
//     if (len > 0.00001f) {
//         right_x /= len; right_y /= len; right_z /= len;
//     }
//     // Вектор "вверх" (без учёта roll) = cross(right, forward)
//     up_x = right_y * forward_z - right_z * forward_y;
//     up_y = right_z * forward_x - right_x * forward_z;
//     up_z = right_x * forward_y - right_y * forward_x;
//     // Уже единичный, если right и forward ортогональны и единичны
//     // Применяем смещения
//     camera.pos_x += forward_x * forwardDelta + right_x * rightDelta + up_x * upDelta;
//     camera.pos_y += forward_y * forwardDelta + right_y * rightDelta + up_y * upDelta;
//     camera.pos_z += forward_z * forwardDelta + right_z * rightDelta + up_z * upDelta;
// }

// void MoveCameraLocal(float forwardDelta, float rightDelta, float upDelta) {
//     float yaw_rad, pitch_rad, roll_rad, len;
//     float forward_x, forward_y, forward_z;
//     float right_x, right_y, right_z;
//     float up_x, up_y, up_z;
//     float fx, fy, fz;
//     float rx0, ry0, rz0;
//     float ux0, uy0, uz0;
//     float rx, ry, rz;
//     float ux, uy, uz;
//     float dot, sin_r, cos_r;
//     yaw_rad   = camera.yaw   * DEGTORAD;
//     pitch_rad = camera.pitch * DEGTORAD;
//     roll_rad  = camera.roll  * DEGTORAD;
//     // 1. Направление вперёд (уже единичное)
//     fx = fast_sin(yaw_rad) * fast_cos(pitch_rad);
//     fy = fast_sin(pitch_rad);
//     fz = fast_cos(yaw_rad) * fast_cos(pitch_rad);
//     // 2. Базовые векторы без roll (мировая вертикаль (0,1,0))
//     // right0 = cross(forward, (0,1,0))
//     rx0 =  fz;
//     ry0 =  0.0f;
//     rz0 = -fx;
//     // Нормализуем (forward и world_up не коллинеарны при pitch != ±90°)
//     len = sqrt(rx0*rx0 + ry0*ry0 + rz0*rz0);
//     if (len > 0.00001f) {
//         rx0 /= len; ry0 /= len; rz0 /= len;
//     } else {
//         // Особый случай: смотрим строго вверх/вниз – тогда right0 возьмём (1,0,0)
//         rx0 = 1.0f; ry0 = 0.0f; rz0 = 0.0f;
//     }
//     // up0 = cross(right0, forward)
//     ux0 = ry0 * fz - rz0 * fy;
//     uy0 = rz0 * fx - rx0 * fz;
//     uz0 = rx0 * fy - ry0 * fx;
//     // 3. Поворот right0 и up0 вокруг forward на угол roll (формула Родригеса)
//     cos_r = fast_cos(roll_rad);
//     sin_r = fast_sin(roll_rad);
//     // --- Поворачиваем right0 ---
//     // dot = right0 · forward  (должен быть 0, но вычислим для общности)
//     dot = rx0 * fx + ry0 * fy + rz0 * fz;
//     rx = rx0 * cos_r + (fy * rz0 - fz * ry0) * sin_r + fx * dot * (1.0f - cos_r);
//     ry = ry0 * cos_r + (fz * rx0 - fx * rz0) * sin_r + fy * dot * (1.0f - cos_r);
//     rz = rz0 * cos_r + (fx * ry0 - fy * rx0) * sin_r + fz * dot * (1.0f - cos_r);
//     // --- Поворачиваем up0 ---
//     dot = ux0 * fx + uy0 * fy + uz0 * fz;
//     ux = ux0 * cos_r + (fy * uz0 - fz * uy0) * sin_r + fx * dot * (1.0f - cos_r);
//     uy = uy0 * cos_r + (fz * ux0 - fx * uz0) * sin_r + fy * dot * (1.0f - cos_r);
//     uz = uz0 * cos_r + (fx * uy0 - fy * ux0) * sin_r + fz * dot * (1.0f - cos_r);
//     // 4. Применяем смещения
//     camera.pos_x += fx * forwardDelta + rx * rightDelta + ux * upDelta;
//     camera.pos_y += fy * forwardDelta + ry * rightDelta + uy * upDelta;
//     camera.pos_z += fz * forwardDelta + rz * rightDelta + uz * upDelta;
// }

// Поворот камеры вокруг точки начала координат в градусах
void RotateCameraAroundWorld(float rotate_delta_pitch, float rotate_delta_yaw, float rotate_delta_roll) {
    camera.pitch += rotate_delta_pitch;
    camera.yaw += rotate_delta_yaw;
    camera.roll += rotate_delta_roll;
}

// Поворот камеры вокруг её самой в кватернионах
void RotateCameraAroundLocal(float pitch_delta, float yaw_delta, float roll_delta) {
    float pitch_rad, yaw_rad, roll_rad;
    Quaternion delta, new_orient;

    // Переводим дельты в радианы
    pitch_rad = pitch_delta * DEGTORAD;
    yaw_rad = yaw_delta   * DEGTORAD;
    roll_rad = roll_delta  * DEGTORAD;
    
    // Создаём кватернион изменения: порядок YXZ (yaw, pitch, roll)
    quat_from_euler(&delta, pitch_rad, yaw_rad, roll_rad);
    
    // Применяем к текущей ориентации
    quat_multiply(&new_orient, &camera.orientation, &delta);
    camera.orientation = new_orient;
    quat_normalize(&camera.orientation);
}

// Движение камеры с учётом её вращения в кватернионах
void MoveCameraLocal(float forwardDelta, float rightDelta, float upDelta) {
    // Получаем текущие направления из кватерниона
    float fx = 0.0f, fy = 0.0f, fz = 1.0f;   // forward
    float rx = 1.0f, ry = 0.0f, rz = 0.0f;   // right
    float ux = 0.0f, uy = 1.0f, uz = 0.0f;   // up
    
    quat_rotate_vector(&fx, &fy, &fz, &camera.orientation);
    quat_rotate_vector(&rx, &ry, &rz, &camera.orientation);
    quat_rotate_vector(&ux, &uy, &uz, &camera.orientation);
    
    camera.pos_x += fx * forwardDelta + rx * rightDelta + ux * upDelta;
    camera.pos_y += fy * forwardDelta + ry * rightDelta + uy * upDelta;
    camera.pos_z += fz * forwardDelta + rz * rightDelta + uz * upDelta;
}
