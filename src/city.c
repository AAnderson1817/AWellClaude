// city.c -- what is beyond the far wall's two openings: through the window's cell, more of
// the archive; through the heart's grate, the tall ones.
//
// Drawn only in the new look. Everything here is self-lit: it is not touched by the light
// pass, because it is far away and it is theirs. It is points and lines of their colour on
// black -- the archive far off is read by its lights, never by its stone. It does not
// notice you (L9).
#include "aw.h"
#include <math.h>
#include <stdlib.h>

// Each is drawn in the frame's own coordinates, a little behind the room: when the view
// slides, the walls cross the whole frame and what is beyond only part of it, which is most
// of what says it is far away.
#define PARALLAX 0.3f
static int OX, OY;          // frame px of the piece's origin this frame
static f32 clipX, clipY, clipR;       // the opening it is seen through, room px; r 0: none

static void Dot(int x, int y, int pl) {
    if (clipR > 0) {
        f32 dx = camX + OX + x + 0.5f - clipX, dy = camY + OY + y - ROOM_Y + 0.5f - clipY;
        if (dx * dx + dy * dy > clipR * clipR) return;
    }
    DrawRectangle(OX + x, OY + y, 1, 1, PAL[pl]);
}

// A light's state, hashed: whether it is lit now. Now and then one changes its mind, slowly
// -- the archive at its cataloguing, far off, taking no notice.
static int Lit(u32 h, int pct) {
    int lit = (int)(h % 100) < pct;
    if (((frameNo / 150) + (h >> 8) % 211) % 211 == 0) lit = !lit;
    return lit;
}

static void Rect(int x, int y, int w, int h, int pl) {
    for (int j = y; j < y + h; j++) for (int i = x; i < x + w; i++) Dot(i, j, pl);
}

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

// Through the window: the archive goes on. A hall of stacks, seen down its length from high
// in its end wall -- its ribs arching over it bay after bay, each a ring of their glass
// smaller than the last; on its walls and up into its vault the kept things in their
// courses, lit as they are kept; a vein down the middle of its floor to where it ends, at
// another cell, awake. The sitters on the sill face it.
#define VY 14               // the vanishing point, below the window's centre: the sill's eye
#define HALF 150.0f         // the hall's half-width, its floor under the eye and its arches' spring
#define FLOORY 60.0f
#define SPRING (-40.0f)
static void Far(void) {
    int bx, by, bw, bh;
    if (!OpeningBox(F_WINDOW, &bx, &by, &bw, &bh)) return;
    if (camX + GW < bx - 8 || camX > bx + bw + 8 || camY + GH < by - 8 || camY > by + bh + 8) return;
    clipR = bw * 0.5f; clipX = bx + clipR; clipY = by + clipR;
    Place(clipX, clipY, 2 * SW * TS, 0, PARALLAX, &OX, &OY);          // local (0, 0) is the window's centre
    // the light the far end throws back up the hall
    for (int y = -60; y < 60; y++)
        for (int x = -70; x < 70; x++) {
            f32 dx = x / 70.0f, dy = (y - VY + 8) / 50.0f, a = 0.55f - (dx * dx + dy * dy) * 0.9f;
            if (a > 0) Glow(x, y, a);
        }
    // the bays, far to near: an arch at each, the stacks between
    static const f32 Z[] = { 1.6f, 2.1f, 2.7f, 3.4f, 4.3f, 5.4f, 6.8f, 8.6f, 11.0f, 14.0f, 18.0f };
    const int NZ = (int)(sizeof Z / sizeof Z[0]);
    for (int k = NZ - 2; k >= 0; k--) {
        f32 z0 = Z[k], z1 = Z[k + 1];
        int far = k > 5;
        // the walls' courses and the vault's, from this arch to the next
        for (f32 z = z0 + 0.02f; z < z1; z += 2.0f * z * z / HALF) {
            f32 s = 1.0f / z;
            int zi = (int)(z * 97.0f);
            for (int side = -1; side <= 1; side += 2) {
                for (int j = 0; j <= 12; j++) {              // up the wall, a course every eight
                    f32 h = FLOORY - 8.0f * j;
                    u32 hh = Hash2(zi * 2 + (side > 0), j * 31 + k);
                    if (!Lit(hh, far ? 52 : 64)) continue;
                    int pl = (hh >> 12) % 100 < 9 ? PL_CITY : ((hh >> 12) % 100 < 30 && !far ? PL_COOLM : PL_COOLD);
                    Dot((int)lroundf(side * HALF * s), (int)lroundf(VY + h * s), pl);
                }
            }
            for (int j = 1; j < 16; j++) {                    // and over, up the vault
                f32 ph = j * 3.14159f / 16.0f;
                u32 hh = Hash2(zi, j * 53 + k + 7);
                if (!Lit(hh, far ? 40 : 50)) continue;
                int pl = (hh >> 12) % 100 < 7 ? PL_CITY : PL_COOLD;
                Dot((int)lroundf(-HALF * cosf(ph) * s), (int)lroundf(VY + (SPRING - HALF * sinf(ph)) * s), pl);
            }
        }
        // the rib at this arch: up the wall, over, down, glass at every band
        f32 s = 1.0f / z0;
        int n = (int)(HALF * s * 3.3f) + 8;
        for (int i = 0; i <= n; i++) {
            f32 ph = i * 3.14159f / n;
            int x = (int)lroundf(-HALF * cosf(ph) * s), y = (int)lroundf(VY + (SPRING - HALF * sinf(ph)) * s);
            if ((i % 7) == 3) Dot(x, y, PL_CITY);
            else if (i & 1) Dot(x, y, PL_COOLD);
        }
        for (int side = -1; side <= 1; side += 2)
            for (f32 h = FLOORY; h > SPRING; h -= 1.0f / s) {
                int k2 = (int)((FLOORY - h) * s), node = (k2 % 7) == 3;
                if (node || (k2 & 1)) Dot((int)lroundf(side * HALF * s), (int)lroundf(VY + h * s), node ? PL_CITY : PL_COOLD);
            }
    }
    // the floor: a vein down the middle to the far end, and the reading running along it
    int run = (int)(frameNo % 900);
    for (f32 z = 1.4f; z < 18.0f; z *= 1.045f) {
        f32 s = 1.0f / z;
        int y = (int)lroundf(VY + FLOORY * s);
        int k = (int)((z - 1.4f) * 40.0f);
        int pulse = run < 600 && abs(k - run * 2 / 3) < 3;
        Dot(0, y, pulse ? PL_CITYH : ((k % 5) == 0 ? PL_CITY : PL_COOLM));
        if ((k % 3) == 0) { Dot((int)lroundf(-40.0f * s), y, PL_COOLD); Dot((int)lroundf(40.0f * s), y, PL_COOLD); }
    }
    // the far end: a cell like the door, and awake
    {
        f32 s = 1.0f / 18.0f, r = 70.0f * s * 1.6f;
        int cy = (int)lroundf(VY + (FLOORY - 70.0f) * s);
        for (int i = 0; i < 40; i++) {
            f32 a = i * 6.2832f / 40;
            Dot((int)lroundf(r * cosf(a)), cy + (int)lroundf(r * sinf(a)), (i % 4) ? PL_COOLM : PL_CITY);
        }
        Dot(0, cy, PL_CITYH); Dot(1, cy, PL_CITY); Dot(-1, cy, PL_CITY); Dot(0, cy - 1, PL_CITY); Dot(0, cy + 1, PL_CITY);
    }
    clipR = 0;
}

