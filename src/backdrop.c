// backdrop.c -- the far wall and the hall's big pieces on it: the colossus in its niche, the
// great window, the fireguard, the pilasters and the cornice.
//
// All of it is still, so it is painted once, when the room is entered, into one picture of
// the whole room's far wall, and each frame draws the part of it in view. That is what lets
// it be painted a pixel at a time: the tall ones' masonry in blocks the size of a person, the
// raw rock of the vault rough and cracked, the colossus sculpted (art/colossus.png, made by
// tools/art/colossus.py). Where you can stand on a piece, the map carves a tile under it
// ('X') and the room leaves the drawing to this.
//
// The openings -- the window, the fireguard -- are holes in the picture: alpha zero, which is
// how the composite knows to show what is beyond (city.c).
#include "aw.h"
#include <math.h>
#include <string.h>

extern const unsigned char ART_COLOSSUS[];
extern const int ART_COLOSSUS_LEN;
extern const unsigned char ART_BAY_A[], ART_BAY_A_GLOW[];
extern const int ART_BAY_A_LEN, ART_BAY_A_GLOW_LEN;
extern const i16 VEINS_BAY_A[];          // x, y pairs; a path ends at -1
extern const int VEINS_BAY_A_LEN;

// The bays: the far wall of each screen drawn in the archive's parts (claude/ARCHIVE.md,
// tools/art/kit.py). A picture for the wall, a picture of its own light, the paths of its
// veins. Built so far: A.
static const struct { const unsigned char *a, *g; const int *an, *gn; const i16 *v; const int *vn; } BAYS[] = {
    { ART_BAY_A, ART_BAY_A_GLOW, &ART_BAY_A_LEN, &ART_BAY_A_GLOW_LEN, VEINS_BAY_A, &VEINS_BAY_A_LEN },
};
#define BAY_KINDS ((int)(sizeof BAYS / sizeof BAYS[0]))
static Texture2D bayGlow[BAY_KINDS];
static int bayX[BAY_KINDS], bayY[BAY_KINDS];     // room px of each bay picture's top-left
static f32 glowTile[RH][RW];            // how much of each tile a bay's own light covers, 0..1
f32 BackdropGlow(int tx, int ty) { return (tx < 0 || tx >= RW || ty < 0 || ty >= RH) ? 0 : glowTile[ty][tx]; }

static Texture2D wallTex;
static Color *px;                       // the picture being painted, (RW*TS) x (RH*TS)
#define PW (RW * TS)
#define PH (RH * TS)

static const Feature *Find(int kind) {
    for (int i = 0; ROOM_FEATURES[i].kind != F_NONE; i++)
        if (ROOM_FEATURES[i].kind == kind) return &ROOM_FEATURES[i];
    return 0;
}

// ---------------------------------------------------------------- painting
static void Put(int x, int y, int pl, u8 tag) {
    if (x < 0 || x >= PW || y < 0 || y >= PH) return;
    Color c = PAL[pl]; c.a = tag;
    px[y * PW + x] = c;
}
static void Fill(int x, int y, int w, int h, int pl, u8 tag) {
    for (int j = y; j < y + h; j++) for (int i = x; i < x + w; i++) Put(i, j, pl, tag);
}
static void Hole(int x, int y) { if (x >= 0 && x < PW && y >= 0 && y < PH) px[y * PW + x] = (Color){ 0, 0, 0, 0 }; }

// Smooth noise, 0..1, on a lattice of `cell` px.
static f32 Noise(int x, int y, int cell, int seed) {
    int cx = x / cell, cy = y / cell;
    f32 fx = (f32)(x % cell) / cell, fy = (f32)(y % cell) / cell;
    fx = fx * fx * (3 - 2 * fx); fy = fy * fy * (3 - 2 * fy);
    f32 a = (Hash2(cx + seed, cy) & 1023) / 1023.0f, b = (Hash2(cx + 1 + seed, cy) & 1023) / 1023.0f;
    f32 c = (Hash2(cx + seed, cy + 1) & 1023) / 1023.0f, d = (Hash2(cx + 1 + seed, cy + 1) & 1023) / 1023.0f;
    return (a + (b - a) * fx) * (1 - fy) + (c + (d - c) * fx) * fy;
}

