// LIGHT.H - headers, global declarations

#ifndef LIGHT_H
#define LIGHT_H

#define MAX_LIGHTS 8

typedef struct {
    /* Position in world-space.
     * If range < 0, the light is treated as directional
     * (position is ignored, direction = -(nx, ny, nz)).
     */
    float x, y, z;

    /* Direction for directional lights (normalized).
     * Ignored for point lights (range >= 0).
     */
    float nx, ny, nz;

    /* Color components, each in [0, 1]. */
    float r, g, b;

    /* Overall intensity multiplier, >= 0. */
    float intensity;

    /* Effective range for point lights.
     * Negative value = directional light.
     */
    float range;

    /* 0 = off, 1 = on */
    int enabled;

    /* --- internal, updated by update_lights() --- */
    /* Position in view-space. */
    float vx, vy, vz;
    /* Direction in view-space (for directional lights). */
    float vnx, vny, vnz;
} Light;

/* Ambient light — added once per triangle regardless of sources. */
extern float ambient_r;
extern float ambient_g;
extern float ambient_b;

/* Global array of light sources. */
extern Light lights[MAX_LIGHTS];
extern int   num_lights;

/* Initialize all lights to disabled state. */
void init_lights(void);

/* Call once per frame, AFTER UpdateMatricesViewProj(),
 * to transform all light positions and directions into view-space.
 */
void update_lights(void);

/* Compute lighting for a single triangle.
 *
 * nx, ny, nz  - normalized triangle normal in view-space
 * px, py, pz  - one point on the triangle (any vertex), view-space
 *
 * out_r, out_g, out_b - resulting color multipliers, each clamped to [0, 1].
 */
void calc_triangle_lighting(float nx, float ny, float nz,
                            float px, float py, float pz,
                            float* out_r, float* out_g, float* out_b);

#endif
