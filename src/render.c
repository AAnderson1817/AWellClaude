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
Color palBrine  = { 30, 58, 80, 255 };
Color palGrass  = { 52, 74, 58, 255 };
Color palSkin   = {206,197,168, 255 };
Color palBrineL = { 92,138,150, 255 };   // brine surface / droplets
Color palSalt   = {150,154,156, 255 };   // salt crust
Color palSaltLit= {198,202,204, 255 };
Color palDust   = {126,118,110, 255 };
Color palTimber = { 88, 72, 56, 255 };
Color palIron   = { 66, 74, 78, 255 };
Color palSkinWet= {118,127,124, 255 };   // the part of you that is under the brine

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
                    int open   = (y == 0) || !TileSolid(r->tiles[y - 1][x]);
                    int buried = (y > 1) && (y < RH - 1)
                              && TileSolid(r->tiles[y - 1][x]) && TileSolid(r->tiles[y + 1][x]);
                    DrawRectangle(px, py, TS, TS, buried ? palRockDeep : palRock);
                    if (open) {
                        DrawRectangle(px, py, TS, 1, palRockLit);
                        DrawRectangle(px, py + 1, TS, 1, palRock);
                    }
                } break;

                case T_DARK:
                    DrawRectangle(px, py, TS, TS, palDark);
                    break;

                case T_LEDGE:
                    DrawRectangle(px, py, TS, 2, palRockLit);
                    break;

                case T_TIMBER: {
                    DrawRectangle(px, py, TS, 3, palTimber);
                    DrawRectangle(px, py, TS, 1, (Color){118, 98, 76, 255});
                    if (((x * 5 + y * 3) & 3) == 0) DrawRectangle(px + 3, py, 1, 3, palRockDeep);
                } break;

                case T_RIM: {
                    // A rim has to read as a thing you can act on without a single
                    // word saying so: a worn iron lip, brighter than any stone, with a
                    // dark channel behind it and salt crusted in the channel.
                    DrawRectangle(px, py, TS, TS, palIron);
                    DrawRectangle(px, py, TS, 1, (Color){138, 150, 156, 255});
                    DrawRectangle(px, py + 1, TS, 1, (Color){ 96, 106, 112, 255});
                    DrawRectangle(px, py + 3, TS, 2, (Color){ 38,  42,  46, 255});
                    DrawRectangle(px + 1, py + 3, 1, 1, palSaltLit);
                    DrawRectangle(px + 5, py + 4, 1, 1, palSalt);
                } break;

                case T_CRUST: {
                    // Fatigue is drawn, not counted. A crust you have been standing on
                    // shows hairline cracks that knit back if you leave it alone.
                    u8 st = scratch.stress[y][x];
                    int open = (y == 0) || !TileSolid(r->tiles[y - 1][x]);
                    DrawRectangle(px, py, TS, TS, palSalt);
                    if (open) DrawRectangle(px, py, TS, 1, palSaltLit);
                    if (st > 3) {
                        int n = st / 5; if (n > 4) n = 4;
                        for (int i = 0; i < n; i++) {
                            int cx = px + 1 + ((x * 7 + i * 3) % 6);
                            int cy = py + 2 + ((y * 5 + i * 2) % 5);
                            DrawRectangle(cx, cy, 1, 1, palRockDeep);
                            if (st > 14 && i < 2) DrawRectangle(cx, cy + 1, 1, 1, palRockDeep);
                        }
                    }
                } break;

                case T_BRINE: {
                    int surface = (y == 0) || (r->tiles[y - 1][x] != T_BRINE);
                    if (surface) {
                        // The 1D wave displaces the surface line only; the body of the
                        // brine stays put, which keeps it readable at 8px.
                        int d = (int)(scratch.surfH[x] * 3.0f);
                        if (d >  3) d =  3;
                        if (d < -3) d = -3;
                        DrawRectangle(px, py + d, TS, TS - d, palBrine);
                        DrawRectangle(px, py + d, TS, 1, palBrineL);
                    } else {
                        DrawRectangle(px, py, TS, TS, palBrine);
                        if (((x * 3 + y * 7 + (int)(frameNo / 26)) % 11) == 0)
                            DrawRectangle(px + 2, py + 3, 1, 1, (Color){44, 76, 100, 255});
                    }
                } break;

                case T_GRASS: {
                    // Salt fringe. Leans away from the player as they pass. Solves nothing.
                    float dx = (player.x + player.w * 0.5f) - (px + 4.0f);
                    int lean = 0;
                    if (dx > -14.0f && dx < 14.0f) lean = (dx > 0) ? -1 : 1;
                    DrawRectangle(px + 1 + lean, py + 5, 1, 3, palGrass);
                    DrawRectangle(px + 4 + lean, py + 4, 1, 4, palGrass);
                    DrawRectangle(px + 6 + lean, py + 6, 1, 2, palGrass);
                } break;

                default: break;
            }
        }
    }
}
