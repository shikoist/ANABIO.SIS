// camera.c - concept of the camera in a 3D world
#include <math.h>

#include "matrix.h"
#include "fastmath.h"
#include "camera.h"

// Camera one and global
Camera camera;
FrustumPlanes frustum;

// The viewing matrix is also one and global
float view[16];
float proj[16];

// Setting camera initial parameters
void SetupCamera(
    float pos_x, float pos_y, float pos_z,
    float pitch, float yaw, float roll,
    float fov, float aspect,
    float near_clip, float far_clip
) {
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

// Calculation of planes from the projection matrix
void ExtractFrustumPlanes(FrustumPlanes* planes, float* proj_matrix) {
    const float* m = proj_matrix;
    int i;
    float far_len;
    
    // For a matrix with a dominant row, indices: m[row * 4 + column]
    // Extracting planes from the projection matrix
    // Near plane: row 3 + row 2
    planes->near_plane[0] = m[12] + m[8];   // m[3][0] + m[2][0]
    planes->near_plane[1] = m[13] + m[9];   // m[3][1] + m[2][1]
    planes->near_plane[2] = m[14] + m[10];  // m[3][2] + m[2][2]
    planes->near_plane[3] = m[15] + m[11];  // m[3][3] + m[2][3]
    
    // Far plane: line 3 - line 2
    planes->far_plane[0] = m[12] - m[8];
    planes->far_plane[1] = m[13] - m[9];
    planes->far_plane[2] = m[14] - m[10];
    planes->far_plane[3] = m[15] - m[11];

    // Left plane: row 3 + row 0
    planes->left_plane[0] = m[12] + m[0];
    planes->left_plane[1] = m[13] + m[1];
    planes->left_plane[2] = m[14] + m[2];
    planes->left_plane[3] = m[15] + m[3];
    
    // Right plane: row 3 - row 0
    planes->right_plane[0] = m[12] - m[0];
    planes->right_plane[1] = m[13] - m[1];
    planes->right_plane[2] = m[14] - m[2];
    planes->right_plane[3] = m[15] - m[3];
    
    // Lower surface: row 3 + row 1
    planes->bottom_plane[0] = m[12] + m[4];
    planes->bottom_plane[1] = m[13] + m[5];
    planes->bottom_plane[2] = m[14] + m[6];
    planes->bottom_plane[3] = m[15] + m[7];
    
    // Upper surface: row 3 - row 1
    planes->top_plane[0] = m[12] - m[4];
    planes->top_plane[1] = m[13] - m[5];
    planes->top_plane[2] = m[14] - m[6];
    planes->top_plane[3] = m[15] - m[7];
    
    // Normalization of planes
    for (i = 0; i < 6; i++) {
        float* p = (float*)planes + i * 4;
        float len = sqrt(p[0]*p[0] + p[1]*p[1] + p[2]*p[2]);
        if (len > 0.0001f) {
            p[0] /= len; p[1] /= len; p[2] /= len; p[3] /= len;
        }
    }
}

// Updating the view matrix with the current camera position and rotation
void UpdateMatricesViewProj() {
    // From the quaternion, we obtain three local axes (left-handed system)
    // We start with the basic axes
    float forward_x = 0.0f, forward_y = 0.0f, forward_z = 1.0f;   // +Z вперед
    float right_x   = 1.0f, right_y   = 0.0f, right_z   = 0.0f;   // +X вправо
    float up_x      = 0.0f, up_y      = 1.0f, up_z      = 0.0f;   // +Y вверх
    
    quat_rotate_vector(&forward_x, &forward_y, &forward_z, &camera.orientation);
    quat_rotate_vector(&right_x,   &right_y,   &right_z,   &camera.orientation);
    quat_rotate_vector(&up_x,      &up_y,      &up_z,      &camera.orientation);
    
    // Build the view matrix (Row-major, Left-Handed)
// Camera axes are stored in columns (since row-major)
    MatrixIdentity(view);
    
    SET_MATRIX_VALUE(view, 0, 0, right_x);
    SET_MATRIX_VALUE(view, 1, 0, right_y);
    SET_MATRIX_VALUE(view, 2, 0, right_z);
    
    SET_MATRIX_VALUE(view, 0, 1, up_x);
    SET_MATRIX_VALUE(view, 1, 1, up_y);
    SET_MATRIX_VALUE(view, 2, 1, up_z);
    
    // For the left-handed system forward = +forward
    SET_MATRIX_VALUE(view, 0, 2, forward_x);
    SET_MATRIX_VALUE(view, 1, 2, forward_y);
    SET_MATRIX_VALUE(view, 2, 2, forward_z);
    
    // -dot(eye, axis)
    SET_MATRIX_VALUE(view, 3, 0, -(right_x   * camera.pos_x + right_y   * camera.pos_y + right_z   * camera.pos_z));
    SET_MATRIX_VALUE(view, 3, 1, -(up_x      * camera.pos_x + up_y      * camera.pos_y + up_z      * camera.pos_z));
    SET_MATRIX_VALUE(view, 3, 2, -(forward_x * camera.pos_x + forward_y * camera.pos_y + forward_z * camera.pos_z));
    SET_MATRIX_VALUE(view, 3, 3, 1.0f);

    // We will calculate the projection matrix as well
    MatrixIdentity(proj);
    MatrixProjection(proj,
        camera.fov,          // FOV 60 degrees
        camera.aspect,       // ratio 4:3
        camera.near_clip,    // nearest plane
        camera.far_clip);    // far plane

    //ExtractFrustumPlanes(&frustum, proj);
    SetupViewSpaceFrustum(&frustum, camera.fov, camera.aspect, camera.near_clip, camera.far_clip);
}

// Rotation of the camera around the coordinate system origin in degrees
void RotateCameraAroundWorld(float rotate_delta_pitch, float rotate_delta_yaw, float rotate_delta_roll) {
    camera.pitch += rotate_delta_pitch;
    camera.yaw += rotate_delta_yaw;
    camera.roll += rotate_delta_roll;
}

// Rotation of the camera around itself in quaternions
void RotateCameraAroundLocal(float pitch_delta, float yaw_delta, float roll_delta) {
    float pitch_rad, yaw_rad, roll_rad;
    Quaternion delta, new_orient;

    // Convert deltas to radians
    pitch_rad = pitch_delta * DEGTORAD;
    yaw_rad = yaw_delta * DEGTORAD;
    roll_rad = roll_delta * DEGTORAD;
    
    // Create a quaternion change: order YXZ (yaw, pitch, roll)
    quat_from_euler(&delta, pitch_rad, yaw_rad, roll_rad);
    
    // Apply to the current orientation
    quat_multiply(&new_orient, &camera.orientation, &delta);
    camera.orientation = new_orient;
    quat_normalize(&camera.orientation);
}
void RotateCameraAroundPos(
    float pos_x, float pos_y, float pos_z,
    float distance, float angle) {
    
    float dx, dy, dz;
    float horiz;
    float a = angle * DEGTORAD;

    // Place the camera in orbit around the target
    // angle = 0  -> camera at +Z from the target
    camera.pos_x = pos_x + distance * fast_sin(a);
    camera.pos_y = pos_y;
    camera.pos_z = pos_z + distance * fast_cos(a);

    // Direction from the camera to the target
    dx = pos_x - camera.pos_x;
    dy = pos_y - camera.pos_y;
    dz = pos_z - camera.pos_z;
    // dx = camera.pos_x - pos_x;
    // dy = camera.pos_y - pos_y;
    // dz = camera.pos_z - pos_z;

    // Basic angles "look at the target".
    // yaw   — around Y, pitch — around X (left-handed system, +Z forward).
    horiz = (float)sqrt((double)dx*(double)dx + (double)dz*(double)dz);
    
    camera.yaw   = (float)atan2((double)dx, (double)dz) * RADTODEG;
    camera.pitch = -(float)atan2((double)dy, (double)horiz) * RADTODEG;
    
    // Rebuilding the quaternion — otherwise MoveCameraLocal
    // and UpdateMatricesViewProj will use the old orientation.
    quat_from_euler(&camera.orientation,
        camera.pitch * DEGTORAD,
        camera.yaw   * DEGTORAD,
        camera.roll  * DEGTORAD);
}

void MoveCameraWorld(float move_delta_x, float move_delta_y, float move_delta_z) {
    camera.pos_x += move_delta_x;
    camera.pos_y += move_delta_y;
    camera.pos_z += move_delta_z;
}

// Movement of the camera taking into account its rotation in quaternions
void MoveCameraLocal(float forwardDelta, float rightDelta, float upDelta) {
    // Getting current directions from the quaternion
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

void SetupViewSpaceFrustum(FrustumPlanes* planes, float fov_deg, float aspect, float near_clip, float far_clip) {
    float fov_rad = fov_deg * DEGTORAD;
    float tan_half = tan(fov_rad * 0.5f);
    float tan_h_half = tan_half * aspect;

    // Внутри — там, где eval_plane_value >= 0.
    // В view-space: z >= near_clip (near)
    //               z <= far_clip  (far)
    //               |x| <= z * tan_h_half   (left/right)
    //               |y| <= z * tan_half     (bottom/top)

    // near: z - near_clip >= 0
    planes->near_plane[0] = 0;
    planes->near_plane[1] = 0;
    planes->near_plane[2] = 1;
    planes->near_plane[3] = -near_clip;

    // far: far_clip - z >= 0
    planes->far_plane[0] = 0;
    planes->far_plane[1] = 0;
    planes->far_plane[2] = -1;
    planes->far_plane[3] = far_clip;

    // left: x + z * tan_h_half >= 0
    planes->left_plane[0] = 1;
    planes->left_plane[1] = 0;
    planes->left_plane[2] = tan_h_half;
    planes->left_plane[3] = 0;

    // right: -x + z * tan_h_half >= 0
    planes->right_plane[0] = -1;
    planes->right_plane[1] = 0;
    planes->right_plane[2] = tan_h_half;
    planes->right_plane[3] = 0;

    // bottom: y + z * tan_half >= 0
    planes->bottom_plane[0] = 0;
    planes->bottom_plane[1] = 1;
    planes->bottom_plane[2] = tan_half;
    planes->bottom_plane[3] = 0;

    // top: -y + z * tan_half >= 0
    planes->top_plane[0] = 0;
    planes->top_plane[1] = -1;
    planes->top_plane[2] = tan_half;
    planes->top_plane[3] = 0;
}
