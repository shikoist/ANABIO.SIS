// PRIMITIV.H - Заголовочный файл для примитивных моделей

#ifndef PRIMITIV_H
#define PRIMITIV_H

#define MAX_CUBE_VERTICES 36

#include "glide.h"
#include "glideutl.h"
#include "texture.h"
#include "camera.h"

extern int triangles_drawn;

// Модель куба из 36 вершин
//extern GrVertex cube_model[MAX_CUBE_VERTICES];

// Прототипы
void generate_base_models();
void DrawTexturedCubeAt(
    float pos_x, float pos_y, float pos_z,
    float rot_x, float rot_y, float rot_z,
    TextureSlot* textureSlot
);
void DrawWorldGridAndAxes();

#endif