// The tall ones' masonry: courses 16 px high, blocks two to four tiles long -- a person is
// less than one block high. Each block a face, a lit top edge where it is set in, joints dark;
// now and then a block weathered darker, a chip out of a corner.
#define COURSE 16
static void CityWall(int x, int y) {
    int c = y / COURSE, r = y % COURSE;
    // the joints in this course: an offset, then hashed lengths
    int j = -(int)(Hash2(c, 91) % 40), k = 0, len = 0;
    while (1) { len = 22 + (int)(Hash2(c * 131 + k, 17) % 20); if (j + len > x) break; j += len + 1; k++; }
    int bx = x - j;                             // px into this block
    u32 h = Hash2(c * 131 + k, 5);
    if (r == COURSE - 1 || bx == len) { Put(x, y, PL_DEEP, TAG_WALL); return; }   // joints
    int face = (h & 7) == 0 ? PL_DARK : PL_STONE;                                  // a weathered block
    if (r == 0) { Put(x, y, PL_STONEL, TAG_WALL); return; }                        // the set-in top edge
    if (bx == 0 || r == COURSE - 2) { Put(x, y, PL_DARK, TAG_WALL); return; }       // shadowed side and foot
    if ((h >> 4 & 7) == 1 && bx < 4 && r < 5 && bx + r < 5) { Put(x, y, PL_DARK, TAG_WALL); return; }   // a chipped corner
    u32 g = Hash2(x, y * 7);
    if ((g & 63) == 0) { Put(x, y, PL_DARK, TAG_WALL); return; }                   // pitting
    if ((g & 255) == 1) { Put(x, y, PL_STONEL, TAG_WALL); return; }
    Put(x, y, face, TAG_WALL);
}

// The vault's raw rock: rough, in patches of dark and less dark, cracked, never coursed.
static void CaveWall(int x, int y) {
    // strata, tipped a little and wandering, broken by cracks; the faces between them rough
    f32 wob = Noise(x, y, 24, 13) * 10.0f;
    f32 s = (y + x * 0.18f + wob) / 9.0f;
    f32 band = s - floorf(s);
    f32 n = Noise(x, y, 7, 3) * 0.55f + Noise(x, y, 3, 7) * 0.45f;
    int pl = n > 0.66f ? PL_STONE : PL_DARK;
    if (band < 0.12f) pl = PL_DEEP;                            // the bed between two strata
    else if (band < 0.22f && n > 0.45f) pl = PL_STONE;          // its upper lip
    f32 crack = fabsf(Noise(x, y, 16, 11) - 0.5f);
    if (crack < 0.02f) pl = PL_DEEP;
    u32 g = Hash2(x * 3, y * 5);
    if ((g & 255) == 0) pl = PL_STONEL;
    Put(x, y, pl, TAG_WALL);
}

