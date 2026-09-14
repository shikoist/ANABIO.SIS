// camera.h - заголовочный файл: концепция игровой камеры в 3д-мире
#ifndef CAMERA_H
#define CAMERA_H

#include "fastmath.h"

#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 480


typedef struct {
    float       pos_x;
    float       pos_y;
    float       pos_z;
    Quaternion  orientation;
    float       pitch;
    float       yaw;
    float       roll;
    float       fov;
    float       aspect;
    float       near_clip;
    float       far_clip;
    int         wireframe_mode;
} Camera;

typedef struct {
    float near_plane[4];
    float left_plane[4];
    float right_plane[4];
    float bottom_plane[4];
    float top_plane[4];
    float far_plane[4];
} FrustumPlanes;

extern float view[16];
extern float proj[16];
extern Camera camera;
extern FrustumPlanes frustum;

void ExtractFrustumPlanes(FrustumPlanes* planes, float* proj_matrix);

void SetupCamera(
    float pos_x, float pos_y, float pos_z,
    float pitch, float yaw, float roll,
    float fov, float aspect,
    float near_clip, float far_clip);
void UpdateMatricesViewProj();
void UpdateMatricesViewProj();
void MoveCameraWorld(float move_delta_x, float move_delta_y, float move_delta_z);
void RotateCameraAroundWorld(float rotate_delta_pitch, float rotate_delta_yaw, float rotate_delta_roll);
void RotateCameraAroundLocal(float pitch_delta, float yaw_delta, float roll_delta);
void MoveCameraLocal(float forwardDelta, float rightDelta, float upDelta);
void MoveCameraLocal(float forwardDelta, float rightDelta, float upDelta);
void MoveCameraLocal(float forwardDelta, float rightDelta, float upDelta);

#endif
