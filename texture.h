#ifndef TEXTURE_H
#define TEXTURE_H

#include "glide.h"

typedef struct {
    GrTexInfo      grTexInfo;      // Glide-структура для grTexSource / grTexDownloadMipMap
    FxU32          baseAddr;    // адрес в текстурной памяти TMU
    char           filename[256];
    FxU32          mem_required;
    int            tmu;
    FxU32          width;
    FxU32          height;
} TextureSlot;

int LoadTexture(const char* filename, TextureSlot* slot, int tmu);

#endif
