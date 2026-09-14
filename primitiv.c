// PRIMITIV.С - отрисовка на экране примитивных моделей

// GrVertex v = {
//    -1.0f, -1.0f, 1.0f,           // x y z
//    255, 0, 0, 255,               // r g b a
//    0.0f,                         // ooz
//    1.0f,                         // oow = 1/w
//    0.0f, 0.0f                    // s0 t0 для TMU0
    // если несколько TMU — дальше tmuVtx[1][0], [1][1] и т.д.
//};

/*
    Всё ешё не могу понять, почему в разных треугольниках
    текстура маппится по-разному.
    Например, для шестой грани текстурные 0, 0
    находятся слева сверху.
    А для красной грани текстурные 0, 0 находятся
    в левом нижнем углу.
*/

/*
typedef struct
{
  float x, y, z;                // X, Y, and Z of scrn space -- Z is ignored
  float r, g, b;                // R, G, B, ([0..255.0])
  float ooz;                    // 65535/Z (used for Z-buffering)
  float a;                      // Alpha [0..255.0]
  float oow;                    // 1/W (used for W-buffering, texturing)
  GrTmuVertex  tmuvtx[GLIDE_NUM_TMU];
} GrVertex;
*/

#define CUBE_SIZE 1.0f

#include <string.h>
#include <stdio.h>

#include "primitiv.h"
#include "matrix.h"
#include "glide.h"
#include "glideutl.h"

typedef struct {
    float x, y, z;
    unsigned char r, g, b;
} FaceInfo;

GrVertex cube_model[MAX_CUBE_VERTICES];

void generate_base_models() {
    int idx = 0;
    int f, i, j;
    float nx, ny, nz;
    unsigned char cr, cg, cb;
    
    // Базовые 4 угла грани в локальной системе координат грани
    float u[4] = {0.0f, 1.0f, 1.0f, 0.0f};
    float v[4] = {0.0f, 0.0f, 1.0f, 1.0f};
    
    // Индексы для двух треугольников: 0-1-2 и 0-2-3
    //             2--3
    //             | \|    3
    //             1--0  / |
    //        3--2     2---0
    //        |/ |     | /
    //        0--1     1
    int tri_front[6] = {0,1,2, 0,2,3};
    int tri_back[6]  = {1,0,2, 0,3,2};
    
    int corner_front, corner_back;
    float px = 0, py = 0, pz = 0;
    float s, t;
    GrVertex *vtx;

    // 6 граней: нормаль (направление), цвет
    FaceInfo faces[6] = {
        { 0,  0,  1, 255,   0,   0 },  // +Z  красная
        { 0,  0, -1,   0, 255,   0 },  // -Z  зелёная
        { 0,  1,  0,   0,   0, 255 },  // +Y  синяя
        { 0, -1,  0, 255, 255,   0 },  // -Y  жёлтая
        {-1,  0,  0, 255,   0, 255 },  // -X  маджента
        { 1,  0,  0,   0, 255, 255 }   // +X  голубая
    };

    for (f = 0; f < 6; f++) {
        nx = faces[f].x;
        ny = faces[f].y;
        nz = faces[f].z;
        cr = faces[f].r;
        cg = faces[f].g;
        cb = faces[f].b;

        // Для каждой грани генерируем 2 треугольника (6 вершин)
        // Порядок вершин важен для правильной ориентации (counter-clockwise)
        for (i = 0; i < 6; i++) {
            corner_front = tri_front[i];
            corner_back = tri_back[i];
            
            // Проецируем на нужную грань
            if (nx == 1) {       // грань голубая X = +1
                py = (u[corner_front]*2.0f - 1.0f) * CUBE_SIZE;
                pz = (v[corner_front]*2.0f - 1.0f) * CUBE_SIZE;
                px = nx * CUBE_SIZE;
                s = v[corner_front];
                t = u[corner_front];
            }
            else if (nx == -1) { // грань маджента X = -1
                py = (u[corner_back]*2.0f - 1.0f) * CUBE_SIZE;
                pz = (v[corner_back]*2.0f - 1.0f) * CUBE_SIZE;
                px = nx * CUBE_SIZE;
                s = 1 - v[corner_back];
                t = u[corner_back];
            }
            else if (ny == 1) { // грань синяя Y = +1
                px = (u[corner_back]*2.0f - 1.0f) * CUBE_SIZE;
                pz = (v[corner_back]*2.0f - 1.0f) * CUBE_SIZE;
                py = ny * CUBE_SIZE;
                s = u[corner_back];
                t = v[corner_back];
            }
            else if (ny == -1) { // грань жёлтая Y = -1
                px = (u[corner_front]*2.0f - 1.0f) * CUBE_SIZE;
                pz = (v[corner_front]*2.0f - 1.0f) * CUBE_SIZE;
                py = ny * CUBE_SIZE;
                s = u[corner_front];
                t = 1 - v[corner_front];
            }
            else if (nz == 1) { // грань красная Z = +1
                px = (u[corner_front]*2.0f - 1.0f) * CUBE_SIZE;
                py = (v[corner_front]*2.0f - 1.0f) * CUBE_SIZE;
                pz = nz * CUBE_SIZE;
                s = 1 - u[corner_front];
                t = v[corner_front];
            }
            else if (nz == -1) { // грань зелёная Z = -1
                px = (u[corner_back]*2.0f - 1.0f) * CUBE_SIZE;
                py = (v[corner_back]*2.0f - 1.0f) * CUBE_SIZE;
                pz = nz * CUBE_SIZE;
                s = u[corner_back];
                t = v[corner_back];
            }

            vtx = &cube_model[idx++];
            vtx->x   = px;
            vtx->y   = py;
            vtx->z   = pz;
            vtx->r   = cr;
            vtx->g   = cg;
            vtx->b   = cb;
            vtx->a   = 255;
            vtx->oow = 1.0f;

            for (j = 0; j < GLIDE_NUM_TMU; j++) {
                vtx->tmuvtx[j].sow = s * 1.0f;
                vtx->tmuvtx[j].tow = t * 1.0f;
                vtx->tmuvtx[j].oow = 1.0f;
            }
        }
    }
}

