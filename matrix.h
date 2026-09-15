#ifndef MATRIX_H
#define MATRIX_H

#define SET_MATRIX_VALUE(m, row, col, val) ((m)[(row)*4 + (col)] = (val))
#define GET_MATRIX_VALUE(m, row, col)      ((m)[(row)*4 + (col)])

#define DEGTORAD (3.141592653589793f / 180.0f)
#define RADTODEG (180.0f / 3.141592653589793f)

#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 480

#define X_TO_SCREEN(srcX, width)   ((width / 2.0f) + (srcX * (width / 2.0f)))
#define Y_TO_SCREEN(srcY, height)   ((height / 2.0f) - (srcY * (height / 2.0f)))

#include "glide.h"

void MatrixIdentity     (float* m);
void MatrixMultiply     (float* out, const float* a, const float* b); // C = A x B
void MatrixTranslation  (float* m, float x, float y, float z);
void MatrixRotationX    (float* m, float angle_rad);
void MatrixRotationY    (float* m, float angle_rad);
void MatrixRotationZ    (float* m, float angle_rad);
void MatrixEulerRotation     (float* m, float x, float y, float z); // Euler rotation
void MatrixScale        (float* m, float sx, float sy, float sz); // Scaling
void MatrixProjection  (float* m, float fov_deg, float aspect, float near_clip, float far_clip);

// Camera
void MatrixLookAt(float* m,
                  float camera_pos_x, float camera_pos_y, float camera_pos_z,
                  float look_at_x,  float look_at_y,  float look_at_z,
                  float up_direction_x,  float up_direction_y,  float up_direction_z);      // direction "up"
void MatrixLookAt2(float* m,
                  float eye_x, float eye_y, float eye_z,
                  float at_x,  float at_y,  float at_z,
                  float up_x,  float up_y,  float up_z);

void MatrixPrint        (const float* m); // Output matrix to console (for debugging)
void ApplyMatrix(GrVertex* v, const float* mat);
void VertexToScreen(GrVertex* inputVertex, GrVertex* outputVertex);
void VertexToScreen2(GrVertex* outputVertex);

#endif
