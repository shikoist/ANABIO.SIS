/************************************************************************
*  AHABIO.SIS by shikoist                                               *
* This is a game engine for 3dfx Voodoo 1 and Sound Blaster Pro 2 under DOS.*
* Compiled under 3dfx Glide SDK 2.43 and Open Watcom 2.0 beta.        *
************************************************************************/

// MAIN.C - start
#define MAX_TMU            3

#include <stdarg.h>
#include <i86.h>   // for MK_FP
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <dos.h>
#include <malloc.h>

#include "MAIN.H"

#include "mesh.h"
#include "keyboard.h"
#include "MOUSE.H"
#include "camera.h"
#include "timer.h"
#include "file.h"
#include "text.h"
#include "texture.h"
#include "primitiv.h" // Render primitive sets
#include "matrix.h"   // Matrix functions
#include "fastmath.h"
#include "glideutl.h"
#include "glide.h"
#include "SOUND.H"
#include "GAME_HUB.H"
#include "GAME_00.H"
#include "GAME_01.H"

unsigned int build = 0;

unsigned int current_screen = 0;

GrHwConfiguration hwconfig;
FxI32 num_tmu = 0; // Number of TMU

int frame = 0;

int exit_now = 0;

float frequency;
float f1;
float f2;
float delta_time;
unsigned long curr_ticks;
unsigned long prev_ticks;
unsigned long delta_ticks;
float local_time = 0;
float secondsConsole = 0;

int count = 0;

GrColor_t backColor;

//int i, j, k, a, b, c;
int i;

Font fpsFont;
char fpsString[80];

