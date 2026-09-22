// backdrop.c -- the hall's big pieces: the colossus, the great window, the fireguard.
//
// They are backdrop: drawn over the far wall and under everything else, and where you can
// stand on one, the map carves a tile under it ('X') and the room leaves the drawing to
// this. The colossus is a picture (art/colossus.png, made by tools/art/colossus.py); the
// window and the fireguard are openings cut in the far wall, and what shows through them is
// city.c's.
#include "aw.h"
#include <math.h>

extern const unsigned char ART_COLOSSUS[];
extern const int ART_COLOSSUS_LEN;

static Texture2D texColossus;

static const Feature *Find(int kind) {
    for (int i = 0; ROOM_FEATURES[i].kind != F_NONE; i++)
        if (ROOM_FEATURES[i].kind == kind) return &ROOM_FEATURES[i];
    return 0;
}

void BackdropInit(void) {
    Image im = LoadImageFromMemory(".png", ART_COLOSSUS, ART_COLOSSUS_LEN);
    texColossus = LoadTextureFromImage(im);
    UnloadImage(im);
}

// Whether a room-px rectangle is anywhere near the view.
static int InView(f32 x, f32 y, f32 w, f32 h) {
    return x + w > camX - 8 && x < camX + GW + 8 && y + h > camY - 8 && y < camY + GH + 8;
}

// ---------------------------------------------------------------- the openings
// The great window: a tall round-headed arch. The fireguard: a lower one, square-headed.
// Both are spans of room px per row, so the cut, the frame and the city agree exactly.
static int ArchSpan(const Feature *f, int round, int y, int *x0, int *x1) {
    f32 X0 = f->x * TS, W = f->w * TS, Y0 = f->y * TS, H = f->h * TS;
    if (y < Y0 || y >= Y0 + H) return 0;
    f32 r = W * 0.5f, cx = X0 + r, hw = r;
    if (round && y < Y0 + r) {                   // the head of the arch: a half circle
        f32 dy = (Y0 + r) - (y + 0.5f);
        hw = sqrtf(fmaxf(r * r - dy * dy, 0.0f));
    }
    if (hw < 1.0f) return 0;
    *x0 = (int)lroundf(cx - hw); *x1 = (int)lroundf(cx + hw);
    return *x1 > *x0;
}
int WindowSpan(int y, int *x0, int *x1) { const Feature *f = Find(F_WINDOW); return f && ArchSpan(f, 1, y, x0, x1); }
int GrilleSpan(int y, int *x0, int *x1) { const Feature *f = Find(F_GRILLE); return f && ArchSpan(f, 0, y, x0, x1); }

// Cut the openings out of the far wall just drawn: subtract-blend transparent black writes
// zero alpha, and alpha zero is how the composite knows to show what is beyond.
static void CutOpenings(void) {
    BeginBlendMode(BLEND_SUBTRACT_COLORS);
    for (int pass = 0; pass < 2; pass++) {
        const Feature *f = Find(pass ? F_GRILLE : F_WINDOW);
        if (!f || !InView(f->x * TS, f->y * TS, f->w * TS, f->h * TS)) continue;
        for (int y = f->y * TS; y < (f->y + f->h) * TS; y++) {
            int x0, x1;
            if (y < camY - 4 || y > camY + GH + 4) continue;
            if (ArchSpan(f, !pass, y, &x0, &x1)) DrawRectangle(x0, ROOM_Y + y, x1 - x0, 1, (Color){ 0, 0, 0, 0 });
        }
    }
    EndBlendMode();
}