// ---------------------------------------------------------------- the pieces
static void Niche(const Feature *f) {
    // A round-headed recess, deeper than the wall and so darker, in long courses; its arch a
    // band of voussoirs lit on the outer edge; its jambs straight.
    int X0 = f->x * TS, W = f->w * TS, Y0 = f->y * TS, H = f->h * TS;
    f32 r = W * 0.5f, cx = X0 + r, cy = Y0 + r;
    for (int y = Y0; y < Y0 + H; y++) {
        f32 hw = r;
        if (y < cy) { f32 dy = cy - (y + 0.5f); hw = sqrtf(fmaxf(r * r - dy * dy, 0)); }
        int x0 = (int)lroundf(cx - hw), x1 = (int)lroundf(cx + hw);
        int course = (y - Y0) / 12, cr = (y - Y0) % 12;
        for (int x = x0; x < x1; x++) {
            int pl = cr == 11 ? PL_DEEP : PL_DARK;
            int j = (int)(Hash2(course, 5) % 50);
            if (((x - X0 + j) % 57) == 0) pl = PL_DEEP;
            if (cr == 0 && (Hash2(x, course) & 3) == 0) pl = PL_STONE;
            Put(x, y, pl, TAG_WALL);
        }
    }
    for (int a = 0; a <= 180 * 4; a++) {
        f32 t = a * 0.25f * 0.0174533f;
        for (int k = 0; k < 9; k++) {
            f32 rr = r + 1 + k;
            int x = (int)lroundf(cx - rr * cosf(t)), y = (int)lroundf(cy - rr * sinf(t));
            int joint = ((int)(a * 0.25f) % 8) == 0;
            Put(x, y, k == 8 ? PL_STONEL : (k == 0 ? PL_DEEP : (joint ? PL_DARK : (k < 3 ? PL_STONEL : PL_STONE))), TAG_WALL);
        }
    }
    for (int side = 0; side < 2; side++) {
        int x = side ? (int)(cx + r + 1) : (int)(cx - r - 10);
        for (int y = (int)cy; y < Y0 + H; y++) {
            for (int i = 0; i < 9; i++) Put(x + i, y, ((y - (int)cy) % 16) == 15 ? PL_DARK : PL_STONE, TAG_WALL);
            Put(side ? x + 8 : x, y, PL_STONEL, TAG_WALL);
            Put(side ? x : x + 8, y, PL_DEEP, TAG_WALL);
        }
    }
}

static void Pillar(const Feature *f) {
    // A pilaster, fluted, with a capital of three mouldings and a base of two.
    int X0 = f->x * TS, W = f->w * TS, Y0 = f->y * TS, H = f->h * TS;
    for (int y = Y0 + 12; y < Y0 + H - 10; y++)
        for (int x = X0 + 2; x < X0 + W - 2; x++) {
            int pl = ((x - X0 - 1) % 4) == 0 ? PL_DARK : PL_STONE;
            if (x == X0 + 2) pl = PL_STONEL;
            if (x == X0 + W - 3) pl = PL_DEEP;
            Put(x, y, pl, TAG_WALL);
        }
    Fill(X0 - 2, Y0, W + 4, 4, PL_STONE, TAG_WALL);   Fill(X0 - 2, Y0, W + 4, 1, PL_STONEL, TAG_WALL);
    Fill(X0, Y0 + 4, W, 4, PL_STONE, TAG_WALL);       Fill(X0, Y0 + 7, W, 1, PL_DARK, TAG_WALL);
    Fill(X0 + 1, Y0 + 8, W - 2, 4, PL_STONEL, TAG_WALL); Fill(X0 + 1, Y0 + 11, W - 2, 1, PL_DARK, TAG_WALL);
    Fill(X0, Y0 + H - 10, W, 5, PL_STONE, TAG_WALL);  Fill(X0, Y0 + H - 10, W, 1, PL_STONEL, TAG_WALL);
    Fill(X0 - 2, Y0 + H - 5, W + 4, 5, PL_STONE, TAG_WALL); Fill(X0 - 2, Y0 + H - 5, W + 4, 1, PL_STONEL, TAG_WALL);
}

static void Cornice(const Feature *f) {
    int X0 = f->x * TS, W = f->w * TS, Y0 = f->y * TS;
    Fill(X0, Y0, W, 5, PL_STONE, TAG_WALL);
    Fill(X0, Y0 + 4, W, 1, PL_STONEL, TAG_WALL);
    Fill(X0, Y0 + 5, W, 1, PL_DEEP, TAG_WALL);
    for (int x = X0 + 1; x < X0 + W - 3; x += 6) { Fill(x, Y0 + 6, 4, 4, PL_STONE, TAG_WALL); Fill(x, Y0 + 9, 4, 1, PL_DARK, TAG_WALL); }
}

