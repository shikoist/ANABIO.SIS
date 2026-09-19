#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "MESH.H"
#include "CAMERA.H"
#include "TEXTURE.H"
#include "TEXT.H"
#include "KEYBOARD.H"
#include "SOUND.H"
#include "MAIN.H"
#include "GAME_00.H"

// Prepared textures count
// With numbers in the TEXTURES folder
// At values =192 =256 =512 it gives Stack Overflow
#define MAX_TEXTURES       32
#define MAX_TMU 3

#define TEXTURE_DATA_SIZE  131072

TextureSlot someTextureSlot;
char someTextureName[] = "TEXTURES\\TEST.3DF";
//char ebakotModelName[] = "MODELS\\SPHERE.OBJ";
//char someModelName[] = "MODELS\\SKYBOX.OBJ";
//char someModelName[] = "MODELS\\PLANE.OBJ";
char someModelName[] = "MODELS\\CUBE.OBJ";

float angleX, angleY, angleZ;

float moveZ = 20;
float moveZ_plus = 0.1f;

int current_texture = 0;

int i, j, a, b;




unsigned long frame_ticks_before;
unsigned long frame_ticks_after;

FxU32 max_tmu_memory[MAX_TMU];

int max_textures[MAX_TMU]; // Maximum number of textures allowed for TMU

int key;


float stop = 2000.0f;

float camera_rotation_speed = 45.0f;
float camera_speed = 1;
// float default_camera_yaw   = 180.0f;  // I still don't understand why the camera only looks at +Z
// float default_camera_pitch = -4.0f;   // at these values
// float default_camera_roll = 0.0f;   // at these values
float default_camera_yaw   = 0.0f;  // I still don't understand why the camera only looks at +Z
float default_camera_pitch = 0.0f;   // at these values
float default_camera_roll = 0.0f;   // at these values

Mesh* someMesh;

int field_size = 11;
int field_size_half = 5;

// unsigned char *synth;
// unsigned char *upbeat;
// unsigned char *gunshot;
// unsigned char *door;
// unsigned char *explosion;

int player_shoots = 0;
int door_opened = 0;
int explosion_happens = 0;

float player_shoots_time = 0;
float player_shoots_time_rate = 0.5f;

float door_opened_time = 0;
float door_opened_time_rate = 0.7f;

float explosion_happens_time = 0;
float explosion_happens_time_rate = 1.0f;

// unsigned int synth_size, synth_rate, synth_channels;
// unsigned int upbeat_size, upbeat_rate, upbeat_channels;
// unsigned int gunshot_size, gunshot_rate, gunshot_channels;
// unsigned int door_size, door_rate, door_channels;
// unsigned int explosion_size, explosion_rate, explosion_channels;

Sound synth, upbeat, gunshot, door, explosion;

int game_00_start() {
   if (load_wav("MUSIC/SYNTH.WAV", &synth) != 0) return -1;
   if (load_wav("MUSIC/UPBEAT.WAV", &upbeat) != 0) return -1;
   if (load_wav("SOUNDS/GUNSHOT.WAV", &gunshot) != 0) return -1;
   if (load_wav("SOUNDS/DOOR.WAV", &door) != 0) return -1;
   if (load_wav("SOUNDS/EXPLODE.WAV", &explosion) != 0) return -1;
   
   //test_sound_generator();
   //test_stereo();
   mixer_play_sound_ex(-1, &synth, 1, 0);

   if (LoadTexture(someTextureName, &someTextureSlot, 0) != 0) {
      printf("Error loading %s\n", someTextureName);
      return -1;
   }

   // Try to load ship model
   someMesh = LoadMeshFromOBJ(someModelName);
   if (!someMesh) {
      printf("Failed to load mesh %s\n", someModelName);
      return -1;
   }

   SetupCamera(
      0, 30, -7, // position
      75, 0, 0,  // rotation
      90, 4.0f/3.0f, // FOV, aspect
      3.0f, 100.0f); // near_clip, far_clip

   return 1;
}

