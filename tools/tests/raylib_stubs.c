#include "raylib.h"
#include <stdio.h>
#include <stdarg.h>
float testFrameTime = 1.0f/60.0f;
int testJumpDown;
bool IsKeyDown(int key) { return key == KEY_Z && testJumpDown; }
bool IsKeyPressed(int key) { return false; }
float GetFrameTime(void) { return testFrameTime; }
void InitWindow(int w,int h,const char *s) {}
void CloseWindow(void) {}
bool WindowShouldClose(void) { return false; }
void SetTraceLogLevel(int l) {}
void SetConfigFlags(unsigned int f) {}
void SetTargetFPS(int n) {}
void TraceLog(int level,const char *text,...) { va_list a;va_start(a,text);vfprintf(stderr,text,a);fputc('\n',stderr);va_end(a); }
void DrawRectangle(int x,int y,int w,int h,Color c) {}
void DrawTexturePro(Texture2D t,Rectangle a,Rectangle b,Vector2 o,float r,Color c) {}
void BeginBlendMode(int m) {}
void EndBlendMode(void) {}
void BeginDrawing(void) {}
void EndDrawing(void) {}
void BeginShaderMode(Shader s) {}
void EndShaderMode(void) {}
void BeginTextureMode(RenderTexture2D t) {}
void EndTextureMode(void) {}
void ClearBackground(Color c) {}
int GetScreenHeight(void) { return 720; }
int GetScreenWidth(void) { return 1280; }
int GetShaderLocation(Shader s,const char *n) { return 0; }
RenderTexture2D LoadRenderTexture(int w,int h) { return (RenderTexture2D){0}; }
Shader LoadShaderFromMemory(const char *v,const char *f) { return (Shader){0}; }
void SetShaderValue(Shader s,int l,const void *v,int t) {}
void SetTextureFilter(Texture2D t,int f) {}
void SetTextureWrap(Texture2D t,int w) {}
void UpdateTexture(Texture2D t,const void *p) {}
Image GenImageColor(int w,int h,Color c) { return (Image){0}; }
Texture2D LoadTextureFromImage(Image i) { return (Texture2D){0}; }
Image LoadImageFromScreen(void) { return (Image){0}; }
void UnloadImage(Image i) {}
bool ExportImage(Image i,const char *p) { return false; }
