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

// The city is seen through the great window (backdrop.c cuts it). It is drawn in the frame's
// own coordinates, a little behind the room: when the view slides, the walls cross the whole
// frame and the city a third of it, which is most of what says it is far away.
#define PARALLAX 0.3f
static int OX, OY;          // frame px of the city's origin this frame

static void Dot(int x, int y, int pl) { DrawRectangle(OX + x, OY + y, 1, 1, PAL[pl]); }

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

// Where a room point shows in the frame, a layer at depth p behind the view: at p = 0 it is
// fixed to the room, at 1 fixed to the frame. The home view is the one the piece is composed
// for: there it sits exactly where the room says.
static void Place(f32 wx, f32 wy, f32 homeX, f32 homeY, f32 p, int *sx, int *sy) {
    f32 cx = homeX + (camX - homeX) * (1.0f - p), cy = homeY + (camY - homeY) * (1.0f - p);
    *sx = (int)lroundf(wx - cx); *sy = (int)lroundf(ROOM_Y + wy - cy);
}

static void Far(void);
static void Beyond(void);
void CityDraw(void) { Far(); Beyond(); }

static void Far(void) {
    int wx0, wx1;
    if (!WindowSpan(15 * TS, &wx0, &wx1) || camX + GW < wx0 - 64 || camX > wx1 + 64) return;
    // composed for the view of the great window's screen; the city's (176, 78) -- where its
    // avenues meet the horizon -- is on the window's centre line, at the sill
    Place((wx0 + wx1) * 0.5f - 176.0f, 13 * TS - 78.0f, 2 * SW * TS, 0, PARALLAX, &OX, &OY);   // the horizon just over the sill
    int X0 = 50, X1 = 296;
    // The glow the city throws up into the air over itself, strongest at the horizon.
    for (int y = HZ - 50; y < HZ + 8; y++)
        for (int x = X0; x < X1; x++) {
            float dy = y < HZ ? (HZ - y) / 50.0f : (y - HZ) / 8.0f;
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
        DrawRectangle(OX + x0, OY + HZ - ht, w, ht, PAL[PL_VOID]);
        for (int y = HZ - ht + 1; y < HZ - 1; y += 3)
            for (int x = x0; x < x0 + w; x += 2) {
                u32 g = Hash2(x * 13, y * 17 + i);
                if (Lit(g, 12)) Dot(x, y, PL_COOLM);
            }
    }
    // The ring: a great circle of lights hung over the city, a pulse running round it.
    {
        float cx = 176.0f, cy = 12.0f, r = 40.0f;          // centred in the window's head
        int n = 120, pulse = (int)(frameNo / 4) % n;
        for (int k = 0; k < n; k++) {
            float a = k * 6.2831853f / n;
            int x = (int)lroundf(cx + r * cosf(a)), y = (int)lroundf(cy + r * 0.94f * sinf(a));
            int d = (k - pulse + n) % n;
            Dot(x, y, d == 0 ? PL_CITYH : (d < 6 ? PL_CITY : ((k % 3) ? PL_CITY : PL_COOLM)));
            if ((k & 1) == 0) {
                int xi = (int)lroundf(cx + (r - 4) * cosf(a)), yi = (int)lroundf(cy + (r - 4) * 0.94f * sinf(a));
                Dot(xi, yi, PL_COOLD);
            }
        }
        for (int t = 0; t < 3; t++) {                              // it hangs from threads of light
            float a = 1.75f + t * 0.42f;
            int xs = (int)(cx + r * cosf(a)), ys = (int)(cy + r * 0.94f * sinf(a));
            for (int y = ys + 2; y < HZ - 40; y += 2) Dot(xs, y, PL_COOLD);
        }
    }
    // Nearer towers: black against the glow, lit down one edge by it, windows in columns.
    static const int NX[7] = { 78, 106, 136, 168, 199, 243, 270 };
    static const int NH[7] = { 42, 70, 50, 92, 62, 48, 34 };
    static const int NW[7] = { 11, 9, 14, 9, 12, 10, 8 };
    for (int i = 0; i < 7; i++) {
        int x0 = NX[i], w = NW[i], top = HZ + 8 - NH[i], bot = HZ + 16;
        DrawRectangle(OX + x0, OY + top, w, bot - top, PAL[PL_VOID]);
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

// Beyond the fireguard: the city's near halls, lit green from below, and the tall ones in
// them -- standing, still, a long way in. Only ever silhouettes (LORE.md section 6).
static void Beyond(void) {
    int gx0, gx1, gy = 0;
    for (int y = 20 * TS; y < 44 * TS; y++) if (GrilleSpan(y, &gx0, &gx1)) { gy = y; break; }
    if (!gy || camX + GW < gx0 - 32 || camX > gx1 + 32 || camY + GH < gy - 32) return;
    int w = gx1 - gx0, h = 44 * TS - gy;
    Place((f32)gx0, (f32)gy, 2 * SW * TS, SH * TS, 0.45f, &OX, &OY);
    // the light, strongest low and in the middle: it comes from further in, under the water.
    // Bring a lamp near and they put it out.
    f32 lit = 1.0f - HallDouse();
    for (int y = -8; y < h + 8; y++)
        for (int x = -12; x < w + 12; x++) {
            f32 dy = (f32)y / h, dx = fabsf((x - w * 0.5f) / (w * 0.6f));
            f32 a = (0.25f + 0.75f * dy) * (1.0f - dx * dx) * lit;
            if (a > 0) Glow(x, y, a * 1.1f);
        }
    // the far wall of that hall: a colonnade, black against the glow
    for (int k = 0; k < 6; k++) {
        int x = -6 + k * (w + 12) / 5;
        DrawRectangle(OX + x, OY - 8, 5, h + 16, PAL[PL_DEEP]);
    }
    // the tall ones: three, at different depths, the nearest largest; heads long, shoulders
    // narrow, arms down. One of them is always a step nearer than you remember.
    static const int TX[3] = { 30, 68, 98 }, TH[3] = { 70, 92, 60 };
    for (int i = 0; i < 3; i++) {
        int x = TX[i] * w / 120, H = TH[i] + (i == 0 ? HallNearer() * 8 : 0), foot = h - 6 - i * 3 + (i == 0 ? HallNearer() * 3 : 0), top = foot - H;
        int hw = 3 + H / 30;
        DrawRectangle(OX + x - hw, OY + top + H / 5, hw * 2, H - H / 5, PAL[PL_VOID]);            // body
        DrawRectangle(OX + x - hw - 1, OY + top + H / 5 + 2, 1, H / 2, PAL[PL_VOID]);              // arm
        DrawRectangle(OX + x + hw, OY + top + H / 5 + 2, 1, H / 2, PAL[PL_VOID]);
        DrawRectangle(OX + x - hw / 2 - 1, OY + top + H / 5 - 3, hw + 2, 4, PAL[PL_VOID]);         // neck
        for (int r = 0; r < H / 5 + 2; r++) {                                                         // the long head, tipped forward
            int hwid = (int)(hw * 0.9f * sinf(3.1416f * (r + 0.5f) / (H / 5 + 2))) + 1;
            DrawRectangle(OX + x - hwid - r / 4, OY + top - 2 + r, hwid * 2, 1, PAL[PL_VOID]);
        }
        DrawRectangle(OX + x + hw, OY + top + H / 5, 1, H - H / 5, PAL[PL_COOLD]);                // the glow on one edge
    }
}