// The window's frame and tracery, in dressed stone: a moulded edge round the arch, two
// mullions, and a ring in the head. Stone, so the city behind rims every edge of it.
static void Stone(int x, int y, int w, int h, int pl) {
    Color c = PAL[pl]; c.a = TAG_STONE;
    DrawRectangle(x, ROOM_Y + y, w, h, c);
}
static void WindowFrame(void) {
    const Feature *f = Find(F_WINDOW);
    if (!f || !InView(f->x * TS, f->y * TS, f->w * TS, f->h * TS)) return;
    int X0 = f->x * TS, W = f->w * TS, Y0 = f->y * TS, H = f->h * TS;
    f32 r = W * 0.5f, cx = X0 + r, cy = Y0 + r;
    // the moulding: two px of lit stone just inside the opening, all the way round
    for (int y = Y0; y < Y0 + H; y++) {
        int x0, x1;
        if (!ArchSpan(f, 1, y, &x0, &x1)) continue;
        Stone(x0, y, 2, 1, PL_STONEL); Stone(x1 - 2, y, 2, 1, PL_STONE);
    }
    for (int x = X0; x < X0 + W; x++) {
        f32 dx = x + 0.5f - cx;
        if (fabsf(dx) >= r - 1) continue;
        int y = (int)(cy - sqrtf(r * r - dx * dx));
        Stone(x, y, 1, 2, PL_STONEL);
    }
    // two mullions, the full height of the lights, and a transom where the head begins
    int m1 = X0 + W / 3, m2 = X0 + 2 * W / 3;
    Stone(m1 - 1, (int)cy - 20, 3, Y0 + H - ((int)cy - 20), PL_STONE);
    Stone(m2 - 1, (int)cy - 20, 3, Y0 + H - ((int)cy - 20), PL_STONE);
    Stone(X0 + 2, (int)cy - 2, W - 4, 3, PL_STONE);
    // the ring in the head, and its spokes
    f32 rr = r * 0.42f, ry = cy - r * 0.46f;
    for (int k = 0; k < 360; k++) {
        f32 a = k * 0.0174533f;
        Stone((int)lroundf(cx + rr * cosf(a)), (int)lroundf(ry + rr * sinf(a)), 2, 2, PL_STONE);
    }
    for (int k = 0; k < 6; k++) {
        f32 a = k * 1.0472f + 0.5236f;
        for (f32 t = 5; t < rr; t += 1.0f) Stone((int)lroundf(cx + t * cosf(a)), (int)lroundf(ry + t * sinf(a)), 1, 1, PL_STONE);
    }
    Stone((int)cx - 3, (int)ry - 3, 6, 6, PL_STONEL);
}

// The fireguard: iron bars close together, bands across them, the foot in the water.
static void GrilleBars(void) {
    const Feature *f = Find(F_GRILLE);
    if (!f || !InView(f->x * TS, f->y * TS, f->w * TS, f->h * TS)) return;
    int X0 = f->x * TS, W = f->w * TS, Y0 = f->y * TS, H = f->h * TS;
    for (int x = X0 + 2; x < X0 + W - 1; x += 5) {
        Stone(x, Y0, 2, H, PL_DARK);
        Stone(x, Y0, 1, H, PL_STONE);
    }
    for (int y = Y0 + 6; y < Y0 + H; y += 36) Stone(X0, y, W, 3, PL_DARK);
    Stone(X0 - 3, Y0 - 4, W + 6, 4, PL_STONE);            // the lintel
    Stone(X0 - 3, Y0 - 5, W + 6, 1, PL_STONEL);
}

// ---------------------------------------------------------------- architecture
// Far wall only: lit as the wall is, never stood on. It is what says the hall was built.
static void Wall(int x, int y, int w, int h, int pl) {
    Color c = PAL[pl]; c.a = TAG_WALL;
    DrawRectangle(x, ROOM_Y + y, w, h, c);
}

// The colossus sits in a niche: a round-headed recess in the far wall, deeper than the wall
// and so darker, coursed in long stones, framed by a moulded arch of voussoirs.
static void Niche(const Feature *f) {
    int X0 = f->x * TS, W = f->w * TS, Y0 = f->y * TS, H = f->h * TS;
    f32 r = W * 0.5f, cx = X0 + r, cy = Y0 + r;
    int vy0 = (int)camY - 4, vy1 = (int)camY + GH + 4;
    for (int y = Y0; y < Y0 + H; y++) {
        if (y < vy0 || y > vy1) continue;
        f32 hw = r;
        if (y < cy) { f32 dy = cy - (y + 0.5f); hw = sqrtf(fmaxf(r * r - dy * dy, 0)); }
        int x0 = (int)lroundf(cx - hw), x1 = (int)lroundf(cx + hw);
        if (x1 <= x0) continue;
        // courses 10 px high, stones of hashed lengths; the joints a shade darker
        int course = (y - Y0) / 10, cy0 = (y - Y0) % 10;
        if (cy0 == 9) { Wall(x0, y, x1 - x0, 1, PL_DEEP); continue; }
        Wall(x0, y, x1 - x0, 1, PL_DARK);
        int x = x0 - (int)(Hash2(course, 5) % 40);
        while (x < x1) {
            int len = 26 + (int)(Hash2(x, course) % 30);
            if (x + len > x0 && x + len < x1) Wall(x + len, y, 1, 1, PL_DEEP);
            x += len + 1;
        }
    }
    // the arch round it: a band of stone, lit on its outer edge, its joints radial
    for (int a = 0; a <= 180 * 4; a++) {
        f32 t = a * 0.25f * 0.0174533f;
        for (int k = 0; k < 7; k++) {
            f32 rr = r + 1 + k;
            int x = (int)lroundf(cx - rr * cosf(t)), y = (int)lroundf(cy - rr * sinf(t));
            if (y < vy0 || y > vy1) continue;
            int joint = ((int)(a * 0.25f) % 9) == 0;
            Wall(x, y, 1, 1, k == 6 ? PL_STONEL : (k == 0 ? PL_DEEP : (joint ? PL_DARK : PL_STONE)));
        }
    }
    for (int side = 0; side < 2; side++) {              // and down the jambs
        int x = side ? (int)(cx + r + 1) : (int)(cx - r - 8);
        for (int y = (int)cy; y < Y0 + H; y++) {
            if (y < vy0 || y > vy1) continue;
            Wall(x, y, 7, 1, ((y - (int)cy) % 12) == 0 ? PL_DARK : PL_STONE);
            Wall(side ? x + 6 : x, y, 1, 1, PL_STONEL);
            Wall(side ? x : x + 6, y, 1, 1, PL_DEEP);
        }
    }
}

