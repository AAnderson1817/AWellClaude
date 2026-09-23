// backdrop.c -- the far wall: the archive, all six screens of it, in one picture.
//
// The picture is art/archive.png, made by tools/art/archive.py out of the building's parts
// (tools/art/kit.py, claude/ARCHIVE.md): the door's cell, the stacks, the keeper in its cell,
// the window's cell, the heart's grate, the ribs, the roots. With it come a picture of its
// own light (drawn in the emissive layer) and the paths of its veins, along which the
// archive's reading runs now and then, always toward the heart.
//
// All of it is still, so it is decoded once, when the room is entered, and the stone in
// front of it is painted into it then; each frame draws the part of it in view. Where you
// can stand on a piece of it (the colossus), the map carves a tile under it ('X') and the
// room leaves the drawing to this.
//
// The openings -- the window's cell and the heart's grate -- are holes in the picture: alpha
// zero, which is how the composite knows to show what is beyond (city.c). They are circles,
// the map's F_WINDOW and F_GRILLE squares, and the picture is cut to the same circles.
#include "aw.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

extern const unsigned char ART_ARCHIVE[], ART_ARCHIVE_GLOW[];
extern const int ART_ARCHIVE_LEN, ART_ARCHIVE_GLOW_LEN;
extern const i16 VEINS_ARCHIVE[];        // x, y pairs in room px; a path ends at -1
extern const int VEINS_ARCHIVE_LEN;

#define PW (RW * TS)
#define PH (RH * TS)
static Texture2D wallTex, glowTex;
static Color *px;                       // the picture, while the stone is painted into it
static f32 glowTile[RH][RW];            // how much of each tile the archive's own light covers, 0..1
static u8 *glowAt;                      // per room px, whether its own light shows there (not behind stone
                                        // or the colossus): a pulse shows only where its vein does
f32 BackdropGlow(int tx, int ty) { return (tx < 0 || tx >= RW || ty < 0 || ty >= RH) ? 0 : glowTile[ty][tx]; }

static const Feature *Find(int kind) {
    for (int i = 0; ROOM_FEATURES[i].kind != F_NONE; i++)
        if (ROOM_FEATURES[i].kind == kind) return &ROOM_FEATURES[i];
    return 0;
}

// Stone in front, at a room px: what the far wall's light must not shine through.
static int Hidden(int x, int y) {
    int tx = x / TS, ty = y / TS;
    u8 t = TileGet(tx, ty);
    return (t == T_ROCK || t == T_VEIN) && !(tileDeco[ty][tx] & TD_CARVED);
}

static int Shows(int x, int y) {
    for (int j = y - 1; j <= y + 1; j++)
        for (int i = x - 1; i <= x + 1; i++)
            if (i >= 0 && i < PW && j >= 0 && j < PH && glowAt[j * PW + i]) return 1;
    return 0;
}

static void Put(int x, int y, int pl, u8 tag) {
    if (x < 0 || x >= PW || y < 0 || y >= PH) return;
    Color c = PAL[pl]; c.a = tag;
    px[y * PW + x] = c;
}

// Smooth noise, 0..1, on a lattice of `cell` px.
static f32 Noise(int x, int y, int cell, int seed) {
    int cx = x / cell, cy = y / cell;
    f32 fx = (f32)(x % cell) / cell, fy = (f32)(y % cell) / cell;
    fx = fx * fx * (3 - 2 * fx); fy = fy * fy * (3 - 2 * fy);
    f32 a = (Hash2(cx + seed, cy) & 1023) / 1023.0f, b = (Hash2(cx + 1 + seed, cy) & 1023) / 1023.0f;
    f32 c = (Hash2(cx + seed, cy + 1) & 1023) / 1023.0f, d = (Hash2(cx + 1 + seed, cy + 1) & 1023) / 1023.0f;
    return (a + (b - a) * fx) * (1 - fy) + (c + (d - c) * fx) * fy;
}

// ---------------------------------------------------------------- the openings
// Room px spans of a round opening, per row: the circle in the feature's square of tiles.
static int CircleSpan(const Feature *f, int y, int *x0, int *x1) {
    f32 r = f->w * TS * 0.5f, cx = f->x * TS + r, cy = f->y * TS + r, dy = (y + 0.5f) - cy;
    if (fabsf(dy) >= r) return 0;
    f32 hw = sqrtf(r * r - dy * dy);
    *x0 = (int)lroundf(cx - hw); *x1 = (int)lroundf(cx + hw);
    return *x1 > *x0;
}
int WindowSpan(int y, int *x0, int *x1) { const Feature *f = Find(F_WINDOW); return f && CircleSpan(f, y, x0, x1); }
int GrilleSpan(int y, int *x0, int *x1) { const Feature *f = Find(F_GRILLE); return f && CircleSpan(f, y, x0, x1); }
int OpeningBox(int kind, int *x, int *y, int *w, int *h) {
    const Feature *f = Find(kind);
    if (!f) return 0;
    *x = f->x * TS; *y = f->y * TS; *w = f->w * TS; *h = f->h * TS;
    return 1;
}