void DrawTexturedCubeAt(
    float pos_x,
    float pos_y,
    float pos_z,
    float rot_x,
    float rot_y,
    float rot_z, 
    TextureSlot* textureSlot
) {
    GrState grState;

    // Матрица мира
    //float world[16];

    float model[16];      // Матрица модели (положение, поворот, масштаб объекта)
    //float view[16];       // Матрица вида (камера)
    float proj[16];       // Матрица проекции
    float mvp[16];        // Итоговая MVP матрица
       
    // Запоминаем значения tw (oow)
    //float check[MAX_CUBE_VERTICES];
    int i;
    //float oow;

    // Временный набор вершин для рендера
    GrVertex vtx[MAX_CUBE_VERTICES];
    GrVertex vtxProj[MAX_CUBE_VERTICES];

    // Прежде чем что-то менять в стейте, сохраним его
    grGlideGetState(&grState);

    // Так цвет берётся из цвета вершин
    //guColorCombineFunction(GR_COLORCOMBINE_ITRGB);
    
    // а так из текстуры
    //guColorCombineFunction(GR_COLORCOMBINE_TEXTURE);

    // Включим текстурирование (предполагаем, что текстура уже загружена и забиндена)
    //grTexCombineFunction(GR_TMU0, GR_COMBINE_FUNCTION_SCALE_OTHER,
    // GR_COMBINE_FACTOR_ONE, GR_COMBINE_LOCAL_NONE,
        //    GR_COMBINE_OTHER_TEXTURE, FXFALSE, FXFALSE);

    // grColorCombine(
        // GR_COMBINE_FUNCTION_LOCAL,          // или SCALE_OTHER_ADD_LOCAL и т.д.
        // GR_COMBINE_FACTOR_NONE,
        // GR_COMBINE_LOCAL_ITERATED,          
        // GR_COMBINE_OTHER_NONE,
        // FXFALSE                             // без инверсии
    // );
    

    if (textureSlot->tmu == 0) {
        grTexCombine(
            GR_TMU0,
            GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE,
            GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE,
            FXFALSE, FXFALSE);
        // grTexCombine(
        //     GR_TMU1,
        //     GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_ZERO,
        //     GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_ZERO,
        //     FXFALSE, FXFALSE);
        // grTexCombine(
        //     GR_TMU2,
        //     GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_ZERO,
        //     GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_ZERO,
        //     FXFALSE, FXFALSE);
        
    }
    else if (textureSlot->tmu == 1) {
        grTexCombine( // Отключаем TMU0 (выдаёт 0)
            GR_TMU0,
            GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_NONE,
            GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_NONE,
            FXFALSE, FXFALSE);
        grTexCombine(
            GR_TMU1,
            GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE,
            GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE,
            FXFALSE, FXFALSE);
        // grTexCombine(
        //     GR_TMU2,
        //     GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_ZERO,
        //     GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_ZERO,
        //     FXFALSE, FXFALSE);
    }
    else if (textureSlot->tmu == 2) {
        grTexCombine( // Отключаем TMU0 (выдаёт 0)
            GR_TMU0,
            GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_ZERO,
            GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_ZERO,
            FXFALSE, FXFALSE);
        grTexCombine(// Отключаем TMU1 (выдаёт 0)
            GR_TMU1,
            GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_ZERO,
            GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_ZERO,
            FXFALSE, FXFALSE);
        grTexCombine(
            GR_TMU2,
            GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE,
            GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE,
            FXFALSE, FXFALSE);
    }
    else { // Incorrect number of TMU
        grGlideSetState(&grState);
        printf("[ERROR] Incorrect number of TMU: %d", textureSlot->tmu);
        return;
    }

    // Только текстура
    grColorCombine(
        GR_COMBINE_FUNCTION_SCALE_OTHER,
        GR_COMBINE_FACTOR_LOCAL,
        GR_COMBINE_LOCAL_CONSTANT,
        GR_COMBINE_OTHER_TEXTURE,
        FXFALSE );

    // Добавляем к текстуре цвет вершин
    // grColorCombine(
    //     GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_LOCAL,
    //     GR_COMBINE_LOCAL_ITERATED, GR_COMBINE_OTHER_TEXTURE,
    //     FXFALSE );

    /*grAlphaCombine(
        GR_COMBINE_FUNCTION_SCALE_OTHER,
        GR_COMBINE_FACTOR_ONE,
        GR_COMBINE_LOCAL_NONE,
        GR_COMBINE_OTHER_TEXTURE,
        FXFALSE);

    grAlphaBlendFunction(
        GR_BLEND_SRC_ALPHA,
        GR_BLEND_ONE_MINUS_SRC_ALPHA,
        GR_BLEND_ONE,
        GR_BLEND_ZERO);*/
    
    grTexSource(textureSlot->tmu,
        textureSlot->baseAddr,
        GR_MIPMAPLEVELMASK_BOTH,
        &textureSlot->grTexInfo);

    //grTexMipMapMode(GR_TMU0, GR_MIPMAP_DISABLE, FXFALSE);
    //grTexFilterMode(GR_TMU0, GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_BILINEAR);
    //grTexClampMode(GR_TMU0, GR_TEXTURECLAMP_CLAMP, GR_TEXTURECLAMP_CLAMP);

    //grCoordinateSpace(GR_CLIP_COORDS);

    // Матрица мира
    //MatrixIdentity(world);
    //MatrixEulerRotation(world, rot_x, rot_y, rot_z);
    //MatrixTranslation(world, pos_x, pos_y, pos_z);

    //MatrixMultiply(world, world, camera);

    //MatrixPerspective(world, 60 * DEGTORAD, 640.0f / 480.0f, 5.0f, 200.0f);

    MatrixIdentity(     model);
    MatrixEulerRotation(model, rot_x, rot_y, rot_z);
    MatrixTranslation(  model, pos_x, pos_y, pos_z);
    
    //MatrixScale(model, 1.0f, 1.0f, 1.0f);

    // Матрица View обрабатывается в UpdateView (camera.c)
    // MatrixLookAt(view,
    //     0.0f, 0.0f, 0.0f,      // позиция глаза (камеры)
    //     0.0f, 0.0f, 50.0f,      // куда смотрит камера
    //     0.0f, 1.0f, 0.0f);     // направление "вверх"

    MatrixIdentity(proj);
    MatrixProjection(proj,
        camera.fov,          // FOV 60 градусов
        camera.aspect,       // aspect ratio 4:3
        camera.near_clip,    // near plane
        camera.far_clip);    // far plane

    MatrixIdentity(mvp);
    
    // Порядок MVP = Model x View x Projection,
    // потому что в проекте используется порядок Row Major
    // то есть строки матрицы хранятся последовательно
    MatrixMultiply(mvp, mvp, model);
    MatrixMultiply(mvp, mvp, view);
    MatrixMultiply(mvp, mvp, proj);
    // MatrixMultiply(mvp, model, view);
    // MatrixMultiply(mvp, mvp, proj);
    
    // Вот здесь бы определять и отбрасывать вершины,
    // находящиеся за пределами экрана или сзади камеры

    // Копируем вертексы из модели куба
    memcpy(vtx, cube_model, sizeof(cube_model));
    memcpy(vtxProj, cube_model, sizeof(cube_model));

    for (i = 0; i < MAX_CUBE_VERTICES; i++) {
        //check[i] = ApplyMatrix(&vtx[i], world);
        ApplyMatrix(&vtx[i], mvp);

        //oow = vtx[i].oow;
        //if (oow <= 1.0f) check[i] = -1;

        // Если у вас текстурные координаты хранятся в sow/tow 
        // уже в модели в размерах [0.0f, 1.0f] ,
        // то нужно их умножить на ширину и высоту текстуры:
        vtx[i].tmuvtx[textureSlot->tmu].sow *= vtx[i].oow * (unsigned long)textureSlot->width;   // s/w
        vtx[i].tmuvtx[textureSlot->tmu].tow *= vtx[i].oow * (unsigned long)textureSlot->height;   // t/w
        // vtx[i].tmuvtx[0].sow *= oow * textureSlot->width;   // s/w
        // vtx[i].tmuvtx[0].tow *= oow * textureSlot->height;   // t/w
        // vtx[i].tmuvtx[textureSlot->tmu].sow *= (unsigned long)textureSlot->width;   // s/w
        // vtx[i].tmuvtx[textureSlot->tmu].tow *= (unsigned long)textureSlot->height;   // t/w
        
        // printf("%.5f %.5f\n", vtx[i].tmuvtx[textureSlot->tmu].sow, vtx[i].tmuvtx[textureSlot->tmu].tow);
        //printf("%.5f %d %d\n", vtx[i].oow, textureSlot->width, textureSlot->height);

        VertexToScreen(&vtx[i], &vtxProj[i]);
    }

    for (i = 0; i < MAX_CUBE_VERTICES; i += 3)
    {
        // Если в треугольнике одна из вершин
        // с неправильным tw - не рисуем
        if (    vtxProj[i+0].oow > 0
             && vtxProj[i+1].oow > 0
             && vtxProj[i+2].oow > 0) {
            //grDrawTriangle(&vtxProj[i], &vtxProj[i+1], &vtxProj[i+2]);
            guAADrawTriangleWithClip(&vtxProj[i], &vtxProj[i+1], &vtxProj[i+2]);
            triangles_drawn++;
            //grDrawTriangle(&vtxProj[i], &vtxProj[i+1], &vtxProj[i+2]);
            // printf("%.1f %.1f %.1f %.1f %.1f|%.1f %.1f %.1f %.1f %.1f|%.1f %.1f %.1f %.1f %.1f\n",
            //    vtxProj[i+0].x, vtxProj[i+0].y, vtxProj[i+0].z, vtxProj[i+0].ooz, vtxProj[i+0].oow,
            //    vtxProj[i+1].x, vtxProj[i+1].y, vtxProj[i+1].z, vtxProj[i+1].ooz, vtxProj[i+1].oow,
            //    vtxProj[i+2].x, vtxProj[i+2].y, vtxProj[i+2].z, vtxProj[i+2].ooz, vtxProj[i+2].oow);
        }
        // if (   vtx[i+0].z >= 0.0f 
        //     && vtx[i+1].z >= 0.0f 
        //     && vtx[i+2].z >= 0.0f) {
        //     //grDrawTriangle(&vtxProj[i], &vtxProj[i+1], &vtxProj[i+2]);
        //     guAADrawTriangleWithClip(&vtx[i], &vtx[i+1], &vtx[i+2]);
        //     printf("%.1f %.1f %.1f %.1f %.1f %.1f %.1f %.1f %.1f %.1f %.1f %.1f\n",
        //         vtx[i+0].x, vtx[i+0].y, vtx[i+0].z, vtx[i+0].oow,
        //         vtx[i+1].x, vtx[i+1].y, vtx[i+1].z, vtx[i+1].oow,
        //         vtx[i+2].x, vtx[i+2].y, vtx[i+2].z, vtx[i+2].oow);
        // }
        //guAADrawTriangleWithClip(&vtx[i], &vtx[i+1], &vtx[i+2]);
    }
    grGlideSetState(&grState);
}