// A pilaster: a flat column in the wall, fluted, with a capital and a base.
static void Pillar(const Feature *f) {
    int X0 = f->x * TS, W = f->w * TS, Y0 = f->y * TS, H = f->h * TS;
    int vy0 = (int)camY - 4, vy1 = (int)camY + GH + 4;
    for (int y = Y0 + 12; y < Y0 + H - 10; y++) {
        if (y < vy0 || y > vy1) continue;
        Wall(X0 + 2, y, W - 4, 1, PL_STONE);
        for (int x = X0 + 5; x < X0 + W - 4; x += 4) Wall(x, y, 1, 1, PL_DARK);   // the flutes
        Wall(X0 + 2, y, 1, 1, PL_STONEL);
        Wall(X0 + W - 3, y, 1, 1, PL_DEEP);
    }
    // capital: three mouldings stepping out; base: two
    Wall(X0 - 2, Y0, W + 4, 4, PL_STONE);   Wall(X0 - 2, Y0, W + 4, 1, PL_STONEL);
    Wall(X0, Y0 + 4, W, 4, PL_STONE);       Wall(X0, Y0 + 7, W, 1, PL_DARK);
    Wall(X0 + 1, Y0 + 8, W - 2, 4, PL_STONEL); Wall(X0 + 1, Y0 + 11, W - 2, 1, PL_DARK);
    Wall(X0, Y0 + H - 10, W, 5, PL_STONE);  Wall(X0, Y0 + H - 10, W, 1, PL_STONEL);
    Wall(X0 - 2, Y0 + H - 5, W + 4, 5, PL_STONE); Wall(X0 - 2, Y0 + H - 5, W + 4, 1, PL_STONEL);
}

// A cornice along the top of the wall: a moulding and a row of dentils under it.
static void Cornice(const Feature *f) {
    int X0 = f->x * TS, W = f->w * TS, Y0 = f->y * TS;
    if (Y0 > camY + GH + 8 || Y0 + 12 < camY) return;
    Wall(X0, Y0, W, 5, PL_STONE);
    Wall(X0, Y0 + 4, W, 1, PL_STONEL);
    Wall(X0, Y0 + 5, W, 1, PL_DEEP);
    for (int x = X0 + 1; x < X0 + W - 3; x += 6) { Wall(x, Y0 + 6, 4, 4, PL_STONE); Wall(x, Y0 + 9, 4, 1, PL_DARK); }
}

// ---------------------------------------------------------------- drawing
void BackdropDraw(void) {
    for (int i = 0; ROOM_FEATURES[i].kind != F_NONE; i++) {
        const Feature *f = &ROOM_FEATURES[i];
        if (!InView(f->x * TS, f->y * TS, f->w * TS, f->h * TS)) continue;
        if (f->kind == F_NICHE) Niche(f);
        else if (f->kind == F_PILLAR) Pillar(f);
        else if (f->kind == F_CORNICE) Cornice(f);
    }
    CutOpenings();
    WindowFrame();
    GrilleBars();
    const Feature *c = Find(F_COLOSSUS);
    if (c && InView(36 * TS, TS, (f32)texColossus.width, (f32)texColossus.height))
        DrawTexture(texColossus, 36 * TS, ROOM_Y + TS, WHITE);
}

// What gives its own light: the colossus's eye, green glass, and the sliver of the other.
void BackdropDrawEmis(void) {
    const Feature *c = Find(F_COLOSSUS);
    if (!c) return;
    int ex = 36 * TS + 150, ey = ROOM_Y + TS + 46;
    if (!InView((f32)ex - 8, (f32)ey - 8, 16, 16)) return;
    DrawRectangle(ex, ey, 5, 2, PAL[PL_CITY]);
    DrawRectangle(ex + 1, ey - 1, 3, 1, PAL[PL_CITY]);
    DrawRectangle(ex + 1, ey, 2, 1, PAL[PL_CITYH]);
    DrawRectangle(ex - 9, ey + 3, 2, 1, PAL[PL_COOLM]);
}

void BackdropLights(void) {
    // the eye lights a little of the face round it, in their colour
    LightAddPointCool(36 * TS + 152, TS + 46, 3.0f, 0.35f);
}