// The archive reading: now and then a pulse of light runs the length of a vein, always the
// same way -- toward the heart. Each vein has its own slow period, so they never march.
static void VeinPulses(void) {
    const i16 *v = VEINS_ARCHIVE;
    int n = VEINS_ARCHIVE_LEN, start = 0, path = 0;
    while (start < n) {
        int end = start;
        while (end < n && v[end] >= 0) end += 2;
        // the path's length, and where along it the pulse is
        f32 len = 0;
        for (int i = start; i + 3 < end; i += 2) len += hypotf((f32)(v[i + 2] - v[i]), (f32)(v[i + 3] - v[i + 1]));
        int period = 720 + (int)(Hash2(path, 7) % 840);          // half as often as they were: the user found them busy
        f32 t = (f32)((frameNo + Hash2(path, 0) % period) % period) * 1.3f;
        if (t < len) {
            for (int tail = 0; tail < 5; tail++) {
                f32 want = t - tail * 1.5f, acc = 0;
                if (want < 0) break;
                for (int i = start; i + 3 < end; i += 2) {
                    f32 dx = (f32)(v[i + 2] - v[i]), dy = (f32)(v[i + 3] - v[i + 1]), L = hypotf(dx, dy);
                    if (acc + L >= want && L > 0) {
                        f32 u = (want - acc) / L;
                        int x = (int)(v[i] + dx * u), y = (int)(v[i + 1] + dy * u);
                        if (x > camX - 4 && x < camX + GW + 4 && y > camY - 4 && y < camY + GH + 4 && Shows(x, y))
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

// ---------------------------------------------------------------- the stone
// Masonry, where the map has the city's stone: the same giant courses as the wall behind,
// set a course apart so a mass and the wall do not read as one surface, and lighter -- it is
// nearer. Its open faces edged: a lit cap on top, a shadowed foot, a lit left side.
#define COURSE 16
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
    Image im = LoadImageFromMemory(".png", ART_ARCHIVE, ART_ARCHIVE_LEN);
    ImageFormat(&im, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    if (im.data && (im.width != PW || im.height != PH))
        TraceLog(LOG_WARNING, "art/archive.png is %dx%d, not the room's %dx%d", im.width, im.height, PW, PH);
    px = (im.data && im.width == PW && im.height == PH) ? (Color *)im.data : 0;
    for (int ty = 0; ty < RH && px; ty++)         // the stone in front of all of it
        for (int tx = 0; tx < RW; tx++) {
            if (!TileBaked(tx, ty)) continue;
            if (ZoneAt(tx, ty) == Z_CITY) CityStone(tx, ty); else BuriedRock(tx, ty);
        }
    if (wallTex.id) UnloadTexture(wallTex);
    wallTex = LoadTextureFromImage(im);
    UnloadImage(im);
    px = 0;
    // its own light: none of it through the stone in front, and what is left seeds the bake
    memset(glowTile, 0, sizeof glowTile);
    if (!glowAt) glowAt = (u8 *)calloc(PW * PH, 1);
    memset(glowAt, 0, PW * PH);
    Image g = LoadImageFromMemory(".png", ART_ARCHIVE_GLOW, ART_ARCHIVE_GLOW_LEN);
    ImageFormat(&g, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    Color *gc = (Color *)g.data;
    for (int y = 0; y < g.height && y < PH; y++)
        for (int x = 0; x < g.width && x < PW; x++) {
            if (!gc[y * g.width + x].a) continue;
            if (Hidden(x, y)) { gc[y * g.width + x] = BLANK; continue; }
            glowAt[y * PW + x] = 1;
            if (glowTile[y / TS][x / TS] < 1) glowTile[y / TS][x / TS] += 1.0f / 12.0f;
        }
    if (glowTex.id) UnloadTexture(glowTex);
    glowTex = LoadTextureFromImage(g);
    UnloadImage(g);
}

// The part of the far wall in view, with a tile of margin.
static Rectangle InView(void) {
    int x0 = (int)floorf(camX) - TS, y0 = (int)floorf(camY) - TS;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    int w = GW + 2 * TS, h = GH + 2 * TS;
    if (x0 + w > PW) w = PW - x0;
    if (y0 + h > PH) h = PH - y0;
    return (Rectangle){ (f32)x0, (f32)y0, (f32)w, (f32)h };
}
void BackdropDraw(void) {
    Rectangle r = InView();
    DrawTextureRec(wallTex, r, (Vector2){ r.x, ROOM_Y + r.y }, WHITE);
}

// What gives its own light: the archive's glass, the keeper's eye, the reading in the veins.
void BackdropDrawEmis(void) {
    Rectangle r = InView();
    DrawTextureRec(glowTex, r, (Vector2){ r.x, ROOM_Y + r.y }, WHITE);
    VeinPulses();
}

void BackdropLights(void) {
    // the door is a cell awake: its lens lights its own blades from within, their colour
    LightAddPointCool(104.0f, 88.0f, 9.5f, 0.75f + 0.08f * sinf(frameNo * 0.021f));
    // the keeper's eye lights a little of the face round it
    LightAddPointCool(440.0f, 55.0f, 3.5f, 0.5f);
}
