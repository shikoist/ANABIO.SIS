#ifndef TEXT_H
#define TEXT_H

#include "glide.h"

#define COLOR_ABGR(a,b,g,r)  ((FxU32)(((a)&0xFF)<<24) | (((b)&0xFF)<<16) | (((g)&0xFF)<<8) | ((r)&0xFF))

#define COLOR_WHITE   COLOR_ABGR(255,255,255,255)
#define COLOR_RED     COLOR_ABGR(255,  0,  0,255)
#define COLOR_GREEN   COLOR_ABGR(255,  0,255,  0)
#define COLOR_BLUE    COLOR_ABGR(255,255,  0,  0)
#define COLOR_YELLOW  COLOR_ABGR(255,  0,255,255)
#define COLOR_CYAN    COLOR_ABGR(255,255,255,  0)
#define COLOR_MAGENTA COLOR_ABGR(255,255,  0,255)

extern int triangles_drawn;

typedef struct {
    GrTexInfo   grTexInfo;
    FxU32       baseAddr;
    int         charW;
    int         charH;
    float       texW;
    float       texH;
    FxU8        fontTable[128][2];
} Font;

int  LoadFont(const char* filename, Font* font);           // loading
void DrawText(Font* font, float x, float y, FxU32 color, const char* str);
void DrawTextF(Font* font, float x, float y, FxU32 color, const char* fmt, ...);


#endif
