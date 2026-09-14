// FASTMATH.H - заголовочные для математики

#ifndef FASTMATH_H
#define FASTMATH_H

#define LOBYTE(w) ((unsigned char)((unsigned short)(w) & 0xFF))
#define HIBYTE(w) ((unsigned char)(((unsigned short)(w) >> 8)) & 0xFF)

// Когда ставил 8192, висло на железе
#define FASTMATH_TABLE_SIZE 4096

#define FASTMATH_MASK (FASTMATH_TABLE_SIZE - 1)
#define FASTMATH_TWO_PI     6.283185307179586f
#define FASTMATH_PI         3.141592653589793f
#define FASTMATH_HALF_PI    1.570796326794897f

typedef struct {
    float x, y, z, w;   // кватернион ориентации
} Quaternion;

extern float _sin_tab[FASTMATH_TABLE_SIZE];
extern float _tan_tab[FASTMATH_TABLE_SIZE];

void init_fast_math();

// Быстрые функции (принимают радианы)
float fast_sin(float rad);
float fast_cos(float rad);
//float fast_tan(float rad); // На реальном железе виснет

void quat_identity(Quaternion* q);
void quat_multiply(Quaternion* out, const Quaternion* a, const Quaternion* b);
void quat_normalize(Quaternion* q);
void quat_rotate_vector(float* vx, float* vy, float* vz, const Quaternion* q);
void quat_from_euler(Quaternion* q, float pitch, float yaw, float roll);


#endif