// small self-test unit for math, matrix, transformation functions
void selftest_and_exit(void) {
    Quaternion q;
    float vx, vy, vz;
    float m[16];
    float a[3], b[3], c[3];
    int i;

    printf("=== SELFTEST START ===\n\n");

    /* --- 1. Identity quaternion --- */
    quat_identity(&q);
    printf("[QUAT] identity = (%.4f %.4f %.4f %.4f)\n", q.x, q.y, q.z, q.w);

    /* --- 2. quat_rotate_vector on identity: should not change vector --- */
    vx = 1.0f; vy = 0.0f; vz = 0.0f;
    quat_rotate_vector(&vx, &vy, &vz, &q);
    printf("[QUAT] identity rotates (1,0,0) -> (%.4f %.4f %.4f)  [expect (1,0,0)]\n",
           vx, vy, vz);

    vx = 0.0f; vy = 1.0f; vz = 0.0f;
    quat_rotate_vector(&vx, &vy, &vz, &q);
    printf("[QUAT] identity rotates (0,1,0) -> (%.4f %.4f %.4f)  [expect (0,1,0)]\n",
           vx, vy, vz);

    vx = 0.0f; vy = 0.0f; vz = 1.0f;
    quat_rotate_vector(&vx, &vy, &vz, &q);
    printf("[QUAT] identity rotates (0,0,1) -> (%.4f %.4f %.4f)  [expect (0,0,1)]\n",
           vx, vy, vz);

    /* --- 3. Yaw 90 deg around Y: (1,0,0) should go to (0,0,-1) or (0,0,1) --- */
    quat_from_euler(&q, 0.0f, FASTMATH_HALF_PI, 0.0f);
    printf("\n[QUAT] yaw 90 deg: q = (%.4f %.4f %.4f %.4f)\n", q.x, q.y, q.z, q.w);

    vx = 1.0f; vy = 0.0f; vz = 0.0f;
    quat_rotate_vector(&vx, &vy, &vz, &q);
    printf("[QUAT] rotate (1,0,0) by yaw90 -> (%.4f %.4f %.4f)\n", vx, vy, vz);

    vx = 0.0f; vy = 0.0f; vz = 1.0f;
    quat_rotate_vector(&vx, &vy, &vz, &q);
    printf("[QUAT] rotate (0,0,1) by yaw90 -> (%.4f %.4f %.4f)\n", vx, vy, vz);

    /* --- 4. Check that axes remain orthonormal and right-handed --- */
    /* Use nonzero yaw+pitch to avoid degenerate case */
    quat_from_euler(&q, 30.0f * DEGTORAD, 40.0f * DEGTORAD, 0.0f);

    a[0] = 1.0f; a[1] = 0.0f; a[2] = 0.0f;   /* right */
    b[0] = 0.0f; b[1] = 1.0f; b[2] = 0.0f;   /* up */
    c[0] = 0.0f; c[1] = 0.0f; c[2] = 1.0f;   /* forward */

    quat_rotate_vector(&a[0], &a[1], &a[2], &q);
    quat_rotate_vector(&b[0], &b[1], &b[2], &q);
    quat_rotate_vector(&c[0], &c[1], &c[2], &q);

    printf("\n[QUAT] after pitch=30 yaw=40:\n");
    printf("  right   = (%.4f %.4f %.4f)\n", a[0], a[1], a[2]);
    printf("  up      = (%.4f %.4f %.4f)\n", b[0], b[1], b[2]);
    printf("  forward = (%.4f %.4f %.4f)\n", c[0], c[1], c[2]);

    /* Length check */
    printf("  |right|   = %.4f  [expect 1.0]\n",
           sqrt(a[0]*a[0] + a[1]*a[1] + a[2]*a[2]));
    printf("  |up|      = %.4f  [expect 1.0]\n",
           sqrt(b[0]*b[0] + b[1]*b[1] + b[2]*b[2]));
    printf("  |forward| = %.4f  [expect 1.0]\n",
           sqrt(c[0]*c[0] + c[1]*c[1] + c[2]*c[2]));

    /* right x up should equal forward (right-handed) or -forward (left-handed).
       In your engine you assume right x up = forward, which means RIGHT-handed. */
    {
        float cx = a[1]*b[2] - a[2]*b[1];
        float cy = a[2]*b[0] - a[0]*b[2];
        float cz = a[0]*b[1] - a[1]*b[0];
        printf("  right x up = (%.4f %.4f %.4f)\n", cx, cy, cz);
        printf("  forward    = (%.4f %.4f %.4f)\n", c[0], c[1], c[2]);
        printf("  -> if they match: system is RIGHT-handed\n");
    }

    /* --- 5. View matrix for identity camera at origin --- */
    SetupCamera(0, 0, 0, 0, 0, 0, 90, 4.0f/3.0f, 2.0f, 100.0f);
    UpdateMatricesViewProj();

    printf("\n[VIEW] camera at origin, no rotation\n");
    for (i = 0; i < 4; i++) {
        printf("  [%8.4f %8.4f %8.4f %8.4f]\n",
               view[i*4+0], view[i*4+1], view[i*4+2], view[i*4+3]);
    }
    printf("  [expect identity]\n");

    /* --- 6. View matrix for camera at (0, 0, -5) looking at +Z --- */
    SetupCamera(0, 0, -5, 0, 0, 0, 90, 4.0f/3.0f, 2.0f, 100.0f);
    UpdateMatricesViewProj();

    printf("\n[VIEW] camera at (0, 0, -5), no rotation\n");
    for (i = 0; i < 4; i++) {
        printf("  [%8.4f %8.4f %8.4f %8.4f]\n",
               view[i*4+0], view[i*4+1], view[i*4+2], view[i*4+3]);
    }
    printf("  [expect identity with last row 0 0 +5 1]\n");

    /* --- 7. Projection matrix --- */
    MatrixIdentity(m);
    MatrixProjection(m, 90.0f, 4.0f/3.0f, 2.0f, 100.0f);

    printf("\n[PROJ] fov=90, aspect=4/3, near=2, far=100\n");
    for (i = 0; i < 4; i++) {
        printf("  [%8.4f %8.4f %8.4f %8.4f]\n",
               m[i*4+0], m[i*4+1], m[i*4+2], m[i*4+3]);
    }

    /* --- 8. ApplyMatrix on a known point --- */
    {
        GrVertex v;
        memset(&v, 0, sizeof(v));
        v.x = 0.0f; v.y = 0.0f; v.z = 5.0f;   /* 5 units in front of camera */

        /* Use simple projection with camera at origin, looking +Z */
        SetupCamera(0, 0, 0, 0, 0, 0, 90, 4.0f/3.0f, 2.0f, 100.0f);
        UpdateMatricesViewProj();

        /* First apply view */
        ApplyMatrix(&v, view);
        printf("\n[APPLY] point (0,0,5) after view  -> (%.4f %.4f %.4f, oow=%.4f)\n",
               v.x, v.y, v.z, v.oow);
        printf("  [expect (0, 0, 5) with oow=1]\n");

        /* Then apply projection */
        ApplyMatrix(&v, proj);
        printf("[APPLY] same point after proj -> (%.4f %.4f %.4f, oow=%.4f)\n",
               v.x, v.y, v.z, v.oow);
        printf("  [expect x=0, y=0, z between 0 and 1, oow=1/5=0.2]\n");

        /* --- 9. Point to the right (+X) --- */
        v.x = 1.0f; v.y = 0.0f; v.z = 5.0f; v.oow = 1.0f;
        SetupCamera(0, 0, 0, 0, 0, 0, 90, 4.0f/3.0f, 2.0f, 100.0f);
        UpdateMatricesViewProj();
        ApplyMatrix(&v, view);
        ApplyMatrix(&v, proj);
        printf("[APPLY] point (1,0,5) -> (%.4f %.4f %.4f, oow=%.4f)\n",
               v.x, v.y, v.z, v.oow);
        printf("  [expect x > 0, y = 0, z between 0 and 1, oow=0.2]\n");
        printf("  -> if x < 0: engine mirrors X!\n");

        /* --- 10. Point above (+Y) --- */
        v.x = 0.0f; v.y = 1.0f; v.z = 5.0f; v.oow = 1.0f;
        SetupCamera(0, 0, 0, 0, 0, 0, 90, 4.0f/3.0f, 2.0f, 100.0f);
        UpdateMatricesViewProj();
        ApplyMatrix(&v, view);
        ApplyMatrix(&v, proj);
        printf("[APPLY] point (0,1,5) -> (%.4f %.4f %.4f, oow=%.4f)\n",
               v.x, v.y, v.z, v.oow);
        printf("  [expect x = 0, y > 0, z between 0 and 1, oow=0.2]\n");
        printf("  -> if y < 0: engine mirrors Y!\n");
    }

    printf("\n=== SELFTEST END ===\n");
    exit(0);
}