int game_00_update() {
   if (key_states[KEY_ESCAPE]) return -2; // ESC
   
   if (key_states[KEY_DELETE])    RotateCameraAroundLocal(0, -camera_rotation_speed * delta_time, 0);
   if (key_states[KEY_PAGEDOWN])  RotateCameraAroundLocal(0,  camera_rotation_speed * delta_time, 0);
   if (key_states[KEY_HOME])      RotateCameraAroundLocal( camera_rotation_speed * delta_time, 0, 0);
   if (key_states[KEY_END])       RotateCameraAroundLocal(-camera_rotation_speed * delta_time, 0, 0);
   if (key_states[KEY_INSERT])    RotateCameraAroundLocal(0, 0, -camera_rotation_speed * delta_time);
   if (key_states[KEY_PAGEUP])    RotateCameraAroundLocal(0, 0,  camera_rotation_speed * delta_time);
   
   if (key_states[KEY_Q]) 
      // TranslateCamera(0,  camera_speed * delta_time, 0);
      MoveCameraLocal(0, 0, -camera_speed * delta_time);
   if (key_states[KEY_E])
      // TranslateCamera(0, -camera_speed * delta_time, 0);
      MoveCameraLocal(0, 0, camera_speed * delta_time);
   if (key_states[KEY_D])
      // TranslateCamera( camera_speed * delta_time, 0, 0);
      MoveCameraLocal( 0, camera_speed * delta_time, 0);
   if (key_states[KEY_A])
      // TranslateCamera(-camera_speed * delta_time, 0, 0);
      MoveCameraLocal( 0, -camera_speed * delta_time, 0);
   if (key_states[KEY_W])
      // TranslateCamera(0, 0,  camera_speed * delta_time);
      MoveCameraLocal(camera_speed * delta_time, 0, 0);
   if (key_states[KEY_S])
      // TranslateCamera(0, 0, -camera_speed * delta_time);
      MoveCameraLocal(-camera_speed * delta_time, 0, 0);

   // To be pressed only once for F6
   if (key_states[KEY_F6] == 1 && key_states_prev[KEY_F6] == 0) {
      if (camera.wireframe_mode == 0) {
         camera.wireframe_mode = 1;
      }
      else {
         camera.wireframe_mode = 0;
      }
      key_states_prev[KEY_F6] = 1;
   }
   if (key_states[KEY_F6] == 0) {
      key_states_prev[KEY_F6] = 0;
   }

   // Load game 01
   if (key_states[KEY_1] == 1 && key_states_prev[KEY_1] == 0) {
      player_shoots = 1;
      key_states_prev[KEY_1] = 1;
   }
   if (key_states[KEY_1] == 0) {
      key_states_prev[KEY_1] = 0;
   }

   // Test sound 2 (door opened)
   if (key_states[KEY_2] == 1 && key_states_prev[KEY_2] == 0) {
      if (door_opened == 0) {
         door_opened = 1;
      }
      else {
         door_opened = 0;
      }
      key_states_prev[KEY_2] = 1;
   }
   if (key_states[KEY_2] == 0) {
      key_states_prev[KEY_2] = 0;
   }

   // Test sound 3 (explosion occurred)
   if (key_states[KEY_3] == 1 && key_states_prev[KEY_3] == 0) {
      if (explosion_happens == 0) {
         explosion_happens = 1;
      }
      else {
         explosion_happens = 0;
      }
      key_states_prev[KEY_3] = 1;
   }
   if (key_states[KEY_3] == 0) {
      key_states_prev[KEY_3] = 0;
   }

   // Update view and proj matrix
   // with new camera movement and rotation data
   UpdateMatricesViewProj();

   //angleX += 45.0f * delta_time;
   angleY += 360.0f * delta_time;
   //angleZ += 60.0f * delta_time;

   // Normalize angles in range [0, 360)
   if (angleX >= 360.0f) angleX -= 360.0f;
   if (angleY >= 360.0f) angleY -= 360.0f;
   if (angleZ >= 360.0f) angleZ -= 360.0f;

   // DrawMesh(someMesh, &someTextureSlot,
   //    0, 0, 0,
   //    0, 0, 0,
   //    4.0f, 4.0f, 4.0f);

   for (a = 0; a < field_size; a++) {
      for (b = 0; b < field_size; b++) {
         DrawMesh(someMesh, &someTextureSlot,
            (a - field_size_half) * 4, 0.0f, (b - field_size_half) * 3,
            angleX, angleY, angleZ,
            1.0f, 1.0f, 1.0f);
      }  
   }

   // DrawMesh(someMesh, &someTextureSlot,
   //    1.0f, 0.0f, 1.0f,
   //    angleX, angleY, angleZ,
   //    1.0f, 1.0f, 1.0f);

   // DrawMesh(someMesh, &someTextureSlot,
   //    -1.0f, 0.0f, 1.0f,
   //    angleX, angleY, angleZ,
   //    1.0f, 1.0f, 1.0f);
   
   
   

   // Test sound 1 - gunshot (KEY 1)
   if (player_shoots) {
      int ch;
      int dir;
      dir = (int)(rand() % 255);
      //printf("dir = %d\n", dir);
      //mixer_play_sound(-1, gunshot, gunshot_size, gunshot_rate, 0, 0);
      ch = mixer_play_sound_ex(-1, &gunshot, 0, 0);
      if (ch >= 0) mixer_set_pan(ch, dir);   // 0 leftmost, 255 rightmost
      player_shoots = 0;
   }

   // Test sound 2 - door (KEY 2)
   if (door_opened) {
      mixer_play_sound_ex(-1, &door, 0, 1);
      door_opened = 0;
   }

   // Test sound 3 - explosion (KEY 3)
   if (explosion_happens) {
      mixer_play_sound_ex(-1, &explosion, 0, 0);
      explosion_happens = 0;
   }
   return 1;
}

int game_00_clear() {
   int i;

   UnloadMesh(someMesh);

   for (i = 0; i < MAX_STREAMS; i++) {
      mixer_stop(i);
      mixer_free_channel(i);
   }

   // free(gunshot.data);
   // free(door.data);
   // free(explosion.data);

   // More safer
   if (synth.data)    { free(synth.data);    synth.data = NULL; }
   if (upbeat.data)   { free(upbeat.data);   upbeat.data = NULL; }
   if (gunshot.data)  { free(gunshot.data);  gunshot.data = NULL; }
   if (door.data)     { free(door.data);     door.data = NULL; }
   if (explosion.data){ free(explosion.data);explosion.data = NULL; }

   return 1;
}
