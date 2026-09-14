// text.c - отрисовка текста на экране
#include "text.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "glide.h"
#include "texture.h"

//static const char fontString[] = " !\"#$%&\'()*+,-.\/0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_\`abcdefghijklmnopqrstuvwxyz{|}~";
static const char fontString[] = " ! #$%& ()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[ ]^_ abcdefghijklmnopqrstuvwxyz{|}~";

int LoadFont(const char* filename, Font* font)
{
    TextureSlot slot;
    int i;

    if (LoadTexture(filename, &slot, 0) != 0)
        return -1;

    font->grTexInfo = slot.grTexInfo;
    font->baseAddr  = slot.baseAddr;
    font->charW     = 16;
    font->charH     = 16;
    font->texW      = 256.0f;
    font->texH      = 256.0f;

    // Заполняем таблицу UV (точно как в tlib, только проще)
    memset(font->fontTable, 0, sizeof(font->fontTable));

    for (i = 32; i < 128; i++) {           // от пробела до ~
        char* hit = strchr(fontString, (char)i);
        if (hit) {
            int offset = (int)(hit - fontString);
            int col = offset % font->charW;
            int row = offset / font->charH;

            font->fontTable[i][0] = (FxU8)(col * font->charW);   // x в текселях
            font->fontTable[i][1] = (FxU8)(row * font->charH);   // y в текселях
        }
    }

    // Скачиваем в TMU (если ещё не скачано в LoadTexture)
    //grTexDownloadMipMap(GR_TMU0, font->baseAddr, GR_MIPMAPLEVELMASK_BOTH, &font->grTexInfo);

    // Освобождаем host-память
    //if (slot.grTexInfo.data) {
    //    free(slot.grTexInfo.data);
    //    slot.grTexInfo.data = NULL;
    //}

    printf("[FONT] Loaded %s at address %u\n", filename, (unsigned)font->baseAddr);
    return 0;
}

/* Основная функция вывода текста */
void DrawText(Font* font, float x, float y, FxU32 color, const char* str)
{
    GrState state;
    GrVertex v[4];
    float curX = x;
    int i;
    float tx;
    float ty;

    if (!font || !str) return;

    grGlideGetState(&state);

    // Включаем отбрасывание чёрных пикселей
    grChromakeyMode(GR_CHROMAKEY_ENABLE);
    grChromakeyValue(0x00000000);

    // Красим в заданный цвет
    grConstantColorValue(color);
    //grConstantColorValue(0xFFFF0000);

    // Убираем размазывание для текста
    // Будем резаться об острые края пикселей
    grTexFilterMode(
        GR_TMU0,
        GR_TEXTUREFILTER_POINT_SAMPLED,
        GR_TEXTUREFILTER_POINT_SAMPLED);

    /* Настраиваем состояние только для текста */
    grColorCombine(
        GR_COMBINE_FUNCTION_SCALE_OTHER,
        GR_COMBINE_FACTOR_LOCAL,
        GR_COMBINE_LOCAL_CONSTANT,
        //GR_COMBINE_LOCAL_ITERATED,
        GR_COMBINE_OTHER_TEXTURE,
        FXFALSE);

    grTexCombine(
        GR_TMU0,
        GR_COMBINE_FUNCTION_LOCAL,
        GR_COMBINE_FACTOR_NONE,
        GR_COMBINE_FUNCTION_LOCAL,
        GR_COMBINE_FACTOR_NONE,
        FXFALSE,
        FXFALSE);

    grAlphaCombine(
        GR_COMBINE_FUNCTION_SCALE_OTHER,
        GR_COMBINE_FACTOR_ONE,
        GR_COMBINE_LOCAL_NONE,
        GR_COMBINE_OTHER_TEXTURE,
        FXFALSE);

    grAlphaBlendFunction(
        GR_BLEND_SRC_ALPHA,
        GR_BLEND_ONE_MINUS_SRC_ALPHA,
        GR_BLEND_ONE,
        GR_BLEND_ZERO);


    // Что-то странное тут творится
    //grDepthBufferFunction(GR_CMP_ALWAYS);
    //grDepthMask(FXFALSE);
    grCullMode(GR_CULL_DISABLE);
    


    grTexSource(GR_TMU0, font->baseAddr, GR_MIPMAPLEVELMASK_BOTH, &font->grTexInfo);
    //grTexSource(GR_TMU0, 131072, GR_MIPMAPLEVELMASK_BOTH, &font->grTexInfo);
    //grTexSource(GR_TMU0, font->baseAddr, GR_MIPMAPLEVELMASK_BOTH, NULL);
    //grTexSource(GR_TMU0, 0, GR_MIPMAPLEVELMASK_BOTH, &font->grInfo);

    while (*str) {
        unsigned char c = (unsigned char)*str++;
        //if (c < 32 || font->fontTable[c][0] == 0) {
        if (c < 32) {
            if (c == ' ') curX += font->charW;
            continue;
        }

        tx = font->fontTable[c][0];
        ty = font->fontTable[c][1];

        /* Четыре вершины */
        // GR_ORIGIN_LOWER_LEFT
        // v[0].x = curX;
        // v[0].y = y + font->charH;
        // v[1].x = curX + font->charW;
        // v[1].y = y + font->charH;
        // v[2].x = curX + font->charW;
        // v[2].y = y;
        // v[3].x = curX;
        // v[3].y = y;

        // GR_ORIGIN_UPPER_LEFT = WIP
        v[0].x = curX;
        v[0].y = y;
        v[1].x = curX + font->charW;
        v[1].y = y;
        v[2].x = curX + font->charW;
        v[2].y = y + font->charH;
        v[3].x = curX;
        v[3].y = y + font->charH;

        v[0].oow = 1.0f;
        v[1].oow = 1.0f;
        v[2].oow = 1.0f; 
        v[3].oow = 1.0f;

        /* Текстурные координаты (в формате Glide — sow/tow) */
        // GR_ORIGIN_LOWER_LEFT
        // v[0].tmuvtx[0].sow = tx;
        // v[0].tmuvtx[0].tow = ty;
        // v[1].tmuvtx[0].sow = tx + font->charW;
        // v[1].tmuvtx[0].tow = ty;
        // v[2].tmuvtx[0].sow = tx + font->charW;
        // v[2].tmuvtx[0].tow = ty + font->charH;
        // v[3].tmuvtx[0].sow = tx;
        // v[3].tmuvtx[0].tow = ty + font->charH;

        // GR_ORIGIN_UPPER_LEFT
        v[0].tmuvtx[0].sow = tx;
        v[0].tmuvtx[0].tow = ty;
        v[1].tmuvtx[0].sow = tx + font->charW;
        v[1].tmuvtx[0].tow = ty;
        v[2].tmuvtx[0].sow = tx + font->charW;
        v[2].tmuvtx[0].tow = ty + font->charH;
        v[3].tmuvtx[0].sow = tx;
        v[3].tmuvtx[0].tow = ty + font->charH;

        // GR_ORIGIN_LOWER_LEFT
        grDrawTriangle(&v[0], &v[1], &v[2]);
        grDrawTriangle(&v[0], &v[2], &v[3]);

        triangles_drawn+=2;
        
        // GR_ORIGIN_UPPER_LEFT
        //grDrawTriangle(&v[0], &v[3], &v[2]);
        //grDrawTriangle(&v[1], &v[3], &v[2]);

        curX += font->charW;
    }

    grGlideSetState(&state);   // восстанавливаем всё предыдущее состояние
}

void DrawTextF(Font* font, float x, float y, FxU32 color, const char* fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    va_end(ap);
    DrawText(font, x, y, color, buf);
}
