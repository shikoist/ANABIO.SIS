#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "glide.h"
#include "MESH.H"
#include "CAMERA.H"
#include "TEXTURE.H"
#include "TEXT.H"
#include "KEYBOARD.H"
#include "SOUND.H"
#include "MAIN.H"

#include "GAME_HUB.H"

// Prepared textures count
// With numbers in the TEXTURES folder
// At values =192 =256 =512 it gives Stack Overflow
#define MAX_TEXTURES       32
#define MAX_TMU 3

#define TEXTURE_DATA_SIZE  131072

Texture tableTexture;
Texture cubeTexture;
Texture floorTexture;
Texture wallTexture;
Texture ceilTexture;

int i, j, a, b;

Mesh* cubeMesh;
Mesh* tableMesh;
Mesh* floorMesh;
Mesh* wallMesh;
Mesh* ceilMesh;

Sound synth;

int game_hub_start() {

   cubeTexture.tmu = 0;
   tableTexture.tmu = 0;
   floorTexture.tmu = 0;
   wallTexture.tmu = 0;
   ceilTexture.tmu = 0;

   if (load_wav("MUSIC/SYNTH.WAV", &synth) != 0) return -1;

   //mixer_play_sound_ex(-1, &synth, 1, 0);

   if (LoadTexture("MODELS\\CUBEAXIS.3DF", &cubeTexture) != 0) {
      printf("Error loading %s\n", cubeTexture.filename);
      return -1;
   }
   if (LoadTexture("MODELS\\ANB_TABL.3DF", &tableTexture) != 0) {
      printf("Error loading %s\n", tableTexture.filename);
      return -1;
   }
   if (LoadTexture("MODELS\\ANB_FLR.3DF", &floorTexture) != 0) {
      printf("Error loading %s\n", floorTexture.filename);
      return -1;
   }
   if (LoadTexture("MODELS\\ANB_WALL.3DF", &wallTexture) != 0) {
      printf("Error loading %s\n", wallTexture.filename);
      return -1;
   }
   if (LoadTexture("MODELS\\ANB_CEIL.3DF", &ceilTexture) != 0) {
      printf("Error loading %s\n", ceilTexture.filename);
      return -1;
   }

   // Try to load model
   cubeMesh = LoadMeshFromOBJ("MODELS\\CUBEAXIS.OBJ");
   if (!cubeMesh) {
      printf("Failed to load mesh %s\n", "CUBEAXIS.OBJ");
      return -1;
   }


   tableMesh = LoadMeshFromOBJ("MODELS\\ANB_TABL.OBJ");
   //tableMesh = LoadMeshFromOBJ("MODELS\\CUBE.OBJ");
   if (!tableMesh) {
      printf("Failed to load mesh %s\n", "ANB_TABL.OBJ");
      return -1;
   }

   floorMesh = LoadMeshFromOBJ("MODELS\\ANB_FLR.OBJ");
   if (!floorMesh) {
      printf("Failed to load mesh %s\n", "ANB_FLR.OBJ");
      return -1;
   }

   wallMesh = LoadMeshFromOBJ("MODELS\\ANB_WALL.OBJ");
   if (!wallMesh) {
      printf("Failed to load mesh %s\n", "ANB_WALL.OBJ");
      return -1;
   }

   ceilMesh = LoadMeshFromOBJ("MODELS\\ANB_CEIL.OBJ");
   if (!ceilMesh) {
      printf("Failed to load mesh %s\n", "ANB_CEIL.OBJ");
      return -1;
   }

   SetupCamera(
      0.0f, 3.0f, -4.0f, // position
      20.0f, 0.0f, 0.0f,  // rotation
      90.0f, 4.0f/3.0f, // FOV, aspect
      2.0f, 100.0f); // near_clip, far_clip

   return 1;
}

int game_hub_update() {
   if (key_states[KEY_ESCAPE]) return -2; // ESC
   
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
      
      key_states_prev[KEY_1] = 1;
   }
   if (key_states[KEY_1] == 0) {
      key_states_prev[KEY_1] = 0;
   }

   // Test sound 2 (door opened)
   if (key_states[KEY_2] == 1 && key_states_prev[KEY_2] == 0) {
      
      key_states_prev[KEY_2] = 1;
   }
   if (key_states[KEY_2] == 0) {
      key_states_prev[KEY_2] = 0;
   }

   // Test sound 3 (explosion occurred)
   if (key_states[KEY_3] == 1 && key_states_prev[KEY_3] == 0) {
      
      key_states_prev[KEY_3] = 1;
   }
   if (key_states[KEY_3] == 0) {
      key_states_prev[KEY_3] = 0;
   }

   RotateCameraAroundPos(
      0, 2.5f, 0,
      6.0f, local_time * 20);
   

   // Update view and proj matrix
   // with new camera movement and rotation data
   UpdateMatricesViewProj();

   //DrawMeshWithClip(cubeMesh, &cubeTexture, 2, 1, 0, 0, 0, 0, 1, 1, 1);
   DrawMeshWithClip(tableMesh, &tableTexture, 0, 0, 0, 0, 0, 0, 1, 1, 1);
   DrawMeshWithClip(floorMesh, &floorTexture, 0, 0, 0, 0, 0, 0, 1, 1, 1);
   DrawMeshWithClip(wallMesh, &wallTexture, 0, 0, 0, 0, 0, 0, 1, 1, 1);
   DrawMeshWithClip(ceilMesh, &ceilTexture, 0, 0, 0, 0, 0, 0, 1, 1, 1);
   
   return 1;
}

int game_hub_clear() {
   int i;

   UnloadMesh(cubeMesh);
   UnloadMesh(tableMesh);
   UnloadMesh(floorMesh);
   UnloadMesh(ceilMesh);
   UnloadMesh(wallMesh);

   for (i = 0; i < MAX_STREAMS; i++) {
      mixer_stop(i);
      mixer_free_channel(i);
   }

   // More safer
   if (synth.data)    { free(synth.data);    synth.data = NULL; }
   
   return 1;
}
