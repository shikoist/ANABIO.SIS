/************************************************************************
*  AHABIO.SIS by shikoist                                               *
*  Это игровой движок для 3dfx Voodoo 1 и Sound Blaster Pro 2 под DOS.  *
*  Компилируется под 3dfx Glide SDK 2.43 и Open Watcom 2.0 beta.        *
************************************************************************/

// MAIN.C - запуск

// Число заранее подготовленных
// текстур с номерами в папке TEXTURES
// На значениях =192 =256 =512 выдаёт Stack Overflow
#define MAX_TEXTURES       32

#define TEXTURE_DATA_SIZE  131072
#define MAX_TMU            3

#include <stdarg.h>
#include <i86.h>   // для MK_FP
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <dos.h>
#include <malloc.h>

#include "mesh.h"
#include "keyboard.h"
#include "camera.h"
#include "timer.h"
#include "file.h"
#include "text.h"
#include "texture.h"
#include "primitiv.h" // Наборы примитивов для рендера
#include "matrix.h"   // Матричные функции
#include "fastmath.h"
#include "glideutl.h"
#include "glide.h"
#include "SOUND.H"

//GrVertex* tempVtxBuffer;

int SafeReturn() {

   //free(tempVtxBuffer);
   grSstWinClose();
   grGlideShutdown();
   timer_shutdown();
   keyboard_shutdown();
   return 0;
}

// Для Watcom в защищённом режиме (DOS4GW):
unsigned long GetFreeMemory(void)
{
   return (unsigned long)_memavl();  // Возвращает свободную память в байтах
}

int read_version() {
   FILE *fp;
   char* build = "build.txt";
   int value = 0;

   /* Пытаемся открыть файл для чтения */
   fp = fopen(build, "r");
   if (fp != NULL) {
      /* Если файл существует - читаем число */
      if (fscanf(fp, "%d", &value) != 1) {
         /* Если прочитать не удалось (пустой файл или не число) - начинаем с 0 */
         value = 0;
      }
      fclose(fp);
   } else {
      /* Файла нет - начнём с 0 */
      value = 0;
   }

   return value;
}

