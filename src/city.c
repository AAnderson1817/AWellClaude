// city.c -- what is beyond the far wall's openings: the rest of the archive, a chamber
// miles across, seen past the keeper through the great opening and through the window.
//
// It is a multiplane (tools/art/vista.py paints it): five layers, each a picture at its own
// distance, far to near -- the haze and the pillar of light where the chamber's heart is;
// the constructs on the horizon; the city on the plain; the temples of the middle distance;
// the near towers and the bridge where the tall ones stand. Each layer follows the camera
// by its depth, so when the view slides the near ones cross the frame and the far ones
// hardly move: that is most of what says how far it goes.
//
// Drawn only in the new look, into the back layer, which shows wherever the far wall's
// picture is cut away. It is not touched by the light pass: it is far off and in its own
// light. The lights that answer -- that change their minds now and then, or go out when a
// lamp comes to the edge -- are not in the pictures; they are listed, and drawn here. It
// does not notice you, except to put its lights out (L9).
#include "aw.h"
#include <math.h>
#include <stdlib.h>

extern const unsigned char ART_VISTA_0[], ART_VISTA_1[], ART_VISTA_2[], ART_VISTA_3[], ART_VISTA_4[];
extern const int ART_VISTA_0_LEN, ART_VISTA_1_LEN, ART_VISTA_2_LEN, ART_VISTA_3_LEN, ART_VISTA_4_LEN;
extern const i16 INTS_VISTA[];           // vista.py's table: a header, the layers, the lights; five a row
extern const int INTS_VISTA_LEN;

#define LAYERS 5
enum { V_TWINKLE = 1, V_DOUSE = 2 };
static const unsigned char *const ART[LAYERS] = { ART_VISTA_0, ART_VISTA_1, ART_VISTA_2, ART_VISTA_3, ART_VISTA_4 };
static const int *const ARTN[LAYERS] = { &ART_VISTA_0_LEN, &ART_VISTA_1_LEN, &ART_VISTA_2_LEN, &ART_VISTA_3_LEN, &ART_VISTA_4_LEN };
static Texture2D tex[LAYERS];
static int loaded;

static void Load(void) {
    loaded = 1;
    for (int i = 0; i < LAYERS; i++) {
        Image im = LoadImageFromMemory(".png", ART[i], *ARTN[i]);
        if (!im.data) continue;
        tex[i] = LoadTextureFromImage(im);
        UnloadImage(im);
    }
}

// A light's state, hashed: whether it is lit now. Now and then one changes its mind, slowly
// -- the archive at its cataloguing, far off, taking no notice.
static int Lit(u32 h) {
    return ((frameNo / 150) + (h >> 8) % 211) % 211 != 0;
}

static int OX, OY;                      // frame px of the current layer's room origin
static void Dot(int x, int y, int pl) { DrawRectangle(OX + x, OY + y, 1, 1, PAL[pl]); }
static void Rect(int x, int y, int w, int h, int pl) { DrawRectangle(OX + x, OY + y, w, h, PAL[pl]); }

