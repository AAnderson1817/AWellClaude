// render.c -- the palette, the 320x180 target, and the CRT pass over the top.
#include "aw.h"

RenderTexture2D screenRT;
static Shader post;
static int uTexSize;

// Everything is authored a stop brighter than it wants to look, because the light
// pass multiplies the whole frame down. A colour here is what a thing would be if
// something were shining directly on it.
Color palVoid     = {   4,   4,   8, 255 };
Color palBack     = {  46,  43,  66, 255 };
Color palBackLit  = {  60,  56,  86, 255 };
Color palRock     = {  58,  54,  76, 255 };
Color palRockDeep = {  32,  30,  44, 255 };
Color palRockLit  = { 106, 101, 134, 255 };
Color palLedge    = {  96,  80,  62, 255 };
Color palLedgeLit = { 158, 136, 104, 255 };
Color palVein     = { 176, 122,  62, 255 };
Color palVeinHot  = { 255, 216, 142, 255 };
Color palMoss     = {  62,  98,  70, 255 };
Color palSkin     = { 224, 216, 196, 255 };
Color palSkinDeep = { 150, 143, 128, 255 };
Color palEye      = { 236, 244, 255, 255 };
Color palPupil    = {  22,  20,  32, 255 };
Color palDrop     = { 130, 168, 196, 255 };
Color palBulb     = { 156, 104, 148, 255 };   // something grown, not cut
Color palBulbLit  = { 224, 178, 214, 255 };
Color palBulbDeep = {  92,  58,  90, 255 };

// Not an overlay: scanline depth is modulated per pixel by luminosity, so a bright
// pixel blooms across the gap and a dark one sinks into it. X stays bilinear, Y is
// snapped to source pixel centres. The dither at the end is there to break up the
// banding that 8-bit colour puts across a large flat dark field.
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
    SetTextureFilter(screenRT.texture, TEXTURE_FILTER_BILINEAR);
    post = LoadShaderFromMemory(0, FS);
    uTexSize = GetShaderLocation(post, "uTexSize");
}

void RenderBegin(void) {
    BeginTextureMode(screenRT);
    ClearBackground(palVoid);
}

void RenderPresent(void) {
    EndTextureMode();
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    int s = sw / GW, sy = sh / GH;
    if (sy < s) s = sy;
    if (s < 1) s = 1;
    int ox = (sw - GW * s) / 2, oy = (sh - GH * s) / 2;
    float ts[2] = { (float)GW, (float)GH };

    BeginDrawing();
        ClearBackground(BLACK);
        SetShaderValue(post, uTexSize, ts, SHADER_UNIFORM_VEC2);
        BeginShaderMode(post);
            DrawTexturePro(screenRT.texture,
                (Rectangle){ 0, 0, (float)GW, -(float)GH },
                (Rectangle){ (float)ox, (float)oy, (float)(GW * s), (float)(GH * s) },
                (Vector2){ 0, 0 }, 0.0f, WHITE);
        EndShaderMode();
    EndDrawing();
}
