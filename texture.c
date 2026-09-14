// texture.c - операции с текстурами
#include "texture.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "file.h"
#include "glide.h"
#include "text.h"

static FxU32 currentTexAddr[3] = {0, 0, 0}; // На каждый TMU

int LoadTexture(const char* filename, TextureSlot* slot, int tmu)
{
    Gu3dfInfo guInfo;

    //currentTexAddr[0] = grTexMinAddress(GR_TMU0); // Столько занимает шрифт выше

    if (tmu < 0 || tmu > 3) {
        printf("[LOAD] Found incorrect number of TMUs: %d\n", tmu);
        return -1;
    }

    memset(&guInfo, 0, sizeof(guInfo));

    strncpy(slot->filename, filename, sizeof(slot->filename)-1);
    //slot->grInfo.data = NULL;
    //slot->baseAddr = 0;

    if (!FileExists(filename))
    {
        printf("[ERROR] File %s is not found\n", filename);
        return -4;
    }
    //printf("Loading %s ...\n", filename);
    
    // 1. Читаем заголовок
    if (gu3dfGetInfo(filename, &guInfo) == FXFALSE) {
        printf("Error on gu3dfGetInfo on %s\n", filename);
        return -1;
    }
    
    // 2. Выделяем память
    guInfo.data = malloc(guInfo.mem_required);
    if (!guInfo.data)
    {
        printf("Error on malloc, %d memory required \n", guInfo.mem_required);
        return -2;
    }
    //printf("malloc %d\n", guInfo.mem_required);
    if (currentTexAddr[tmu] + guInfo.mem_required > grTexMaxAddress(tmu))
    {
        printf("WARNING: Not enough texture memory in TMU%d for texture %s! %d > %d\n",
            tmu,
            filename,
            currentTexAddr[tmu] + guInfo.mem_required,
            grTexMaxAddress(tmu)
            );
    }
    slot->baseAddr = currentTexAddr[tmu];
    
    
    slot->mem_required = guInfo.mem_required;
    //slot->grInfo.data = malloc(guInfo.mem_required);
    
    //if (!slot->grInfo.data)
    //{
    //    printf("Error on malloc, %d memory required \n", guInfo.mem_required);
    //    return -2;
    //}
    //printf("malloc %d\n", guInfo.mem_required);

    // 3. Пишем указатель, куда будут загружены данные текстуры
    //guInfo.data = slot->grInfo.data;
    if (gu3dfLoad(filename, &guInfo) == FXFALSE)
    {
        printf("Error on gu3dfLoad\n");
        free(guInfo.data);
        guInfo.data = NULL;
        //free(slot->grInfo.data);
        //slot->grInfo.data = NULL;
        
        return -3;
    }
    //puts("gu3dfLoad OK");

    // 4. Заполняем GrTexInfo
    slot->grTexInfo.smallLod    = guInfo.header.small_lod;
    slot->grTexInfo.largeLod    = guInfo.header.large_lod;
    slot->grTexInfo.aspectRatio = guInfo.header.aspect_ratio;
    slot->grTexInfo.format      = guInfo.header.format;
    slot->grTexInfo.data        = guInfo.data; // Указатель на данные текстуры
    slot->tmu                   = tmu;
    slot->width                 = guInfo.header.width;
    slot->height                = guInfo.header.height;

    grTexDownloadMipMap(tmu,
        currentTexAddr[tmu],
        GR_MIPMAPLEVELMASK_BOTH,
        //&slot->grInfo);
        &slot->grTexInfo);
    
    // Освобождаем host-память сразу после загрузки в VRAM
    free(guInfo.data);
    guInfo.data = NULL;
    //free(slot->grInfo.data);
    //slot->grTexInfo.data = NULL;   // больше не используем

    

    //printf("[TMU%d] texture %s loaded at: %d\n", tmu, filename, (unsigned int)slot->baseAddr);


    currentTexAddr[tmu] += guInfo.mem_required; // Адрес для следующей текстуры
    return 0;
}
