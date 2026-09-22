// city.c -- the far city, seen through a break in the vault's back wall.
//
// Drawn only in the new look. Everything here is self-lit: it is not touched by the light
// pass, because it is far away and it is theirs. It is points and lines of their colour on
// black -- the city is read by its lights, never by its stone -- in planes: a plain of
// lights below the horizon compressing toward it, far towers, nearer towers standing black
// against the far ones, a great ring hung over the city, and one light that climbs a spire
// and never stops climbing. It does not notice you (L9).
#include "aw.h"
#include <math.h>

// Which room has the view. None, for now: the user found the break in the Vault Mouth's
// wall unreadable -- a cut-out with platforms floating in it, in a room not built around a
// view -- and it muddied the look test, which is about the renderer. It comes back only in a
// room composed around it, with the climb kept to the edges.
#define CITY_ROOM (-1)

// The break in the wall, room 0 only: a ragged ellipse over the open middle of the room.
#define BR_CX 172.0f
#define BR_CY  56.0f
#define BR_RX 108.0f
#define BR_RY  46.0f

// Broken stone, not a cut: blocks of a few rows knocked back by different amounts, on top
// of a slow wander, so the edge reads as masonry that fell rather than a shape.
static float Ragged(int y, int side) {
    u32 h = Hash2(y / 4 + side * 101, 7 + side), g = Hash2(y / 9 + side * 37, 3);
    return (float)(h % 9) - 4.0f + (float)(g % 13) - 6.0f
         + 7.0f * sinf(y * (side ? 0.071f : 0.093f) + side * 2.0f);
}
// The span of room row y that is open to the far city, or 0.
int CityBreachSpan(int y, int *x0, int *x1) {
    if (roomIdx != CITY_ROOM) return 0;
    float dy = (y - BR_CY) / BR_RY;
    if (dy <= -1.0f || dy >= 1.0f) return 0;
    float hw = BR_RX * powf(1.0f - dy * dy, 0.38f) * (1.0f + 0.05f * sinf(y * 0.11f));
    if (hw < 3.0f) return 0;
    *x0 = (int)(BR_CX - hw + Ragged(y, 0));
    *x1 = (int)(BR_CX + hw + Ragged(y, 1));
    return *x1 > *x0;
}

// Cut the break out of the back wall that was just drawn: subtract blend of transparent
// black writes zeros, and alpha zero is how the composite knows to show the city there.
void CityErase(void) {
    if (roomIdx != CITY_ROOM) return;
    BeginBlendMode(BLEND_SUBTRACT_COLORS);
    for (int y = 0; y < RH * TS; y++) {
        int x0, x1;
        if (CityBreachSpan(y, &x0, &x1)) DrawRectangle(x0, ROOM_Y + y, x1 - x0, 1, (Color){ 0, 0, 0, 0 });
    }
    EndBlendMode();
    // the broken edge of the wall: a lip of lighter stone, chipped
    for (int y = 0; y < RH * TS; y++) {
        int x0, x1;
        if (!CityBreachSpan(y, &x0, &x1)) continue;
        if (Hash2(x0, y) & 1) DrawRectangle(x0 - 1, ROOM_Y + y, 1, 1, palBackLit);
        if (Hash2(x1, y) & 1) DrawRectangle(x1, ROOM_Y + y, 1, 1, palBackLit);
    }
}

static void Dot(int x, int y, int pl) { DrawRectangle(x, ROOM_Y + y, 1, 1, PAL[pl]); }

// A window's light, hashed: whether it is lit now. A few change their minds, slowly.
static int Lit(u32 h, int pct) {
    int lit = (int)(h % 100) < pct;
    if (((frameNo / 150) + (h >> 8) % 211) % 211 == 0) lit = !lit;
    return lit;
}

#define HZ 78      // the horizon, in room px
#define VX 176     // where the avenues meet it

// A dithered glow, in bands: a is 0..1, above a floor.
static void Glow(int x, int y, float a) {
    float t = (float)((Hash2(x, y * 3) & 15) + 0.5f) / 16.0f - 0.5f;
    float q = a * 3.0f + t * 0.8f;
    if (q > 2.0f) Dot(x, y, PL_COOLM);
    else if (q > 1.0f) Dot(x, y, PL_COOLD);
    else if (q > 0.55f && ((x + y) & 1)) Dot(x, y, PL_DEEP);
}