// Beyond the fireguard: the city's near halls, lit green from below, and the tall ones in
// them -- standing, still, a long way in. Only ever silhouettes (LORE.md section 6).
static void Beyond(void) {
    int gx0, gy, w, h;
    if (!OpeningBox(F_GRILLE, &gx0, &gy, &w, &h)) return;
    if (camX + GW < gx0 - 32 || camX > gx0 + w + 32 || camY + GH < gy - 32 || camY > gy + h + 32) return;
    clipR = w * 0.5f + 1; clipX = gx0 + w * 0.5f; clipY = gy + h * 0.5f;
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
        Rect(x, -8, 5, h + 16, PL_DEEP);
    }
    // the tall ones: three, at different depths, the nearest largest; heads long, shoulders
    // narrow, arms down. One of them is always a step nearer than you remember.
    static const int TX[3] = { 30, 68, 98 }, TH[3] = { 44, 58, 38 };
    for (int i = 0; i < 3; i++) {
        int x = TX[i] * w / 120, H = TH[i] + (i == 0 ? HallNearer() * 6 : 0), foot = h - 18 - i * 3 + (i == 0 ? HallNearer() * 3 : 0), top = foot - H;
        int hw = 3 + H / 30;
        Rect(x - hw, top + H / 5, hw * 2, H - H / 5, PL_VOID);            // body
        Rect(x - hw - 1, top + H / 5 + 2, 1, H / 2, PL_VOID);              // arm
        Rect(x + hw, top + H / 5 + 2, 1, H / 2, PL_VOID);
        Rect(x - hw / 2 - 1, top + H / 5 - 3, hw + 2, 4, PL_VOID);         // neck
        for (int r = 0; r < H / 5 + 2; r++) {                                                         // the long head, tipped forward
            int hwid = (int)(hw * 0.9f * sinf(3.1416f * (r + 0.5f) / (H / 5 + 2))) + 1;
            Rect(x - hwid - r / 4, top - 2 + r, hwid * 2, 1, PL_VOID);
        }
        Rect(x + hw, top + H / 5, 1, H - H / 5, PL_COOLD);                // the glow on one edge
    }
    clipR = 0;
}