int main()
{
   //****************************************************************
   // Объявления переменных
   //****************************************************************
   
   
   int build = 0;

   //TextureSlot textures[MAX_TEXTURES];
   TextureSlot someTextureSlot;
   char someTextureName[] = "TEXTURES\\TEST.3DF";
   //char ebakotModelName[] = "MODELS\\SPHERE.OBJ";
   //char someModelName[] = "MODELS\\SKYBOX.OBJ";
   //char someModelName[] = "MODELS\\PLANE.OBJ";
   char someModelName[] = "MODELS\\CUBE.OBJ";

   // Нужно сначала переместить куб в начало координат
   // потом его крутануть
   // потом переместить его обратно
   // и делать это нужно каждый кадр
   float angleX, angleY, angleZ;

   float moveZ = 20;
   float moveZ_plus = 0.1f;

   GrHwConfiguration hwconfig;
   FxI32 num_tmu = 0; // Количество TMU

   
   int frame = 0;
   int current_texture = 0;

   int i, j, a, b;
   
   Font fpsFont;
   char fpsString[80];

   GrColor_t backColor;

   unsigned long frame_ticks_before;
   unsigned long frame_ticks_after;

   
   FxU32 max_tmu_memory[MAX_TMU];

   int max_textures[MAX_TMU]; // Количество допустимых текстур для TMU

   int key;
   int exit_now = 0;

   float frequency;
   float f1;
   float f2;
   float delta_time;
   unsigned long curr_ticks;
   unsigned long prev_ticks;
   unsigned long delta_ticks;
   float time = 0;
   float secondsConsole = 0;

   float stop = 2000.0f;

   int count = 0;

   float camera_rotation_speed = 45.0f;
   float camera_speed = 1;
   // float default_camera_yaw   = 180.0f;  // я так и не понял, почему только при этих
   // float default_camera_pitch = -4.0f;   // значениях камера смотрит на +Z
   // float default_camera_roll = 0.0f;   // значениях камера смотрит на +Z
   float default_camera_yaw   = 0.0f;  // я так и не понял, почему только при этих
   float default_camera_pitch = 0.0f;   // значениях камера смотрит на +Z
   float default_camera_roll = 0.0f;   // значениях камера смотрит на +Z

   Mesh* someMesh;
   
   int field_size = 11;
   int field_size_half = 5;

   unsigned char *synth;
   unsigned char *upbeat;
   unsigned char *gunshot;
   unsigned char *door;
   unsigned char *explosion;

   int player_shoots = 0;
   int door_opened = 0;
   int explosion_happens = 0;

   float player_shoots_time = 0;
   float player_shoots_time_rate = 0.5f;

   float door_opened_time = 0;
   float door_opened_time_rate = 0.7f;

   float explosion_happens_time = 0;
   float explosion_happens_time_rate = 1.0f;

   unsigned int synth_size, synth_rate, synth_channels;
   unsigned int upbeat_size, upbeat_rate, upbeat_channels;
   unsigned int gunshot_size, gunshot_rate, gunshot_channels;
   unsigned int door_size, door_rate, door_channels;
   unsigned int explosion_size, explosion_rate, explosion_channels;

// **********************************************************************
// Здесь начинаются операции и функции
// **********************************************************************
   build = read_version();

   srand(dos_time);

   init_fast_math();
   generate_base_models();

   f1 = (float)BASE_FREQUENCY;
   f2 = (float)CUSTOM_DIVIDER;
   frequency = f1 / f2;
   printf("New Timer frequency: %.2f Hz\n", frequency);
   
   keyboard_init();
   timer_init();

   prev_ticks = dos_time;

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

   is_playing = 1;  // Запускаем звуковой поток
   first_run = 1;   // Чтобы инициализировать DSP
   //dsp_write(DSP_CMD_SPEAKER_ON);  // Включить колонки!

   
   // getch();

   // Загружаем звуки (данные уже в памяти)
   synth =     load_wav("MUSIC/SYNTH.WAV", &synth_size, &synth_rate, &synth_channels);
   upbeat =    load_wav("MUSIC/UPBEAT.WAV", &upbeat_size, &upbeat_rate, &upbeat_channels);
   gunshot =   load_wav("SOUNDS/GUNSHOT.WAV", &gunshot_size, &gunshot_rate, &gunshot_channels);
   door =      load_wav("SOUNDS/DOOR.WAV", &door_size, &door_rate, &door_channels);
   explosion = load_wav("SOUNDS/EXPLODE.WAV", &explosion_size, &explosion_rate, &explosion_channels);
   
   //test_sound_generator();
   //test_stereo();
   //mixer_play_sound(-1, gunshot, gunshot_size, gunshot_rate, 0, 0);
   //mixer_play_sound(-1, upbeat, upbeat_size, upbeat_rate, 1, 0);
   mixer_play_sound_ex(-1, synth, synth_size, synth_rate, 2, 1, 0);

   //play_sound("MUSIC\\UPBEAT.WAV");

   // Предварительно загружаем звуки в каналы
   // mixer_play_sound(0, gunshot, gunshot_size, gunshot_rate, 0, 0); // Канал 0 - выстрелы
   // mixer_play_sound(1, door, door_size, door_rate, 0, 1);          // Канал 1 - двери
   // mixer_play_sound(2, explosion, explosion_size, explosion_rate, 0, 0); // Канал 2 - взрывы

   // iNIT GlIDE
   printf("[INIT] We want THE 3D GRAPHIX from 3dfx Voodoo!\n");
   grGlideInit(); puts("grGlideInit() OK");
   if (!grSstQueryHardware(&hwconfig)) {
      // Обработка ошибки: Voodoo не найден
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
      2, // Два стандартных буфера, передний и задний
          // Передний отображаем, в заднем рисуем
          // потом меняем местами. Получается плавная анимация.
      1 // Дополнительные буферы, например для Z-буфера
   ); puts("grSstWinOpen OK");

   // Не делаем ненужных отрисовок пикселей за пределами экрана
   grClipWindow(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

   // Я потратил кучу времени, пытаясь понять,
   // почему треугольники не рисуются,
   // пока не поставил здесь задержку
   //sleep(1);
   delay(1);

   // Здесь грузим шрифт
   //if (LoadFont("TEXTURES\\font_sqr.3df", &fpsFont) != 0) {
   if (LoadFont("TEXTURES\\font.3df", &fpsFont) != 0) {
      printf("Failed to load font!\n");
      SafeReturn();
      return 1;
   }

   // Определяем количество текстурной памяти и количество TMU
   num_tmu = hwconfig.SSTs[0].sstBoard.VoodooConfig.nTexelfx;;
   if (num_tmu < 0 || num_tmu > 3) {
      printf("Found incorrect number of TMUs: %d\n", (int)num_tmu);
      SafeReturn();
      return 0;
   }
   else {
      printf("Found TMUs: %d\n", (int)num_tmu);
   }
   
   if (LoadTexture(someTextureName, &someTextureSlot, 0) != 0) {
      printf("Error loading %s\n", someTextureName);
      SafeReturn();
      return 1;
   }

   // Попробуем загрузить модель корабля
   someMesh = LoadMeshFromOBJ(someModelName);
   if (!someMesh) {
      printf("Failed to load mesh %s\n", someModelName);
      SafeReturn();
      return 1;
   }

   grDepthBufferMode( GR_DEPTHBUFFER_WBUFFER );
   //grDepthBufferMode( GR_DEPTHBUFFER_ZBUFFER );
   grDepthBufferFunction( GR_CMP_LEQUAL );
   grDepthMask(FXTRUE);

   //grCullMode(GR_CULL_DISABLE); // Все треугольники видны
   //grCullMode(GR_CULL_POSITIVE); // Видимые треугольники по часовой стрелке
   grCullMode(GR_CULL_NEGATIVE); // Видимые треугольники против часовой стрелки - стандарт для Blender

   SetupCamera(
      0, 30, -7, // position
      75, 0, 0,  // rotation
      90, 4.0f/3.0f, // FOV, aspect
      3.0f, 100.0f); // near_clip, far_clip

   //printf("Free memory: %lu bytes\n", GetFreeMemory());  // Watcom-specific

   delay(1);

   // Главный цикл отрисовки
   while (1)
   {
      mixer_process();

      triangles_drawn = 0;

      backColor = 0x00005500; // ABGR - Dark Blue Screen
      grBufferClear(backColor, 0, GR_WDEPTHVALUE_FARTHEST);

      frame++;

      curr_ticks = dos_time;
      delta_ticks = curr_ticks - prev_ticks;   // разница в тиках
      delta_time = (float)delta_ticks / frequency;   // секунды
      time += delta_time;
      secondsConsole += delta_time;

      // Чтобы не печатало в консоль слишком часто
      if (secondsConsole >= 1.0f) {
         //printf("Frame: %d Delta ticks: %d Delta time: %.6f FPS: %.1f\n", frame, delta_ticks, delta_time, 1.0f/delta_time);
         //fflush(stdout);
         secondsConsole = 0;

         // Чтобы текст с фпс не менялся слишком часто
         sprintf(fpsString, "FPS: %.0f  (%.4f)\n", 1.0f / delta_time, delta_time);
      }

      prev_ticks = curr_ticks;

      exit_now = 0;

      if (key_states[KEY_ESCAPE]) break; // ESC
      
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

      // Чтобы нажималось только один раз для F6
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

      // Тестовый звук 1 (игрок стрелять)
      if (key_states[KEY_1] == 1 && key_states_prev[KEY_1] == 0) {
         player_shoots = 1;
         key_states_prev[KEY_1] = 1;
      }
      if (key_states[KEY_1] == 0) {
         key_states_prev[KEY_1] = 0;
      }

      // Тестовый звук 2 (дверь открылась)
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

      // Тестовый звук 3 (произошёл взрыв)
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

      // Обновим матрицу view и proj 
      // с новыми данными перемещения и поворота камеры
      UpdateMatricesViewProj();

      //angleX += 45.0f * delta_time;
      angleY += 360.0f * delta_time;
      //angleZ += 60.0f * delta_time;

      // Нормализуем углы в диапазон [0, 360)
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
      
      // Текст выводим в последнюю очередь
      // Вывод FPS (цвет белый = 0xFFFFFFFF)
      DrawTextF(&fpsFont, 8.0f, 8, COLOR_WHITE, "ANABIO.SIS BY SHIKOIST build %d", build);
      DrawText(&fpsFont, 8.0f, 8 + 16, COLOR_MAGENTA, fpsString);
      DrawTextF(&fpsFont, 320.0f, 8 + 16, COLOR_MAGENTA, "TIME: %.2f", time);
      DrawTextF(&fpsFont, 8.0f, 8 + 16*2, COLOR_MAGENTA, "Cam Pos: %.1f %.1f %.1f", camera.pos_x, camera.pos_y,camera.pos_z);
      DrawTextF(&fpsFont, 8.0f, 8 + 16*3, COLOR_MAGENTA, "Triangles: %d (%d/sec)", triangles_drawn, triangles_drawn * (int)(1.0f / delta_time));
      DrawTextF(&fpsFont, 8.0f, 8 + 16*4, COLOR_MAGENTA, "irqs: %d | status: %d | read: %d", counter, irq_debug_last_status, irq_debug_read_data);

      //grSstIdle();
      grBufferSwap(0);
      
      //delay(2);

      // И в конце каждого кадра (для отладки)
      // if (frame % 30 == 0) {
      //    printf("Frame %d, free memory: %lu bytes\n", frame, GetFreeMemory());
      // }

      // Тестовый брик для одного прохода
      //break;

      // Обработка звука
      mixer_process();

      // Тестовый звук 1 - выстрел (КЛАВИША 1)
      if (player_shoots) {
         int ch;
         int dir;
         dir = (int)(rand() % 255);
         //printf("dir = %d\n", dir);
         //mixer_play_sound(-1, gunshot, gunshot_size, gunshot_rate, 0, 0);
         ch = mixer_play_sound_ex(-1, gunshot, gunshot_size, gunshot_rate,
                             gunshot_channels, 0, 0);
         if (ch >= 0) mixer_set_pan(ch, dir);   // 0 крайний слева, 255 крайний справа
         player_shoots = 0;
      }

      // Тестовый звук 2 - дверь (КЛАВИША 2)
      if (door_opened) {
         mixer_play_sound(-1, door, door_size, door_rate, 0, 1);
         door_opened = 0;
      }

      // Тестовый звук 3 - взрыв (КЛАВИША 3)
      if (explosion_happens) {
         mixer_play_sound(-1, explosion, explosion_size, explosion_rate, 0, 0);
         explosion_happens = 0;
      }
      
      // Играем музыку по DMA
      // if (dma_block_finished_flag) {
      //    //sb_copy_buffer();
      //    swap_buffers();
         
      //    if (playing_final_chunk) {
      //          is_playing = 0;
      //    }

      //    dma_block_finished_flag = 0;
      //}
      //delay(1);
   }
   
   SafeReturn();
   
   UnloadMesh(someMesh);

   sb_stop_playback();
   //sb_cleanup();
   cleanup_buffers();
   cleanup_irq_handler();

   free(gunshot);
   free(door);
   free(explosion);

   return 0;
}
