// FASTMATH.C - быстрые математические функции
#include <math.h>

#include "FASTMATH.H"

float _sin_tab[FASTMATH_TABLE_SIZE];
float _tan_tab[FASTMATH_TABLE_SIZE];
const float rad_to_idx = (float)FASTMATH_TABLE_SIZE / FASTMATH_TWO_PI;

// Вызвать один раз в main()
void init_fast_math()
{
    int i;
    for (i = 0; i < FASTMATH_TABLE_SIZE; i++) {
        double rad = (double)i * (FASTMATH_TWO_PI / FASTMATH_TABLE_SIZE);
        _sin_tab[i] = (float)sin(rad);
        _tan_tab[i] = (float)tan(rad);
    }
}

float fast_sin(float rad) {
    // Приводим радианы к положительному индексу и используем маску для зацикливания
    int idx = (int)(rad * rad_to_idx) & FASTMATH_MASK;
    return _sin_tab[idx];
}

float fast_cos(float rad) {
    // cos(x) = sin(x + PI/2)
    int idx = (int)((rad + FASTMATH_HALF_PI) * rad_to_idx) & FASTMATH_MASK;
    return _sin_tab[idx];
}

// Не использовать для реального железа - виснет. Лучше tan() из math.h
// float fast_tan(float rad) {
//     int idx = (int)(rad * rad_to_idx) & FASTMATH_MASK;
//     return _tan_tab[idx];
// }

// Кватернионные утилиты
void quat_identity(Quaternion* q) {
    q->x = 0.0f; q->y = 0.0f; q->z = 0.0f; q->w = 1.0f;
}

void quat_multiply(Quaternion* out, const Quaternion* a, const Quaternion* b) {
    out->x = a->w*b->x + a->x*b->w + a->y*b->z - a->z*b->y;
    out->y = a->w*b->y - a->x*b->z + a->y*b->w + a->z*b->x;
    out->z = a->w*b->z + a->x*b->y - a->y*b->x + a->z*b->w;
    out->w = a->w*b->w - a->x*b->x - a->y*b->y - a->z*b->z;
}

void quat_normalize(Quaternion* q) {
    float len, inv;

    len = sqrt(q->x*q->x + q->y*q->y + q->z*q->z + q->w*q->w);
    if (len > 0.00001f) {
        inv = 1.0f / len;
        q->x *= inv; q->y *= inv; q->z *= inv; q->w *= inv;
    } else {
        quat_identity(q);
    }
}

// Поворот вектора v кватернионом q
void quat_rotate_vector(float* vx, float* vy, float* vz, const Quaternion* q) {
    // t = 2 * cross(q.xyz, v)
    float tx, ty, tz;

    tx = 2.0f * (q->y * (*vz) - q->z * (*vy));
    ty = 2.0f * (q->z * (*vx) - q->x * (*vz));
    tz = 2.0f * (q->x * (*vy) - q->y * (*vx));
    
    // v' = v + q.w * t + cross(q.xyz, t)
    *vx += q->w * tx + (q->y * tz - q->z * ty);
    *vy += q->w * ty + (q->z * tx - q->x * tz);
    *vz += q->w * tz + (q->x * ty - q->y * tx);
}

// Создать кватернион из углов Эйлера (в радианах), порядок YXZ
void quat_from_euler(Quaternion* q, float pitch, float yaw, float roll) {
    float cy, sy, cp, sp, cr, sr;
    Quaternion qy, qp, qr, temp;

    cy = fast_cos(yaw   * 0.5f);
    sy = fast_sin(yaw   * 0.5f);
    cp = fast_cos(pitch * 0.5f);
    sp = fast_sin(pitch * 0.5f);
    cr = fast_cos(roll  * 0.5f);
    sr = fast_sin(roll  * 0.5f);
    
    // Композиция: сначала yaw, затем pitch, затем roll (порядок важен)
    // Можно составить за один шаг, но проще перемножить три кватерниона.
    qy.x = 0.0f; qy.y = sy;   qy.z = 0.0f; qy.w = cy;    // поворот вокруг Y
    qp.x = sp;   qp.y = 0.0f; qp.z = 0.0f; qp.w = cp;    // вокруг X
    qr.x = 0.0f; qr.y = 0.0f; qr.z = sr;   qr.w = cr;    // вокруг Z
    
    quat_multiply(&temp, &qy, &qp);
    quat_multiply(q, &temp, &qr);
    quat_normalize(q);
}
