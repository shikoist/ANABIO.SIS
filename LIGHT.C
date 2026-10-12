/* LIGHT.C - lightning subsystem
 *
 * Supports up to MAX_LIGHTS point/directional sources.
 * All lighting calculations happen on the CPU (Voodoo 1 has no
 * hardware T&L), in view-space, with flat shading (one normal
 * per triangle via cross product) or Gauraud shading.
 */

 #include <math.h>

#include "camera.h"
#include "light.h"
#include "matrix.h"   /* for view[] and ApplyMatrix-like transforms */

/* Ambient — a small amount so nothing is ever completely black. */
float ambient_r = 0.15f;
float ambient_g = 0.15f;
float ambient_b = 0.15f;

Light lights[MAX_LIGHTS];
int   num_lights = 0;

void init_lights(void)
{
    int i;

    for (i = 0; i < MAX_LIGHTS; i++) {
        lights[i].x         = 0.0f;
        lights[i].y         = 0.0f;
        lights[i].z         = 0.0f;
        lights[i].nx        = 0.0f;
        lights[i].ny        = 0.0f;
        lights[i].nz        = 1.0f;
        lights[i].r         = 1.0f;
        lights[i].g         = 1.0f;
        lights[i].b         = 1.0f;
        lights[i].intensity = 1.0f;
        lights[i].range     = -1.0f;   /* directional by default */
        lights[i].enabled   = 0;
    }

    num_lights = 0;
}

/* Transform a world-space point (x, y, z) into view-space.
 * Uses the global 'view' matrix. Writes into *ox, *oy, *oz.
 */
static void world_to_view(float x, float y, float z,
                          float* ox, float* oy, float* oz)
{
    /* Row-major view matrix:
     *   view[0..2]   - right axis
     *   view[4..6]   - up axis
     *   view[8..10]  - forward axis
     *   view[12..14] - translation
     *
     * Point transformation (w = 1):
     *   x' = view[0]*x + view[4]*y + view[8]*z  + view[12]
     *   y' = view[1]*x + view[5]*y + view[9]*z  + view[13]
     *   z' = view[2]*x + view[6]*y + view[10]*z + view[14]
     */
    *ox = view[0]*x + view[4]*y + view[8]*z  + view[12];
    *oy = view[1]*x + view[5]*y + view[9]*z  + view[13];
    *oz = view[2]*x + view[6]*y + view[10]*z + view[14];
}

/* Transform a world-space direction (no translation) into view-space. */
static void dir_world_to_view(float x, float y, float z,
                              float* ox, float* oy, float* oz)
{
    *ox = view[0]*x + view[4]*y + view[8]*z;
    *oy = view[1]*x + view[5]*y + view[9]*z;
    *oz = view[2]*x + view[6]*y + view[10]*z;
}

void update_lights(void)
{
    int i;

    for (i = 0; i < MAX_LIGHTS; i++) {
        Light* L = &lights[i];

        if (!L->enabled) continue;

        if (L->range < 0.0f) {
            /* Directional light — transform direction only. */
            dir_world_to_view(L->nx, L->ny, L->nz,
                              &L->vnx, &L->vny, &L->vnz);
        } else {
            /* Point light — transform position. */
            world_to_view(L->x, L->y, L->z,
                          &L->vx, &L->vy, &L->vz);
        }
    }
}

void calc_triangle_lighting(float nx, float ny, float nz,
                            float px, float py, float pz,
                            float* out_r, float* out_g, float* out_b)
{
    float cr = ambient_r;
    float cg = ambient_g;
    float cb = ambient_b;
    int   i;

    for (i = 0; i < MAX_LIGHTS; i++) {
        Light* L = &lights[i];

        float lx, ly, lz;      /* vector from point to light */
        float dist;
        float atten;
        float ndotl;

        if (!L->enabled) continue;

        if (L->range < 0.0f) {
            /* Directional: light comes FROM direction (vnx, vny, vnz). */
            lx = L->vnx;
            ly = L->vny;
            lz = L->vnz;
            atten = 1.0f;
        } else {
            /* Point: vector from surface point to light position. */
            lx = L->vx - px;
            ly = L->vy - py;
            lz = L->vz - pz;

            dist = (float)sqrt((double)(lx*lx + ly*ly + lz*lz));
            if (dist > L->range) continue;   /* out of range */

            /* Linear attenuation: 1 at d=0, 0 at d=range. */
            atten = 1.0f - dist / L->range;

            /* Normalize direction. */
            if (dist > 0.0001f) {
                float inv = 1.0f / dist;
                lx *= inv;
                ly *= inv;
                lz *= inv;
            }
        }

        /* Lambert: dot(N, L), clamped to [0, 1]. */
        ndotl = nx*lx + ny*ly + nz*lz;
        if (ndotl <= 0.0f) continue;   /* back-facing to the light */

        /* Total contribution factor. */
        {
            float f = ndotl * atten * L->intensity;
            cr += L->r * f;
            cg += L->g * f;
            cb += L->b * f;
        }
    }

    /* Clamp to [0, 1]. */
    if (cr > 1.0f) cr = 1.0f;
    if (cg > 1.0f) cg = 1.0f;
    if (cb > 1.0f) cb = 1.0f;

    *out_r = cr;
    *out_g = cg;
    *out_b = cb;
}