void CityDraw(void) {
    if (roomIdx != CITY_ROOM) return;
    int X0 = 50, X1 = 296;
    // The glow the city throws up into the air over itself, strongest at the horizon.
    for (int y = HZ - 38; y < HZ + 8; y++)
        for (int x = X0; x < X1; x++) {
            float dy = y < HZ ? (HZ - y) / 38.0f : (y - HZ) / 8.0f;
            float dx = fabsf((float)(x - VX)) / 140.0f;
            float a = (1.0f - dy) * (1.0f - dx * dx * 0.7f);
            if (a > 0) Glow(x, y, a * 0.9f);
        }
    // The plain below the horizon, in perspective: avenues of lights running out from where
    // they meet the horizon, and cross streets closer together the further off they are.
    for (int k = 0; k < 13; k++) {                               // cross streets
        int y = HZ + 3 + (int)(k * k * 0.30f + k * 1.4f);
        if (y > 116) break;
        float spread = (y - HZ) * 3.2f;
        int step = 2 + k / 4;
        for (int x = VX - (int)spread; x < VX + (int)spread; x += step) {
            if (x < X0 || x >= X1) continue;
            u32 h = Hash2(x * 3 + k, y * 7);
            if (!Lit(h, 62 - k * 2)) continue;
            Dot(x, y, (h >> 12) % 100 < 10 + k ? PL_CITY : PL_COOLM);
        }
    }
    for (int a = -7; a <= 7; a++) {                              // avenues
        float slope = a * 0.62f;
        for (int k = 1; k < 60; k++) {
            float y = HZ + 1 + k * k * 0.034f + k * 0.35f;
            if (y > 116) break;
            int x = VX + (int)lroundf((y - HZ) * slope);
            if (x < X0 || x >= X1) continue;
            u32 h = Hash2(a + 50, k);
            if ((h % 100) < 78) Dot(x, (int)y, (k % 7 == 3) ? PL_CITYH : PL_CITY);
        }
    }
    // Where the far city runs together into one line.
    for (int x = X0; x < X1; x++) {
        u32 h = Hash2(x, 991);
        float dx = fabsf((float)(x - VX)) / 140.0f;
        if ((h % 100) < (u32)(80 - dx * 40)) Dot(x, HZ, (h >> 9) % 100 < 30 ? PL_CITYH : PL_CITY);
    }
    // Far towers on the horizon: taller toward the middle, faint windows every 2 px.
    for (int i = 0; i < 48; i++) {
        u32 h = Hash2(i, 55);
        int x0 = X0 + 2 + i * 5 + (int)(h % 3), w = 2 + (int)((h >> 3) % 4);
        float mid = 1.0f - fabsf((x0 - (float)VX) / 130.0f);
        int ht = 3 + (int)((h >> 7) % 10) + (int)(mid * mid * 26.0f);
        DrawRectangle(x0, ROOM_Y + HZ - ht, w, ht, PAL[PL_VOID]);
        for (int y = HZ - ht + 1; y < HZ - 1; y += 3)
            for (int x = x0; x < x0 + w; x += 2) {
                u32 g = Hash2(x * 13, y * 17 + i);
                if (Lit(g, 12)) Dot(x, y, PL_COOLM);
            }
    }
    // The ring: a great circle of lights hung over the city, a pulse running round it.
    {
        float cx = 216.0f, cy = 32.0f, r = 25.0f;
        int n = 78, pulse = (int)(frameNo / 5) % n;
        for (int k = 0; k < n; k++) {
            float a = k * 6.2831853f / n;
            int x = (int)lroundf(cx + r * cosf(a)), y = (int)lroundf(cy + r * 0.94f * sinf(a));
            int d = (k - pulse + n) % n;
            Dot(x, y, d == 0 ? PL_CITYH : (d < 4 ? PL_CITY : ((k % 3) ? PL_COOLM : PL_CITY)));
            if ((k & 1) == 0) {
                int xi = (int)lroundf(cx + (r - 4) * cosf(a)), yi = (int)lroundf(cy + (r - 4) * 0.94f * sinf(a));
                Dot(xi, yi, PL_COOLD);
            }
        }
        for (int t = 0; t < 3; t++) {                              // it hangs from threads of light
            float a = 1.75f + t * 0.42f;
            int xs = (int)(cx + r * cosf(a)), ys = (int)(cy + r * 0.94f * sinf(a));
            for (int y = ys + 2; y < HZ - 22; y += 2) Dot(xs, y, PL_COOLD);
        }
    }
    // Nearer towers: black against the glow, lit down one edge by it, windows in columns.
    static const int NX[7] = { 78, 106, 136, 168, 199, 243, 270 };
    static const int NH[7] = { 30, 50, 36, 66, 44, 34, 24 };
    static const int NW[7] = { 11, 9, 14, 9, 12, 10, 8 };
    for (int i = 0; i < 7; i++) {
        int x0 = NX[i], w = NW[i], top = HZ + 8 - NH[i], bot = HZ + 16;
        DrawRectangle(x0, ROOM_Y + top, w, bot - top, PAL[PL_VOID]);
        int lit = x0 + w / 2 < VX ? x0 + w - 1 : x0;                 // the edge that faces the glow
        for (int y = top; y < bot; y++) if (y > HZ - 22 && ((y & 1) || y > HZ - 8)) Dot(lit, y, y > HZ - 8 ? PL_COOLM : PL_COOLD);
        for (int x = x0 + 2; x < x0 + w - 2; x += 3)
            for (int y = top + 3; y < bot - 2; y += 3) {
                u32 g = Hash2(x * 7 + i, y * 11);
                if (Lit(g, 30)) Dot(x, y, (g >> 11) % 100 < 14 ? PL_CITYH : PL_CITY);
            }
        int sx = x0 + w / 2, sh = 6 + (int)(Hash2(i, 9) % 10);
        for (int y = top - sh; y < top; y++) Dot(sx, y, PL_COOLD);
        Dot(sx, top - sh - 1, PL_CITYH);
    }
    // One light climbs the tallest spire, all the way up, for twenty seconds, then again.
    {
        int x0 = NX[3] + NW[3] / 2, top = HZ + 8 - NH[3] - 16, bot = HZ + 10;
        float t = (float)(frameNo % 1200) / 1200.0f;
        int y = bot - (int)(t * (bot - top));
        Dot(x0, y, PL_CITYH); Dot(x0, y + 1, PL_CITY);
    }
}