int safe_shutdown() {

   //free(tempVtxBuffer);
   grSstWinClose();
   grGlideShutdown();
   timer_shutdown();
   keyboard_shutdown();
   mouse_shutdown();
   sb_stop_playback();
   //sb_cleanup();
   cleanup_buffers();
   cleanup_irq_handler();
   return 0;
}

// Returns free memory in bytes
unsigned long GetFreeMemory(void)
{
   return (unsigned long)_memavl();
}

int read_version() {
   FILE *fp;
   char* build = "build.txt";
   int value = 0;

   /* Trying to open file for reading */
   fp = fopen(build, "r");
   if (fp != NULL) {
      /* If file exists - read number */
      if (fscanf(fp, "%d", &value) != 1) {
         /* If reading failed (empty file or not a number) - start from 0 */
         value = 0;
      }
      fclose(fp);
   } else {
      /* File does not exist - start from 0 */
      value = 0;
   }

   return value;
}

int main()
{
   // Here we will leave only the most necessary
   build = read_version();
   printf("Build number: %d\n", build);

   srand(dos_time);
   

   init_fast_math(); puts("init_fast_math()");
   generate_base_models();

   //selftest_and_exit();

   f1 = (float)BASE_FREQUENCY;
   f2 = (float)CUSTOM_DIVIDER;
   frequency = f1 / f2;
   printf("New Timer frequency: %.2f Hz\n", frequency);
   
   keyboard_init(); puts("keyboard_init()");
   mouse_init(); puts("mouse_init()");
   timer_init(); puts("timer_init()");

   prev_ticks = dos_time;

   // iNIT GlIDE
   printf("[INIT] We want THE 3D GRAPHIX from 3dfx Voodoo!\n");
   grGlideInit(); puts("grGlideInit() OK");
   if (!grSstQueryHardware(&hwconfig)) {
      // Error handling: Voodoo not found
      puts("Chipset 3dfx Voodoo not found");
      grGlideShutdown();
      return -1;
   }
   grSstSelect(0); puts("grSstSelect(0) OK");
   grSstWinOpen(
      0,
      GR_RESOLUTION_640x480,
      GR_REFRESH_60Hz,
      GR_COLORFORMAT_ABGR,
      //GR_ORIGIN_LOWER_LEFT,
      GR_ORIGIN_UPPER_LEFT,
      2, // Two standard buffers, front and back
          // Front buffer is displayed, back buffer is drawn
          // then swap. This results in smooth animation.
      1 // Additional buffers, e.g. for Z-buffer
   ); puts("grSstWinOpen OK");

   // Do not draw pixels outside the screen unnecessarily
   grClipWindow(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

   // I spent a lot of time trying to figure out,
   // why triangles were not rendering,
   // until I added a delay here
   //sleep(1);
   delay(1);

   // Determine the amount of texture memory and the number of TMU
   num_tmu = hwconfig.SSTs[0].sstBoard.VoodooConfig.nTexelfx;;
   if (num_tmu < 0 || num_tmu > 3) {
      printf("Found incorrect number of TMUs: %d\n", (int)num_tmu);
      safe_shutdown();
      return 0;
   }
   else {
      printf("Found TMUs: %d\n", (int)num_tmu);
   }

   grDepthBufferMode( GR_DEPTHBUFFER_WBUFFER );
   //grDepthBufferMode( GR_DEPTHBUFFER_ZBUFFER );
   grDepthBufferFunction( GR_CMP_LEQUAL );
   grDepthMask(FXTRUE);

   // grCullMode(GR_CULL_DISABLE); // All triangles are visible
   // grCullMode(GR_CULL_POSITIVE); // Visible triangles clockwise
   grCullMode(GR_CULL_NEGATIVE); // Visible triangles counterclockwise - standard for Blender

   delay(1);

   // INIT SOUND
   printf("[INIT] We want THE MUSIC from Sound Blaster!\n");
   setup_sound_blaster();
   setup_irq_handler();

   for (i = 0; i < NUM_BUFFERS; i++) {
      dma_buffers[i] = alloc_low_dos_memory(BUFFER_SIZE, 
                                             &dma_selectors[i], 
                                             &dma_segments[i]);
      if (dma_buffers[i] == NULL) {
         printf("[MEM] Memory allocation error!\n");
         return -1;
      }
   }

   mixer_init();

   first_run = 1;   // To initialize DSP
   // dsp_write(DSP_CMD_SPEAKER_ON);  // Turn on speakers!

   
   // getch();   

   // Here we load the font
   //if (LoadFont("TEXTURES\\font_sqr.3df", &fpsFont) != 0) {
   if (LoadFont("TEXTURES\\font.3df", &fpsFont) != 0) {
      printf("Failed to load font!\n");
      return -1;
   }

   game_hub_start();
   //game_01_start();

   // Main rendering loop
   while (1) {
      mixer_process();

      mouse_update();

      triangles_drawn = 0;

      backColor = 0x00005500; // ABGR - Dark Blue Screen
      grBufferClear(backColor, 0, GR_WDEPTHVALUE_FARTHEST);

      frame++;

      curr_ticks = dos_time;
      delta_ticks = curr_ticks - prev_ticks;   // tick difference
      delta_time = (float)delta_ticks / frequency;   // seconds
      local_time += delta_time;
      secondsConsole += delta_time;

      // To prevent frequent console output
      if (secondsConsole >= 1.0f) {
         //printf("Frame: %d Delta ticks: %d Delta time: %.6f FPS: %.1f\n", frame, delta_ticks, delta_time, 1.0f/delta_time);
         //fflush(stdout);
         secondsConsole = 0;

         // To prevent FPS text from changing too frequently
         sprintf(fpsString, "FPS: %.0f  (%.4f)\n", 1.0f / delta_time, delta_time);
      }

      prev_ticks = curr_ticks;

      switch (current_screen) {
         case 0: {
            if (game_hub_update() == -2) {
               exit_now = 1;
            };
            break;
         }
         case 1: {
            game_01_update();
            break;
         }
      }

      // Text is drawn last
      // Draw FPS (white color = 0xFFFFFFFF)
      DrawTextF(&fpsFont, 8.0f, 8, COLOR_WHITE, "ANABIO.SIS BY SHIKOIST build %d", build);
      DrawText(&fpsFont, 8.0f, 8 + 16, COLOR_MAGENTA, fpsString);
      DrawTextF(&fpsFont, 320.0f, 8 + 16, COLOR_MAGENTA, "TIME: %.2f", local_time);
      DrawTextF(&fpsFont, 8.0f, 8 + 16*2, COLOR_MAGENTA, 
         "Cam Pos: %.1f %.1f %.1f", camera.pos_x, camera.pos_y,camera.pos_z);
         DrawTextF(&fpsFont, 8.0f, 8 + 16*3, COLOR_MAGENTA, 
         "Cam Rot: %.1f %.1f %.1f", camera.pitch, camera.yaw,camera.roll);
      DrawTextF(&fpsFont, 8.0f, 8 + 16*4, COLOR_MAGENTA, 
         "Triangles: %d (%d/sec)", triangles_drawn, triangles_drawn * (int)(1.0f / delta_time));
      DrawTextF(&fpsFont, 8.0f, 8 + 16*5, COLOR_MAGENTA, 
         "irqs: %d | status: %d | read: %d", counter, irq_debug_last_status, irq_debug_read_data);
      DrawTextF(&fpsFont, 8.0f, 8 + 16*6, COLOR_YELLOW, 
         "Mouse: %d | x: %d | y: %d", mouse.visible, mouse.x, mouse.y);

      // Mouse?!
      // Draw a simple cross at the mouse position (to see that the coordinates are live)
      if (mouse.available) DrawTextF(&fpsFont, (float)mouse.x - 4, (float)mouse.y - 8, COLOR_WHITE, "+"); 

      //grSstIdle();
      grBufferSwap(0);
      
      //delay(2);

      // At the end of each frame (for debugging)
      // if (frame % 30 == 0) {
      //    printf("Frame %d, free memory: %lu bytes\n", frame, GetFreeMemory());
      // }

      // Test brick for one pass
      //break;

      // Sound processing
      //mixer_process();
      

      if (exit_now == 1) break;
   }
   
   game_00_clear();

   safe_shutdown(); puts("safe_shutdown()");
   
   return 0;
}