// ---------------------------------------------------------------- the openings
// The great window: a tall round-headed arch. The fireguard: a lower one, square-headed.
// Both are spans of room px per row, so the cut, the frame and the city agree exactly.
static int ArchSpan(const Feature *f, int round, int y, int *x0, int *x1) {
    f32 X0 = f->x * TS, W = f->w * TS, Y0 = f->y * TS, H = f->h * TS;
    if (y < Y0 || y >= Y0 + H) return 0;
    f32 r = W * 0.5f, cx = X0 + r, hw = r;
    if (round && y < Y0 + r) {
        f32 dy = (Y0 + r) - (y + 0.5f);
        hw = sqrtf(fmaxf(r * r - dy * dy, 0.0f));
    }
    if (hw < 1.0f) return 0;
    *x0 = (int)lroundf(cx - hw); *x1 = (int)lroundf(cx + hw);
    return *x1 > *x0;
}
int WindowSpan(int y, int *x0, int *x1) { const Feature *f = Find(F_WINDOW); return f && ArchSpan(f, 1, y, x0, x1); }
int GrilleSpan(int y, int *x0, int *x1) { const Feature *f = Find(F_GRILLE); return f && ArchSpan(f, 0, y, x0, x1); }

static void Window(const Feature *f) {
    // Cut, then frame: a deep moulded arch round it, and two slender mullions -- the lights
    // tall and narrow, so the city is seen in three long strips and the eye goes up them.
    int X0 = f->x * TS, W = f->w * TS, Y0 = f->y * TS, H = f->h * TS;
    f32 r = W * 0.5f, cx = X0 + r, cy = Y0 + r;
    for (int y = Y0; y < Y0 + H; y++) {
        int x0, x1;
        if (!ArchSpan(f, 1, y, &x0, &x1)) continue;
        for (int x = x0; x < x1; x++) Hole(x, y);
    }
    for (int a = 0; a <= 180 * 4; a++) {                     // the arch: three orders stepping in
        f32 t = a * 0.25f * 0.0174533f;
        for (int k = 0; k < 12; k++) {
            f32 rr = r + 1 + k;
            int x = (int)lroundf(cx - rr * cosf(t)), y = (int)lroundf(cy - rr * sinf(t));
            int pl = (k == 0 || k == 4 || k == 8) ? PL_STONEL : ((k == 3 || k == 7) ? PL_DEEP : PL_STONE);
            if (k == 11) pl = PL_STONEL;
            if (((int)(a * 0.25f) % 7) == 0 && k > 8) pl = PL_DARK;
            Put(x, y, pl, TAG_STONE);
        }
    }
    for (int side = 0; side < 2; side++)                      // the jambs, the same three orders
        for (int y = (int)cy; y < Y0 + H; y++)
            for (int k = 0; k < 12; k++) {
                int x = side ? (int)(cx + r) + k : (int)(cx - r) - 1 - k;
                int pl = (k == 0 || k == 4 || k == 8) ? PL_STONEL : ((k == 3 || k == 7) ? PL_DEEP : PL_STONE);
                Put(x, y, pl, TAG_STONE);
            }
    int m1 = X0 + W / 3, m2 = X0 + 2 * W / 3;
    for (int y = Y0; y < Y0 + H; y++) {
        int x0, x1;
        if (!ArchSpan(f, 1, y, &x0, &x1)) continue;
        for (int m = 0; m < 2; m++) {
            int mx = m ? m2 : m1;
            if (mx - 1 < x0 || mx + 2 > x1) continue;
            Put(mx - 1, y, PL_STONEL, TAG_STONE); Put(mx, y, PL_STONE, TAG_STONE); Put(mx + 1, y, PL_DARK, TAG_STONE);
        }
    }
}

