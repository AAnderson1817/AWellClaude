// Toolchain probe: 320x180 render target, integer upscale, headless screenshot path.
#include "raylib.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

#define GW 320
#define GH 180

static RenderTexture2D rt;
static int frame = 0;
static int shotFrames[8]; static int shotCount = 0; static int shotDone = 0;
static const char *outDir = "shots";

static void Frame(void) {
    frame++;
    BeginTextureMode(rt);
        ClearBackground((Color){14,12,20,255});
        for (int y = 0; y < GH; y += 8) for (int x = 0; x < GW; x += 8)
            if (((x/8) ^ (y/8)) & 1) DrawRectangle(x, y, 8, 8, (Color){22,20,30,255});
        int px = 40 + (frame*2) % 240;
        DrawRectangle(px, 100, 8, 12, (Color){220,210,180,255});
        DrawCircle(160, 60, 20 + (frame%30)*0.4f, (Color){70,120,160,255});
    EndTextureMode();

    int sc = GetScreenWidth()/GW; int sy = GetScreenHeight()/GH;
    if (sy < sc) sc = sy; if (sc < 1) sc = 1;
    int ox = (GetScreenWidth() - GW*sc)/2, oy = (GetScreenHeight() - GH*sc)/2;
    BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(rt.texture, (Rectangle){0,0,GW,-GH},
            (Rectangle){ox,oy,GW*sc,GH*sc}, (Vector2){0,0}, 0, WHITE);
    EndDrawing();

    for (int i = 0; i < shotCount; i++) if (shotFrames[i] == frame) {
        char path[256]; snprintf(path, sizeof path, "%s/f%04d.png", outDir, frame);
        TakeScreenshot(path);
        printf("shot %s\n", path); fflush(stdout);
        if (i == shotCount-1) shotDone = 1;
    }
}

int main(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--shots") && i+1 < argc) {
            char *s = argv[++i], *tok = strtok(s, ",");
            while (tok && shotCount < 8) { shotFrames[shotCount++] = atoi(tok); tok = strtok(NULL, ","); }
        } else if (!strcmp(argv[i], "--out") && i+1 < argc) outDir = argv[++i];
    }
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(GW*4, GH*4, "probe");
    SetTargetFPS(60);
    rt = LoadRenderTexture(GW, GH);
    SetTextureFilter(rt.texture, TEXTURE_FILTER_POINT);
#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(Frame, 0, 1);
#else
    while (!WindowShouldClose() && !shotDone) Frame();
#endif
    CloseWindow();
    return 0;
}