// The layer's own life, drawn over its picture.
static void Live(int layer, const i16 *L, int nl) {
    f32 out = HallDouse();
    for (int k = 0; k < nl; k++) {
        const i16 *r = L + k * 5;
        if (r[0] != layer) continue;
        u32 h = Hash2(r[1] * 7 + r[2], r[2] * 13 + layer);
        if ((r[4] & V_TWINKLE) && !Lit(h)) continue;
        if ((r[4] & V_DOUSE) && out > (f32)(h % 1000) / 1000.0f * 0.9f + 0.05f) continue;   // out one by one
        Dot(r[1], r[2], r[3]);
    }
    if (layer == 0) {
        // one light climbs the pillar, from the horizon out of sight, and again
        int t = (int)(frameNo % 1500), y = 170 - t / 6;
        if (y > -40) { Dot(590, y, PL_CITYH); Dot(590, y + 1, PL_CITY); Dot(590, y + 2, PL_COOLM); }
    }
    if (layer == 2) {
        // the reading, far off: a pulse runs up one of the avenues to the pillar's foot
        int t = (int)(frameNo % 1100);
        if (t < 520) {
            f32 u = 1.0f - t / 520.0f, sx = 3 + u * 120.0f;
            Dot((int)lroundf(590 + 3 * sx * 1.7f), (int)lroundf(184 + sx * 0.85f), PL_CITYH);
        }
    }
    if (layer == 3) {
        // lanterns, let go somewhere below, rising slowly through the chamber until they
        // are too small to see
        for (int i = 0; i < 7; i++) {
            u32 h = Hash2(i, 77);
            int period = 900 + (int)(h % 500), t = (int)((frameNo + h % period) % period);
            f32 y = 300.0f - t * 0.3f;
            if (y < 40.0f) continue;
            int x = 330 + (int)(h % 440) + (int)lroundf(sinf(t * 0.013f + i) * 3.0f);
            int pl = (i % 3 == 0) ? PL_AMBER : ((i % 3 == 1) ? PL_ROSEL : PL_CITY);
            Dot(x, (int)y, y < 90.0f && (t & 8) ? PL_COOLD : pl);
        }
    }
    if (layer == 3 && out < 0.5f) {
        // a procession on the bridge, lanterns carried slowly across, one way
        for (int i = 0; i < 7; i++) {
            int x = 430 + (int)((frameNo / 5 + i * 27) % 190);
            if ((i * 37 + (int)(frameNo / 90)) % 5 == 0) continue;
            Dot(x, 230, PL_AMBER); Dot(x, 231, PL_WARM);
        }
    }
    if (layer == 4) {
        // the tall ones, on the near bridge: heads long, shoulders narrow, arms down. One of
        // them is always a step nearer than you remember.
        static const int TX[3] = { 356, 392, 436 }, TH[3] = { 36, 42, 30 };
        for (int i = 0; i < 3; i++) {
            int H = TH[i] + (i == 1 ? HallNearer() * 4 : 0), x = TX[i] + (i == 1 ? HallNearer() * 3 : 0);
            int foot = 262, top = foot - H, hw = 2 + H / 20;
            Rect(x - hw, top + H / 5, hw * 2, H - H / 5, PL_VOID);                // body
            Rect(x - hw - 1, top + H / 5 + 2, 1, H / 2, PL_VOID);                  // arms
            Rect(x + hw, top + H / 5 + 2, 1, H / 2, PL_VOID);
            Rect(x - hw / 2 - 1, top + H / 5 - 3, hw + 2, 4, PL_VOID);             // neck
            for (int r = 0; r < H / 5 + 2; r++) {                                   // the long head, tipped forward
                int hwid = (int)(hw * 0.9f * sinf(3.1416f * (r + 0.5f) / (H / 5 + 2))) + 1;
                Rect(x - hwid - r / 4, top - 2 + r, hwid * 2, 1, PL_VOID);
            }
            Rect(x + hw, top + H / 5, 1, H - H / 5, out < 0.5f ? PL_COOLD : PL_DEEP);   // the light on one edge
        }
    }
}

void CityDraw(void) {
    if (!loaded) Load();
    const i16 *T = INTS_VISTA;
    int nl = T[0], nlights = T[1];
    f32 hx = T[2], hy = T[3];
    const i16 *lights = T + 5 * (1 + nl);
    for (int i = 0; i < nl && i < LAYERS; i++) {
        const i16 *r = T + 5 * (1 + i);
        f32 p = r[0] / 1000.0f;
        // a point of this layer at room (x, y) in the home view shows, with the camera
        // elsewhere, at room (x, y) plus the camera's move times p
        OX = (int)lroundf((camX - hx) * p - camX);
        OY = (int)lroundf(ROOM_Y + (camY - hy) * p - camY);
        if (tex[i].id) DrawTexture(tex[i], OX + r[1], OY + r[2], WHITE);
        Live(i, lights, nlights);
    }
}