static void Grille(const Feature *f) {
    // The fireguard: cut, then iron bars, bands across them, a lintel over. The foot of it
    // is in the water.
    int X0 = f->x * TS, W = f->w * TS, Y0 = f->y * TS, H = f->h * TS;
    for (int y = Y0; y < Y0 + H; y++) for (int x = X0; x < X0 + W; x++) Hole(x, y);
    for (int x = X0 + 3; x < X0 + W - 1; x += 7) Fill(x, Y0, 2, H, PL_DARK, TAG_STONE), Fill(x, Y0, 1, H, PL_STONE, TAG_STONE);
    for (int y = Y0 + 10; y < Y0 + H; y += 40) Fill(X0, y, W, 2, PL_DARK, TAG_STONE), Fill(X0, y, W, 1, PL_STONE, TAG_STONE);
    Fill(X0 - 4, Y0 - 6, W + 8, 6, PL_STONE, TAG_STONE);
    Fill(X0 - 4, Y0 - 6, W + 8, 1, PL_STONEL, TAG_STONE);
    Fill(X0 - 4, Y0 - 1, W + 8, 1, PL_DEEP, TAG_STONE);
    for (int side = 0; side < 2; side++) Fill(side ? X0 + W : X0 - 4, Y0, 4, H, PL_STONE, TAG_STONE);
}