void ClipLine(GrVertex *va, GrVertex *vb) {
    // Предположим, что первая вершина находится за пределами экрана слева
    //    --------
    // a. |   b. |
    //    |      |
    //    --------
    if (va->x < 0 && va->y > 0) { va->x = 0; va->y = vb->y; }



    if (va->y < 0) va->y = 0;

    if (va->x > SCREEN_WIDTH - 1) va->x = SCREEN_WIDTH - 1;
    if (va->y > SCREEN_HEIGHT - 1) va->y = SCREEN_HEIGHT - 1;

    if (vb->x < 0) vb->x = 0;
    if (vb->x > SCREEN_WIDTH - 1) vb->x = SCREEN_WIDTH - 1;
    if (vb->y < 0) vb->y = 0;
    if (vb->y > SCREEN_HEIGHT - 1) vb->y = SCREEN_HEIGHT - 1;
}

//int ClipLineLiangBarsky(float *x0, float *y0, float *x1, float *y1,
int ClipLineLiangBarsky(GrVertex *va, GrVertex *vb) {
    float x0, y0, x1, y1;
    float new_x0, new_y0, new_x1, new_y1;
    float dx, dy;
    float u1, u2;
    int i;
    float t;
    float xmin, xmax, ymin, ymax;
    float p[4];
    float q[4];

    xmin = 0; xmax = SCREEN_WIDTH;
    ymin = 0; ymax = SCREEN_HEIGHT;

    x0 = va->x;
    y0 = va->y;
    x1 = vb->x;
    y1 = vb->y;

    dx = x1 - x0;
    dy = y1 - y0;

    // p-массивы для числителей, q-массивы для знаменателей
    p[0] = -dx; p[1] = dx; p[2] = -dy; p[3] = dy;
    q[0] = x0 - xmin; q[1] = xmax - x0; q[2] = y0 - ymin; q[3] = ymax - y0;

    u1 = 0.0f; u2 = 1.0f;

    for (i = 0; i < 4; i++) {
        if (p[i] == 0) {
            // Линия параллельна границе
            if (q[i] < 0) return -1; // Полностью невидима
        } else {
            t = q[i] / p[i];
            if (p[i] < 0) {
                // Вход в область
                if (t > u2) return -1;
                if (t > u1) u1 = t;
            } else {
                // Выход из области
                if (t < u1) return -1;
                if (t < u2) u2 = t;
            }
        }
    }

    if (u1 > u2) return -1; // Нет видимой части

    // Вычисляем новые координаты концов видимого отрезка
    va->x = x0 + u1 * dx;
    va->y = y0 + u1 * dy;
    vb->x = x0 + u2 * dx;
    vb->y = y0 + u2 * dy;

    return 0;
}


