#ifndef AW_DEPTH_H
#define AW_DEPTH_H
#include "raylib.h"
extern int depthEnabled;
extern int depthStill;
#if defined(AWELL_HEADLESS)
static inline void DepthDraw(void) {}
static inline void DepthUnload(void) {}
#else
void DepthDraw(void);
void DepthUnload(void);
#endif
Texture2D RoomLightTexture(void);
void RoomDepthLightColors(Color *out);
float RoomWaterHeight(int column);
#endif
