// object.h - заголовочный файл: концепция игрового объекта в 3д-мире

#ifndef OBJECT_H
#define OBJECT_H

#include "mesh.h"

#define MAX_OBJECTS 128

typedef struct {
    float       pos_x;
    float       pos_y;
    float       pos_z;
    Mesh        mesh;
} Object;

int InitObjectSystem();
int CreateObject(const char *object_name);

#endif