// Отладочный грид + цветные оси мира (белый грид + RGB-оси)
void DrawWorldGridAndAxes() {
    GrState grState;
    GrVertex va, vb;
    GrVertex tmpA, tmpB;
    // Строим матрицу view * proj (model = identity)
    float proj[16];
    float mvp[16];
    float grid_size = 100.0f;
    float grid_step = 10.0f;
    float x, z;
    // ====================== ЦВЕТНЫЕ ОСИ ======================
    float axis_len = 150.0f;

    grGlideGetState(&grState);

    // Отключаем все текстуры (работает на 1/2/3 TMU)
    grTexCombine(GR_TMU0,
        GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_ZERO,
        GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_ZERO,
        FXFALSE, FXFALSE);

    // Только постоянный цвет (без текстуры и без вершинных цветов)
    grColorCombine(
        GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE,
        GR_COMBINE_LOCAL_CONSTANT, GR_COMBINE_OTHER_NONE,
        FXFALSE);

    // Включаем Z-буфер, чтобы грид не перекрывал кубики неправильно
    grDepthMask(FXTRUE);
    grDepthBufferFunction(GR_CMP_LEQUAL);

    MatrixIdentity(proj);
    MatrixProjection(proj,
        camera.fov,
        camera.aspect,
        camera.near_clip,
        camera.far_clip);

    MatrixIdentity(mvp);
    MatrixMultiply(mvp, mvp, view);   // view уже обновлён в UpdateView()
    MatrixMultiply(mvp, mvp, proj);

    // ====================== БЕЛЫЙ ГРИД НА XZ ======================
    grConstantColorValue(0xFFFFFFFF);   // белый (ABGR)
    // Линии параллельно X (фиксированный Z)
    for (z = -grid_size; z <= grid_size + 0.01f; z += grid_step) {
        // A: (-grid_size, 0, z)
        tmpA.x = -grid_size; tmpA.y = 0.0f; tmpA.z = z; tmpA.oow = 1.0f;
        ApplyMatrix(&tmpA, mvp);
        VertexToScreen(&tmpA, &va);

        // B: (+grid_size, 0, z)
        tmpB.x =  grid_size; tmpB.y = 0.0f; tmpB.z = z; tmpB.oow = 1.0f;
        ApplyMatrix(&tmpB, mvp);
        VertexToScreen(&tmpB, &vb);

        if (va.oow <= 0.0f || vb.oow <= 0.0f) continue;

        // Clip by Screen
        if (ClipLineLiangBarsky(&va, &vb) == 0)

            //if (va.oow > 0.0f || vb.oow > 0.0f)
            // if (va.z > 0.0f && vb.z > 0.0f)
            // if (va.ooz > 0.0f && vb.ooz > 0.0f)
                //grDrawLine(&va, &vb);
                grAADrawLine(&va, &vb);
            
    }

    // Линии параллельно Z (фиксированный X)
    for (x = -grid_size; x <= grid_size + 0.01f; x += grid_step) {
        tmpA.x = x; tmpA.y = 0.0f; tmpA.z = -grid_size; tmpA.oow = 1.0f;
        ApplyMatrix(&tmpA, mvp);
        VertexToScreen(&tmpA, &va);

        tmpB.x = x; tmpB.y = 0.0f; tmpB.z =  grid_size; tmpB.oow = 1.0f;
        ApplyMatrix(&tmpB, mvp);
        VertexToScreen(&tmpB, &vb);

        if (va.oow <= 0.0f || vb.oow <= 0.0f) continue;

        // Clip by Screen
        if (ClipLineLiangBarsky(&va, &vb) == 0)

            //if (va.oow > 0.0f || vb.oow > 0.0f)
            // if (va.z > 0.0f && vb.z > 0.0f)
            // if (va.ooz > 0.0f && vb.ooz > 0.0f)
                //grDrawLine(&va, &vb);
                grAADrawLine(&va, &vb);
    }



    // X — КРАСНЫЙ
    grConstantColorValue(0xFF0000FF);   // ABGR: A=255, B=0, G=0, R=255
    
    tmpA.x = -axis_len; tmpA.y = 0; tmpA.z = 0; tmpA.oow = 1.0f;
    ApplyMatrix(&tmpA, mvp); VertexToScreen(&tmpA, &va);
    
    tmpB.x =  axis_len; tmpB.y = 0; tmpB.z = 0; tmpB.oow = 1.0f;
    ApplyMatrix(&tmpB, mvp); VertexToScreen(&tmpB, &vb);

    // Clip by Screen
    if (ClipLineLiangBarsky(&va, &vb) == 0)

        if (va.oow > 0.0f && vb.oow > 0.0f)
        // if (va.z > 0.0f && vb.z > 0.0f)
        // if (va.ooz > 0.0f && vb.ooz > 0.0f)
            //grDrawLine(&va, &vb);
            grAADrawLine(&va, &vb);

    // Y — ЗЕЛЁНЫЙ
    grConstantColorValue(0xFF00FF00);   // ABGR: A=255, B=0, G=255, R=0
    
    tmpA.x = 0; tmpA.y = -axis_len; tmpA.z = 0; tmpA.oow = 1.0f;
    ApplyMatrix(&tmpA, mvp); VertexToScreen(&tmpA, &va);
    
    tmpB.x = 0; tmpB.y =  axis_len; tmpB.z = 0; tmpB.oow = 1.0f;
    ApplyMatrix(&tmpB, mvp); VertexToScreen(&tmpB, &vb);

    // Clip by Screen
    if (ClipLineLiangBarsky(&va, &vb) == 0)

        if (va.oow > 0.0f && vb.oow > 0.0f)
        // if (va.z > 0.0f && vb.z > 0.0f)
        // if (va.ooz > 0.0f && vb.ooz > 0.0f)
            //grDrawLine(&va, &vb);
            grAADrawLine(&va, &vb);

    // Z — СИНИЙ
    grConstantColorValue(0xFFFF0000);   // ABGR: A=255, B=255, G=0, R=0
    
    tmpA.x = 0; tmpA.y = 0; tmpA.z = -axis_len; tmpA.oow = 1.0f;
    ApplyMatrix(&tmpA, mvp); VertexToScreen(&tmpA, &va);
    
    tmpB.x = 0; tmpB.y = 0; tmpB.z =  axis_len; tmpB.oow = 1.0f;
    ApplyMatrix(&tmpB, mvp); VertexToScreen(&tmpB, &vb);

    // Clip by Screen
    if (ClipLineLiangBarsky(&va, &vb) == 0)

        if (va.oow > 0.0f && vb.oow > 0.0f)
        // if (va.z > 0.0f && vb.z > 0.0f)
        // if (va.ooz > 0.0f && vb.ooz > 0.0f)
            //grDrawLine(&va, &vb);
            grAADrawLine(&va, &vb);

    // Восстанавливаем предыдущее состояние
    grGlideSetState(&grState);
}
