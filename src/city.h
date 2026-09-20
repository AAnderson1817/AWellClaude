#ifndef AW_CITY_H
#define AW_CITY_H
#include "aw.h"

#define CITY_WINDOW_MAX 3
#define CITY_FISH_COUNT 8
// Detached presentation data, in room pixels (y down, no ROOM_Y). No getter
// advances time or consumes randomness. No city response is used by a rule.
typedef struct {
    float x, y, w, h, light, silhouette;
    int dark, returnFrames, crossing, facing;
} CityWindow;
typedef struct { float x, y, w, h, visibility; } CityMural;
typedef struct {
    float x, y, w, h, eyeX[2], eyeY, pulse, acknowledgment;
    int acknowledgmentFrames, responses;
} CityFace;
typedef struct { float x, y, vx, vy, scatter; int facing, gathering; } CityFish;

void CityInit(void);
void CityStep(void);
void CityLight(void);
void CityDrawBack(void);
void CityDrawFront(void);
void CityDrawGlow(void);
void CityPrintStats(void);
int CityWindowViews(CityWindow *out, int max);
int CityFishViews(CityFish *out, int max);
int CityMuralView(CityMural *out);
int CityFaceView(CityFace *out);
#endif
