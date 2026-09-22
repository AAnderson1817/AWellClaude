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
extern const unsigned char ART_DOOR_IRIS[], ART_DOOR_IRIS_GLOW[], ART_DOOR_HEARTWOOD[], ART_DOOR_HEARTWOOD_GLOW[],
                           ART_DOOR_STACKS[], ART_DOOR_STACKS_GLOW[], ART_DOOR_ENGINE[], ART_DOOR_ENGINE_GLOW[];
extern const int ART_DOOR_IRIS_LEN, ART_DOOR_IRIS_GLOW_LEN, ART_DOOR_HEARTWOOD_LEN, ART_DOOR_HEARTWOOD_GLOW_LEN,
                 ART_DOOR_STACKS_LEN, ART_DOOR_STACKS_GLOW_LEN, ART_DOOR_ENGINE_LEN, ART_DOOR_ENGINE_GLOW_LEN;

// The temple door, in the designs being chosen between (tools/art/doors.py). Each is a
// picture for the far wall and a picture of what of it gives its own light.
static const struct { const unsigned char *a, *g; const int *an, *gn; } DOORS[DOOR_KINDS] = {
    { ART_DOOR_IRIS, ART_DOOR_IRIS_GLOW, &ART_DOOR_IRIS_LEN, &ART_DOOR_IRIS_GLOW_LEN },
    { ART_DOOR_HEARTWOOD, ART_DOOR_HEARTWOOD_GLOW, &ART_DOOR_HEARTWOOD_LEN, &ART_DOOR_HEARTWOOD_GLOW_LEN },
    { ART_DOOR_STACKS, ART_DOOR_STACKS_GLOW, &ART_DOOR_STACKS_LEN, &ART_DOOR_STACKS_GLOW_LEN },
    { ART_DOOR_ENGINE, ART_DOOR_ENGINE_GLOW, &ART_DOOR_ENGINE_LEN, &ART_DOOR_ENGINE_GLOW_LEN },
};
int doorKind = -1;                      // -1: the map's choice (F_DOOR's a)
static Texture2D doorGlow;
static int doorX, doorY;                // room px of the door picture's top-left
static f32 glowTile[RH][RW];            // how much of each tile the door's light covers, 0..1
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

// The door: its picture into the wall, its light kept as a texture of its own and as a
// coverage per tile, which the bake seeds its light from.
static void Door(const Feature *f) {
    int k = doorKind >= 0 ? doorKind : f->a;
    if (k < 0 || k >= DOOR_KINDS) k = 0;
    doorKind = k;
    doorX = f->x * TS; doorY = f->y * TS + 2;
    Image im = LoadImageFromMemory(".png", DOORS[k].a, *DOORS[k].an);
    ImageFormat(&im, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    Color *c = (Color *)im.data;
    for (int y = 0; y < im.height; y++)
        for (int x = 0; x < im.width; x++) {
            Color p = c[y * im.width + x];
            if (!p.a || doorX + x >= PW || doorY + y >= PH) continue;
            px[(doorY + y) * PW + doorX + x] = p;
        }
    UnloadImage(im);
    Image g = LoadImageFromMemory(".png", DOORS[k].g, *DOORS[k].gn);
    ImageFormat(&g, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    Color *gc = (Color *)g.data;
    memset(glowTile, 0, sizeof glowTile);
    for (int y = 0; y < g.height; y++)
        for (int x = 0; x < g.width; x++) {
            if (!gc[y * g.width + x].a) continue;
            int tx = (doorX + x) / TS, ty = (doorY + y) / TS;
            if (tx < RW && ty < RH) glowTile[ty][tx] += 1.0f / 12.0f;
        }
    for (int y = 0; y < RH; y++) for (int x = 0; x < RW; x++) if (glowTile[y][x] > 1) glowTile[y][x] = 1;
    if (doorGlow.id) UnloadTexture(doorGlow);
    doorGlow = LoadTextureFromImage(g);
    UnloadImage(g);
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
            if (pass == 2 && f->kind == F_DOOR) Door(f);
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
    if (doorGlow.id && doorX < camX + GW + 8 && doorX + doorGlow.width > camX - 8 && doorY < camY + GH + 8)
        DrawTexture(doorGlow, doorX, ROOM_Y + doorY, WHITE);
    if (!Find(F_COLOSSUS)) return;
    int ex = 36 * TS + 150, ey = ROOM_Y + TS + 46;
    if (ex < camX - 16 || ex > camX + GW + 16 || ey < camY - 16 || ey > camY + GH + 16) return;
    DrawRectangle(ex, ey, 5, 2, PAL[PL_CITY]);
    DrawRectangle(ex + 1, ey - 1, 3, 1, PAL[PL_CITY]);
    DrawRectangle(ex + 1, ey, 2, 1, PAL[PL_CITYH]);
    DrawRectangle(ex - 9, ey + 3, 2, 1, PAL[PL_COOLM]);
}

void BackdropLights(void) {
    // the eye lights a little of the face round it, in their colour
    LightAddPointCool(36 * TS + 152, TS + 46, 5.0f, 0.55f);
}