static void Colossus(const Feature *f) {
    Image im = LoadImageFromMemory(".png", ART_COLOSSUS, ART_COLOSSUS_LEN);
    ImageFormat(&im, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    Color *c = (Color *)im.data;
    int ox = 36 * TS, oy = TS;          // tools/art/colossus.py: the canvas starts at tile (36, 1)
    (void)f;
    for (int y = 0; y < im.height; y++)
        for (int x = 0; x < im.width; x++) {
            Color p = c[y * im.width + x];
            if (!p.a || ox + x >= PW || oy + y >= PH) continue;
            px[(oy + y) * PW + ox + x] = p;
        }
    UnloadImage(im);
}

// A bay: its picture into the wall, its light kept as a texture of its own and as a coverage
// per tile, which the bake seeds its light from.
static void Bay(const Feature *f) {
    int k = f->a;
    if (k < 0 || k >= BAY_KINDS) return;
    bayX[k] = f->x * TS; bayY[k] = f->y * TS;
    Image im = LoadImageFromMemory(".png", BAYS[k].a, *BAYS[k].an);
    ImageFormat(&im, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    Color *c = (Color *)im.data;
    for (int y = 0; y < im.height; y++)
        for (int x = 0; x < im.width; x++) {
            Color p = c[y * im.width + x];
            int X = bayX[k] + x, Y = bayY[k] + y;
            if (!p.a || X < 0 || X >= PW || Y < 0 || Y >= PH) continue;
            px[Y * PW + X] = p;
        }
    UnloadImage(im);
    Image g = LoadImageFromMemory(".png", BAYS[k].g, *BAYS[k].gn);
    ImageFormat(&g, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    Color *gc = (Color *)g.data;
    for (int y = 0; y < g.height; y++)
        for (int x = 0; x < g.width; x++) {
            if (!gc[y * g.width + x].a) continue;
            int tx = (bayX[k] + x) / TS, ty = (bayY[k] + y) / TS;
            if (tx >= 0 && tx < RW && ty >= 0 && ty < RH && glowTile[ty][tx] < 1) glowTile[ty][tx] += 1.0f / 12.0f;
        }
    if (bayGlow[k].id) UnloadTexture(bayGlow[k]);
    bayGlow[k] = LoadTextureFromImage(g);
    UnloadImage(g);
}

// The archive reading: now and then a pulse of light runs the length of a vein, always the
// same way -- toward the heart. Each vein has its own slow period, so they never march.
static void VeinPulses(void) {
    for (int k = 0; k < BAY_KINDS; k++) {
        const i16 *v = BAYS[k].v;
        int n = *BAYS[k].vn, start = 0, path = 0;
        while (start < n) {
            int end = start;
            while (end < n && v[end] >= 0) end += 2;
            // the path's length, and where along it the pulse is
            f32 len = 0;
            for (int i = start; i + 3 < end; i += 2) len += hypotf((f32)(v[i + 2] - v[i]), (f32)(v[i + 3] - v[i + 1]));
            int period = 360 + (int)(Hash2(k * 31 + path, 7) % 420);
            f32 t = (f32)((frameNo + Hash2(path, k) % period) % period) * 1.3f;
            if (t < len) {
                for (int tail = 0; tail < 5; tail++) {
                    f32 want = t - tail * 1.5f, acc = 0;
                    if (want < 0) break;
                    for (int i = start; i + 3 < end; i += 2) {
                        f32 dx = (f32)(v[i + 2] - v[i]), dy = (f32)(v[i + 3] - v[i + 1]), L = hypotf(dx, dy);
                        if (acc + L >= want && L > 0) {
                            f32 u = (want - acc) / L;
                            int x = (int)(v[i] + dx * u), y = (int)(v[i + 1] + dy * u);
                            if (x > camX - 4 && x < camX + GW + 4 && y > camY - 4 && y < camY + GH + 4)
                                DrawRectangle(x, ROOM_Y + y, 1, 1, PAL[tail == 0 ? PL_CITYH : (tail < 3 ? PL_CITY : PL_COOLM)]);
                            break;
                        }
                        acc += L;
                    }
                }
            }
            start = end + 1; path++;
        }
    }
}

// ---------------------------------------------------------------- the stone
// Masonry, where the map has the city's stone: the same giant courses as the wall behind,
// set a course apart so a mass and the wall do not read as one surface, and lighter -- it is
// nearer. Its open faces edged: a lit cap on top, a shadowed foot, a lit left side.
static int Solid(int tx, int ty) { u8 t = TileGet(tx, ty); return t == T_ROCK || t == T_VEIN; }
static void CityStone(int tx, int ty) {
    int up = Solid(tx, ty - 1), dn = Solid(tx, ty + 1), lf = Solid(tx - 1, ty), rt = Solid(tx + 1, ty);
    for (int j = 0; j < TS; j++)
        for (int i = 0; i < TS; i++) {
            int x = tx * TS + i, y = ty * TS + j, yy = y + COURSE / 2;
            int c = yy / COURSE, r = yy % COURSE;
            int jx = -(int)(Hash2(c, 311) % 40), k = 0, len = 0;
            while (1) { len = 20 + (int)(Hash2(c * 57 + k, 23) % 22); if (jx + len > x) break; jx += len + 1; k++; }
            int bx = x - jx, pl = PL_STONEL;
            if (r == COURSE - 1 || bx == len) pl = PL_STONE;
            else if (r == 0) pl = PL_STONEH;
            else if ((Hash2(x, y * 3) & 63) == 0) pl = PL_STONE;
            if (!up && j == 0) pl = PL_STONEH;
            else if (!up && j == 1) pl = PL_STONEL;
            if (!dn && j == TS - 1) pl = PL_DEEP;
            if (!lf && i == 0 && (up || j > 0)) pl = PL_STONEL;
            if (!rt && i == TS - 1 && (up || j > 0)) pl = PL_DARK;
            Put(x, y, pl, TAG_STONE);
        }
}
// Raw rock, buried: patches of less dark in the dark, cracks, a pebble now and then -- a great
// mass of it is still rock and not a hole.
static void BuriedRock(int tx, int ty) {
    for (int j = 0; j < TS; j++)
        for (int i = 0; i < TS; i++) {
            int x = tx * TS + i, y = ty * TS + j;
            f32 n = Noise(x, y, 5, 21) * 0.6f + Noise(x, y, 3, 29) * 0.4f;
            int pl = n > 0.64f ? PL_STONEL : PL_STONE;
            f32 crack = fabsf(Noise(x, y, 14, 41) - 0.5f);
            if (crack < 0.025f) pl = PL_DARK;
            if ((Hash2(x * 7, y) & 511) == 0) pl = PL_STONEH;
            Put(x, y, pl, TAG_STONE);
        }
}

// ---------------------------------------------------------------- build
void BackdropInit(void) {
    memset(glowTile, 0, sizeof glowTile);
    Image im = GenImageColor(PW, PH, BLANK);
    ImageFormat(&im, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    px = (Color *)im.data;
    for (int y = 0; y < PH; y++)
        for (int x = 0; x < PW; x++) {
            if (ZoneAt(x / TS, y / TS) == Z_CITY) CityWall(x, y); else CaveWall(x, y);
        }
    for (int pass = 0; pass < 3; pass++)          // architecture, then the openings, then the colossus
        for (int i = 0; ROOM_FEATURES[i].kind != F_NONE; i++) {
            const Feature *f = &ROOM_FEATURES[i];
            if (pass == 0 && f->kind == F_NICHE) Niche(f);
            if (pass == 0 && f->kind == F_PILLAR) Pillar(f);
            if (pass == 0 && f->kind == F_CORNICE) Cornice(f);
            if (pass == 1 && f->kind == F_WINDOW) Window(f);
            if (pass == 1 && f->kind == F_GRILLE) Grille(f);
            if (pass == 2 && f->kind == F_COLOSSUS) Colossus(f);
            if (pass == 2 && f->kind == F_BAY) Bay(f);
        }
    for (int ty = 0; ty < RH; ty++)               // and last, the stone in front of all of it
        for (int tx = 0; tx < RW; tx++) {
            if (!TileBaked(tx, ty)) continue;
            if (ZoneAt(tx, ty) == Z_CITY) CityStone(tx, ty); else BuriedRock(tx, ty);
        }
    if (wallTex.id) UnloadTexture(wallTex);
    wallTex = LoadTextureFromImage(im);
    UnloadImage(im);
    px = 0;
}

// The part of the far wall in view, with a tile of margin.
void BackdropDraw(void) {
    int x0 = (int)floorf(camX) - TS, y0 = (int)floorf(camY) - TS;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    int w = GW + 2 * TS, h = GH + 2 * TS;
    if (x0 + w > PW) w = PW - x0;
    if (y0 + h > PH) h = PH - y0;
    DrawTextureRec(wallTex, (Rectangle){ (f32)x0, (f32)y0, (f32)w, (f32)h }, (Vector2){ (f32)x0, (f32)(ROOM_Y + y0) }, WHITE);
}

// What gives its own light: the colossus's eye, green glass, and the sliver of the other.
void BackdropDrawEmis(void) {
    for (int k = 0; k < BAY_KINDS; k++)
        if (bayGlow[k].id && bayX[k] < camX + GW + 8 && bayX[k] + bayGlow[k].width > camX - 8
            && bayY[k] < camY + GH + 8 && bayY[k] + bayGlow[k].height > camY - 8)
            DrawTexture(bayGlow[k], bayX[k], ROOM_Y + bayY[k], WHITE);
    VeinPulses();
    if (!Find(F_COLOSSUS)) return;
    int ex = 36 * TS + 150, ey = ROOM_Y + TS + 46;
    if (ex < camX - 16 || ex > camX + GW + 16 || ey < camY - 16 || ey > camY + GH + 16) return;
    DrawRectangle(ex, ey, 5, 2, PAL[PL_CITY]);
    DrawRectangle(ex + 1, ey - 1, 3, 1, PAL[PL_CITY]);
    DrawRectangle(ex + 1, ey, 2, 1, PAL[PL_CITYH]);
    DrawRectangle(ex - 9, ey + 3, 2, 1, PAL[PL_COOLM]);
}

void BackdropLights(void) {
    // bay A's door is a cell awake: its lens lights its own blades from within, their colour
    if (BAY_KINDS > 0 && bayGlow[0].id) LightAddPointCool(bayX[0] + 104.0f, bayY[0] + 88.0f, 9.5f, 0.75f + 0.08f * sinf(frameNo * 0.021f));
    // the eye lights a little of the face round it, in their colour
    LightAddPointCool(36 * TS + 152, TS + 46, 5.0f, 0.55f);
}
