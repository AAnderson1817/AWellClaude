#include "aw.h"
#include <math.h>

RenderTexture2D screenRT;
static Shader post;
static int uTexSize, uOutSize, uTime;

Color palBg     = { 17, 16, 26, 255 };
Color palBgDeep = {  6,  6, 10, 255 };
Color palRockDeep = { 34, 32, 44, 255 };
Color palRock   = { 54, 51, 68, 255 };
Color palRockLit= {103, 98,126, 255 };
Color palDark   = {  0,  0,  0, 255 };
Color palWater  = { 30, 58, 80, 255 };
Color palGrass  = { 52, 74, 58, 255 };
Color palSkin   = {206,197,168, 255 };

// The CRT pass. Not an overlay: the scanline falloff is modulated per-pixel by
// luminosity, so bright pixels bloom across the gap and dark ones sink into it.
// Bilinear on X only; Y is clamped to sharp pixel centres.
#if defined(PLATFORM_WEB)
static const char *FS =
"#version 100\n"
"precision mediump float;\n"
"varying vec2 fragTexCoord;\n"
"varying vec4 fragColor;\n"
"uniform sampler2D texture0;\n"
"uniform vec2 uTexSize;\n"
"void main() {\n"
"  vec2 uv = fragTexCoord;\n"
"  float py = (floor(uv.y * uTexSize.y) + 0.5) / uTexSize.y;\n"
"  vec3 c = texture2D(texture0, vec2(uv.x, py)).rgb;\n"
"  float lum = dot(c, vec3(0.299, 0.587, 0.114));\n"
"  float f = fract(uv.y * uTexSize.y);\n"
"  float d = abs(f - 0.5) * 2.0;\n"
"  float depth = mix(0.30, 0.04, smoothstep(0.0, 0.70, lum));\n"
"  c *= 1.0 - depth * pow(d, 1.55);\n"
"  vec2 q = uv - 0.5;\n"
"  float vig = clamp(1.0 - dot(q * vec2(1.0, 0.86), q * vec2(1.0, 0.86)) * 0.55, 0.0, 1.0);\n"
"  c *= mix(1.0, vig, 0.50);\n"
"  float dth = fract(sin(dot(floor(gl_FragCoord.xy), vec2(12.9898, 78.233))) * 43758.5453);\n"
"  c += (dth - 0.5) * (1.6 / 255.0);\n"
"  gl_FragColor = vec4(c, 1.0);\n"
"}\n";
#else
static const char *FS =
"#version 330\n"
"in vec2 fragTexCoord;\n"
"in vec4 fragColor;\n"
"out vec4 finalColor;\n"
"uniform sampler2D texture0;\n"
"uniform vec2 uTexSize;\n"
"void main() {\n"
"  vec2 uv = fragTexCoord;\n"
"  float py = (floor(uv.y * uTexSize.y) + 0.5) / uTexSize.y;\n"
"  vec3 c = texture(texture0, vec2(uv.x, py)).rgb;\n"
"  float lum = dot(c, vec3(0.299, 0.587, 0.114));\n"
"  float f = fract(uv.y * uTexSize.y);\n"
"  float d = abs(f - 0.5) * 2.0;\n"
"  float depth = mix(0.30, 0.04, smoothstep(0.0, 0.70, lum));\n"
"  c *= 1.0 - depth * pow(d, 1.55);\n"
"  vec2 q = uv - 0.5;\n"
"  float vig = clamp(1.0 - dot(q * vec2(1.0, 0.86), q * vec2(1.0, 0.86)) * 0.55, 0.0, 1.0);\n"
"  c *= mix(1.0, vig, 0.50);\n"
"  float dth = fract(sin(dot(floor(gl_FragCoord.xy), vec2(12.9898, 78.233))) * 43758.5453);\n"
"  c += (dth - 0.5) * (1.6 / 255.0);\n"
"  finalColor = vec4(c, 1.0);\n"
"}\n";
#endif

void RenderInit(void) {
    screenRT = LoadRenderTexture(GW, GH);
    // bilinear so the shader gets X interpolation for free; Y is snapped in the shader
    SetTextureFilter(screenRT.texture, TEXTURE_FILTER_BILINEAR);
    post = LoadShaderFromMemory(0, FS);
    uTexSize = GetShaderLocation(post, "uTexSize");
    uOutSize = GetShaderLocation(post, "uOutSize");
    uTime    = GetShaderLocation(post, "uTime");
}

void RenderBeginRoom(void) {
    BeginTextureMode(screenRT);
    ClearBackground(palBgDeep);
}

void RenderEndRoomAndPresent(void) {
    EndTextureMode();

    int sw = GetScreenWidth(), sh = GetScreenHeight();
    int s = sw / GW; int sy = sh / GH;
    if (sy < s) s = sy;
    if (s < 1) s = 1;
    int ox = (sw - GW * s) / 2, oy = (sh - GH * s) / 2;

    float ts[2] = { (float)GW, (float)GH };
    float os[2] = { (float)(GW * s), (float)(GH * s) };
    float t = (float)frameNo * DT;

    BeginDrawing();
        ClearBackground(BLACK);
        SetShaderValue(post, uTexSize, ts, SHADER_UNIFORM_VEC2);
        if (uOutSize >= 0) SetShaderValue(post, uOutSize, os, SHADER_UNIFORM_VEC2);
        if (uTime    >= 0) SetShaderValue(post, uTime, &t, SHADER_UNIFORM_FLOAT);
        BeginShaderMode(post);
            DrawTexturePro(screenRT.texture,
                (Rectangle){ 0, 0, (float)GW, -(float)GH },
                (Rectangle){ (float)ox, (float)oy, (float)(GW * s), (float)(GH * s) },
                (Vector2){ 0, 0 }, 0.0f, WHITE);
        EndShaderMode();
    EndDrawing();
}

// Autotiling shading: a tile with nothing above it catches the rim light.
void DrawRoom(Room *r) {
    DrawRectangle(0, ROOM_Y, GW, RH * TS, palBg);

    for (int y = 0; y < RH; y++) {
        for (int x = 0; x < RW; x++) {
            u8 t = r->tiles[y][x];
            if (t == T_EMPTY) continue;
            int px = x * TS, py = ROOM_Y + y * TS;
            switch (t) {
                case T_ROCK: {
                    int open  = (y == 0)      || !TileSolid(r->tiles[y - 1][x]);
                    int buried = (y > 1) && (y < RH - 1)
                              && TileSolid(r->tiles[y - 1][x]) && TileSolid(r->tiles[y + 1][x]);
                    DrawRectangle(px, py, TS, TS, buried ? palRockDeep : palRock);
                    if (open) {
                        DrawRectangle(px, py, TS, 1, palRockLit);          // rim light
                        DrawRectangle(px, py + 1, TS, 1, palRock);
                    }
                } break;
                case T_DARK:
                    DrawRectangle(px, py, TS, TS, palDark);
                    break;
                case T_LEDGE:
                    DrawRectangle(px, py, TS, 2, palRockLit);
                    break;
                case T_WATER: {
                    int surface = (y == 0) || (r->tiles[y - 1][x] != T_WATER);
                    DrawRectangle(px, py, TS, TS, palWater);
                    if (surface) DrawRectangle(px, py, TS, 1, palRockLit);
                } break;
                case T_GRASS:
                    DrawRectangle(px + 1, py + 5, 1, 3, palGrass);
                    DrawRectangle(px + 4, py + 4, 1, 4, palGrass);
                    DrawRectangle(px + 6, py + 6, 1, 2, palGrass);
                    break;
                default: break;
            }
        }
    }
}
