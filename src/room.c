// room.c -- the one room: its tiles, its light, and how it is drawn.
//
// The map is authored as text right here and read once at startup. It is level
// data: nothing writes to it while the game runs, and nothing streams from disk.
#include "aw.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

const u8 tileFlags[T_KINDS] = {
    [T_EMPTY] = 0,
    [T_ROCK]  = TF_SOLID | TF_OPAQUE,
    [T_LEDGE] = TF_ONEWAY,
    [T_VEIN]  = TF_SOLID | TF_OPAQUE | TF_EMIT,
    [T_MOSS]  = 0,
    [T_BULB]  = 0,
    [T_WATER] = TF_WATER,
    [T_BUSH]  = 0,
};

// '#' stone   '-' shelf   '~' water   '*' seam   ',' moss   'b' bush   'o' bulb
// 'm' the animal's home   'f' a talking plant   'P' start
static const char *MAPS[ROOM_COUNT][RH] = {
    { // 0: the chamber
        "########################################",
        "#...#.#.###..#..#...#.#*#...#..#..###..#",
        "#.......###............,..........###..#",
        "#........,........................###..#",
        "#..................-----...........,...#",
        "#..f.........-----.........-----.......#",
        "#####.------.....................#######",
        "#####............................#######",
        "#####.......................-----#######",
        "#.....----............................##",
        "#......................................#",
        "#.........................----.........#",
        "#..........----..................,.....#",
        "#.,...............###.........#.b.m..b.#",
        "#b.....,..........###.........#####*####",
        "####..----........###.........##########",
        "####..............###.----....##########",
        "#####.............###.........##########",
        "#####*............###.......o.##########",
        "######.bP...o..b,.###....,.##b##########",
        "#####################------#############",
        "#####################......#############",
    },
    { // 1: below it, flooded
        "####################*......#############",
        "#..##...#...,#.......------..,.##...#..#",
        "#......................................#",
        "#...............------....------.......#",
        "#......................................#",
        "#...b.,..-----.............-----b.f....#",
        "#..#####.......................#####...#",
        "#~~#####~~~~~~~~~~~~~~~~~~~~~~~#####~~~#",
        "#~~#####~~~~~~~~~~~~~~~~~~~~~~~#####~~~#",
        "#~~#####~~~~~~~~~~~~~~~~~~~~~~~#####~~~#",
        "#~~#####~~~~~~~~~~~~~~~~~~~~~~~#####~~~#",
        "#~~#####~~~~~~~~~~~~~~~~~~~~~~~#####~~~#",
        "#~~#####~~~~~~~~~~#*#~~~~~~~~~~#####~~~#",
        "#~~#####~~~~~~~~~~###~~~~~~~~~~#####~~~#",
        "#~~#####~~~~~~~~~~###~~~~~~~~~~#####~~~#",
        "#~~########~~~~~~~###~~~~~~~~~~#####~~~#",
        "#~~########~~~~~~~###~~~~~~~########~~~#",
        "#~~########~~~~~~~###~~~~~~~########~~~#",
        "#~~########~~~~~~~###~~~~~~~########~~~#",
        "#~~########~~~~~~~###~~~~~~~########~~~#",
        "#~~########~~~~~~~###~~~~~~~########~~~#",
        "##########################*#############",
    },
};

u8  tiles[RH][RW];
u8  roomTiles[ROOM_COUNT][RH][RW];
int  roomIdx;
static int markBeastX = -1, markBeastY = -1;
static int markPlantN, markPlantX[4], markPlantY[4];
int  RoomMarkBeast(int *tx, int *ty) { if (markBeastX < 0) return 0; *tx = markBeastX; *ty = markBeastY; return 1; }
int  RoomMarkPlants(int *txs, int *tys, int max) {
    int n = markPlantN < max ? markPlantN : max;
    for (int i = 0; i < n; i++) { txs[i] = markPlantX[i]; tys[i] = markPlantY[i]; }
    return n;
}
static int startTx = 8, startTy = 19;
Bulb bulbs[BULB_MAX];
int  bulbCount;

// The surface. One height and one velocity per column; nothing below the surface
// line moves, which is what keeps it readable at 8px.
static f32 surfH[RW], surfV[RW];

int RoomStartTx(void) { return startTx; }
int RoomStartTy(void) { return startTy; }

u8 TileAtPx(float px, float py) {
    return TileGet((int)floorf(px / TS), (int)floorf(py / TS));
}

// ---------------------------------------------------------------- light
// Light is baked once, by relaxation over the tile grid: a seam pushes into the
// open space beside it, that space pushes into its neighbours, and stone receives
// light but never passes it on. So the wall facing a seam glows and the tunnel
// behind it stays black, which is the whole reason the room reads as having depth.
#define LATT_O 0.796f      // attenuation per orthogonal step
#define LATT_D 0.706f      // per diagonal step
#define LPASS  48          // relaxation passes; the grid is 880 cells, this is free

static f32 lstat[RH][RW];              // baked, never changes
static f32 lnow[RH][RW];               // baked + whatever is moving
static Color lpix[(RH + 1) * (RW + 1)];   // ambient + seam, multiplied over the frame
static Color gpix[(RH + 1) * (RW + 1)];   // seam only, added back on top
static Texture2D lightTex, glowTex;

// Cool where nothing reaches, and a shade less cool near the ceiling, so the room
// feels like it is under something rather than sealed inside it.
static const f32 AMB_R = 0.118f, AMB_G = 0.130f, AMB_B = 0.222f;
static const f32 WARM_R = 1.00f, WARM_G = 0.815f, WARM_B = 0.560f;
#define GLOW 0.38f
#define GLOW_CAP 0.60f

static int Opaque(int x, int y) { return (tileFlags[TileGet(x, y)] & TF_OPAQUE) != 0; }

static void LightBake(void) {
    memset(lstat, 0, sizeof lstat);
    // A seam lights the open space around it, not itself: light starts where air is.
    for (int y = 0; y < RH; y++)
        for (int x = 0; x < RW; x++) {
            if (!(tileFlags[tiles[y][x]] & TF_EMIT)) continue;
            for (int dy = -1; dy <= 1; dy++)
                for (int dx = -1; dx <= 1; dx++) {
                    int nx = x + dx, ny = y + dy;
                    if (nx < 0 || nx >= RW || ny < 0 || ny >= RH) continue;
                    if (Opaque(nx, ny)) continue;
                    if (lstat[ny][nx] < 1.0f) lstat[ny][nx] = 1.0f;
                }
        }
    for (int i = 0; i < bulbCount; i++) {
        int bx = bulbs[i].x / TS, by = (bulbs[i].y - 1) / TS;
        if (bx >= 0 && bx < RW && by >= 0 && by < RH && !Opaque(bx, by) && lstat[by][bx] < 0.42f)
            lstat[by][bx] = 0.42f;
    }
    for (int y = 0; y < RH; y++)
        for (int x = 0; x < RW; x++) {
            if (!(tileFlags[tiles[y][x]] & TF_EMIT)) continue;
            int lit = 0;
            for (int dy = -1; dy <= 1; dy++)
                for (int dx = -1; dx <= 1; dx++) {
                    int nx = x + dx, ny = y + dy;
                    if (nx < 0 || nx >= RW || ny < 0 || ny >= RH) continue;
                    if (!Opaque(nx, ny)) lit = 1;
                }
            // A seam with stone on all six sides lights nothing at all, and looks
            // from the outside exactly like a seam that does. Say so.
            if (!lit) TraceLog(LOG_WARNING, "seam at %d,%d is walled in", x, y);
        }
    for (int p = 0; p < LPASS; p++) {
        // Alternating scan direction so a value can travel the width of the room in
        // far fewer passes than it would crawling one cell at a time.
        int rev = p & 1;
        for (int i = 0; i < RH; i++) {
            int y = rev ? RH - 1 - i : i;
            for (int j = 0; j < RW; j++) {
                int x = rev ? RW - 1 - j : j;
                if (Opaque(x, y)) continue;
                f32 best = lstat[y][x];
                for (int dy = -1; dy <= 1; dy++)
                    for (int dx = -1; dx <= 1; dx++) {
                        if (!dx && !dy) continue;
                        int nx = x + dx, ny = y + dy;
                        if (nx < 0 || nx >= RW || ny < 0 || ny >= RH) continue;
                        if (Opaque(nx, ny)) continue;
                        f32 c = lstat[ny][nx] * ((dx && dy) ? LATT_D : LATT_O);
                        if (TileWater(tiles[y][x])) c *= 0.90f;   // light dies faster under
                        if (c > best) best = c;
                    }
                lstat[y][x] = best;
            }
        }
    }
    // Now let stone take the light off the air beside it. This is the step that
    // draws the shape of the room -- an edge lit from one side and dark on the other.
    f32 face[RH][RW];
    memset(face, 0, sizeof face);
    for (int y = 0; y < RH; y++)
        for (int x = 0; x < RW; x++) {
            if (!Opaque(x, y)) continue;
            f32 best = 0.0f;
            for (int dy = -1; dy <= 1; dy++)
                for (int dx = -1; dx <= 1; dx++) {
                    int nx = x + dx, ny = y + dy;
                    if (nx < 0 || nx >= RW || ny < 0 || ny >= RH) continue;
                    if (Opaque(nx, ny)) continue;
                    f32 c = lstat[ny][nx] * ((dx && dy) ? 0.62f : 0.80f);
                    if (c > best) best = c;
                }
            if (tileFlags[tiles[y][x]] & TF_EMIT) best = 1.0f;
            face[y][x] = best;
        }
    for (int y = 0; y < RH; y++)
        for (int x = 0; x < RW; x++)
            if (Opaque(x, y)) lstat[y][x] = face[y][x];
}

// The body carries a little light of its own -- enough to find yourself by, not
// enough to see the room with. Occluded properly, or it shines through walls.
static void AddPoint(f32 px, f32 py, f32 R, f32 PEAK) {
    f32 cx = px / TS, cy = py / TS;
    int x0 = (int)(cx - R) - 1, x1 = (int)(cx + R) + 1;
    int y0 = (int)(cy - R) - 1, y1 = (int)(cy + R) + 1;
    if (x0 < 0) x0 = 0;
    if (x1 >= RW) x1 = RW - 1;
    if (y0 < 0) y0 = 0;
    if (y1 >= RH) y1 = RH - 1;
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) {
            f32 dx = (x + 0.5f) - cx, dy = (y + 0.5f) - cy;
            f32 d = sqrtf(dx * dx + dy * dy);
            if (d > R) continue;
            f32 v = PEAK * (1.0f - d / R) * (1.0f - d / R);
            // walk the line back to the body; stop at the first stone in the way
            int steps = (int)(d * 2.0f) + 1;
            for (int s = 1; s < steps; s++) {
                f32 t = (f32)s / (f32)steps;
                int sx = (int)(cx + dx * (1.0f - t));
                int sy = (int)(cy + dy * (1.0f - t));
                if (Opaque(sx, sy) && !(sx == x && sy == y)) { v = 0.0f; break; }
            }
            if (v > 0.0f) lnow[y][x] += v;
        }
}

static void AddAura(void) {
    AddPoint(player.x + player.w * 0.5f, player.y + player.h * 0.5f, 4.6f, 0.42f);
}
void LightAddPoint(f32 px, f32 py, f32 R, f32 peak) { AddPoint(px, py, R, peak); }

void LightStep(void) {
    memcpy(lnow, lstat, sizeof lnow);
    AddAura();
    LifeLights();
    LampLight();
    // A bulb that has just been landed on throws light for a moment; more, and further,
    // when the landing was timed. That is the only tell there is, and it is enough.
    for (int i = 0; i < bulbCount; i++)
        if (bulbs[i].flash > 0) {
            f32 t = bulbs[i].flash / (bulbs[i].timed ? 26.0f : 22.0f);
            AddPoint((f32)bulbs[i].x, (f32)bulbs[i].y - 3.0f,
                     bulbs[i].timed ? 5.4f : 4.2f, t * (bulbs[i].timed ? 0.90f : 0.55f));
        }
    // The grid is sampled at tile CORNERS: (RW+1) x (RH+1) values, drawn back over
    // the room half a tile out on every side so each texel centre lands exactly on
    // its corner. Bilinear does the rest, and the falloff comes out smooth.
    for (int j = 0; j <= RH; j++) {
        for (int i = 0; i <= RW; i++) {
            f32 acc = 0.0f;
            int n = 0;
            for (int dy = -1; dy <= 0; dy++)
                for (int dx = -1; dx <= 0; dx++) {
                    int x = i + dx, y = j + dy;
                    if (x < 0 || x >= RW || y < 0 || y >= RH) continue;
                    acc += lnow[y][x]; n++;
                }
            f32 v = n ? acc / n : 0.0f;
            int wn = 0;
            for (int dy = -1; dy <= 0; dy++)
                for (int dx = -1; dx <= 0; dx++) {
                    int x = i + dx, y = j + dy;
                    if (x >= 0 && x < RW && y >= 0 && y < RH && TileWater(tiles[y][x])) wn++;
                }
            f32 wf = n ? (f32)wn / n : 0.0f;          // how much of this corner is under
            f32 h = 1.0f - (f32)j / (f32)RH;          // a shade more sky near the ceiling
            f32 w = powf(v, 1.55f);
            f32 r = AMB_R * (0.88f + 0.26f * h) + w * WARM_R;
            f32 g = AMB_G * (0.88f + 0.24f * h) + w * WARM_G;
            f32 b = AMB_B * (0.92f + 0.22f * h) + w * WARM_B;
            r *= 1.0f - 0.40f * wf;                   // under the water everything goes cold
            g *= 1.0f - 0.10f * wf;
            if (r > 1.0f) r = 1.0f;
            if (g > 1.0f) g = 1.0f;
            if (b > 1.0f) b = 1.0f;
            lpix[j * (RW + 1) + i] = (Color){ (u8)(r * 255), (u8)(g * 255), (u8)(b * 255), 255 };
            // The additive half. Squared, so it stays off everywhere except close in.
            // Capped: past this a lamp in hand blew every channel past 255 and the cast
            // wrapped, red first -- a cyan blotch where the light was strongest.
            f32 q = v * v * GLOW;
            if (q > GLOW_CAP) q = GLOW_CAP;
            gpix[j * (RW + 1) + i] = (Color){ (u8)(q * WARM_R * 255), (u8)(q * WARM_G * 255),
                                              (u8)(q * WARM_B * 255), 255 };
        }
    }
    UpdateTexture(lightTex, lpix);
    UpdateTexture(glowTex, gpix);
}

void LightDraw(void) {
    Rectangle src = { 0, 0, (f32)(RW + 1), (f32)(RH + 1) };
    Rectangle dst = { -TS * 0.5f, ROOM_Y - TS * 0.5f,
                      (f32)((RW + 1) * TS), (f32)((RH + 1) * TS) };
    BeginBlendMode(BLEND_MULTIPLIED);
        DrawTexturePro(lightTex, src, dst, (Vector2){ 0, 0 }, 0.0f, WHITE);
    EndBlendMode();
    BeginBlendMode(BLEND_ADDITIVE);
        DrawTexturePro(glowTex, src, dst, (Vector2){ 0, 0 }, 0.0f, WHITE);
    EndBlendMode();
}

// ---------------------------------------------------------------- load
static void ParseRoom(int idx, u8 dst[RH][RW]) {
    bulbCount = 0; markBeastX = markBeastY = -1; markPlantN = 0;
    for (int y = 0; y < RH; y++) {
        for (int x = 0; x < RW; x++) {
            char c = MAPS[idx][y][x];
            u8 t = T_EMPTY;
            switch (c) {
                case '#': t = T_ROCK;  break;
                case '-': t = T_LEDGE; break;
                case '*': t = T_VEIN;  break;
                case ',': t = T_MOSS;  break;
                case '~': t = T_WATER; break;
                case 'b': t = T_BUSH;  break;
                case 'm': markBeastX = x; markBeastY = y; break;
                case 'f': if (markPlantN < 4) { markPlantX[markPlantN] = x; markPlantY[markPlantN] = y; markPlantN++; } break;
                case 'o':
                    if (bulbCount < BULB_MAX) {
                        bulbs[bulbCount].x = x * TS + TS / 2;
                        bulbs[bulbCount].y = (y + 1) * TS;       // base on the tile floor
                        bulbs[bulbCount].squash = bulbs[bulbCount].flash = bulbs[bulbCount].timed = 0;
                        bulbCount++;
                    }
                    break;
                case 'P': startTx = x; startTy = y; break;
                default: break;
            }
            dst[y][x] = t;
        }
    }
}

// Entering a room rebuilds everything that belongs to it: tiles, bulbs, the baked
// light, the surface, the specks. Nothing carries over except you.
static void FindSurfaces(void);

void RoomEnter(int idx) {
    roomIdx = idx;
    ParseRoom(idx, tiles);          // sets the bulbs for this room too
    FindSurfaces();
    LightBake();
    memset(surfH, 0, sizeof surfH);
    memset(surfV, 0, sizeof surfV);
    FxInit();
    LifeInit();
    AudioAmbience(idx);          // a no-op until the device is up
}

void RoomLoad(void) {
    // A map row that is one character short reads its last column as the string
    // terminator and quietly opens a hole in the wall. Editing these strings by
    // hand did exactly that once, and nothing downstream noticed.
    for (int r = 0; r < ROOM_COUNT; r++)
        for (int y = 0; y < RH; y++) {
            int n = 0;
            while (MAPS[r][y][n]) n++;
            if (n != RW) TraceLog(LOG_ERROR, "room %d row %d is %d wide, expected %d", r, y, n, RW);
        }
    Image im = GenImageColor(RW + 1, RH + 1, WHITE);
    lightTex = LoadTextureFromImage(im);
    glowTex  = LoadTextureFromImage(im);
    UnloadImage(im);
    SetTextureFilter(lightTex, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(lightTex, TEXTURE_WRAP_CLAMP);
    SetTextureFilter(glowTex, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(glowTex, TEXTURE_WRAP_CLAMP);
    for (int r = 0; r < ROOM_COUNT; r++) ParseRoom(r, roomTiles[r]);   // room 0 last finds P
    ParseRoom(0, roomTiles[0]);
    RoomEnter(0);
}

// The body has crossed the top or the bottom. Positions are continuous across the seam
// (y shifts by exactly one room), so nothing about the motion changes but the walls
// around it -- and since TileGet already sees the next room's tiles, nothing about the
// collision changes either.
//
// Down switches when the CENTRE crosses: a body that lands on a shelf just inside the
// lower room, head still through the seam, must be shown in the room its feet are in.
// Up switches only when the centre is UP_MARGIN past the seam: a jump that merely pokes
// into the room above and comes back down -- every plain jump from A2 to A1 does --
// would otherwise cut the camera to the upper room for a few frames and back.
#define UP_MARGIN 16.0f
int RoomTransition(void) {
    float cy = player.y + player.h * 0.5f;
    if (cy >= RH * TS && roomIdx < ROOM_COUNT - 1) {
        RoomEnter(roomIdx + 1);
        player.y -= RH * TS;
        return 1;
    }
    if (cy < -UP_MARGIN && roomIdx > 0) {
        RoomEnter(roomIdx - 1);
        player.y += RH * TS;
        return 1;
    }
    return 0;
}

// ---------------------------------------------------------------- water
void WaterDisturb(float px, float strength) {
    int c = (int)floorf(px / TS);
    if (c < 0 || c >= RW) return;
    surfV[c] += strength;
    if (c > 0)      surfV[c - 1] += strength * 0.5f;
    if (c < RW - 1) surfV[c + 1] += strength * 0.5f;
}

void WaterStep(void) {
    for (int x = 0; x < RW; x++) {
        f32 l = x > 0      ? surfH[x - 1] : surfH[x];
        f32 r = x < RW - 1 ? surfH[x + 1] : surfH[x];
        surfV[x] += ((l + r) * 0.5f - surfH[x]) * 0.24f - surfH[x] * 0.045f;
        surfV[x] *= 0.955f;
    }
    for (int x = 0; x < RW; x++) surfH[x] += surfV[x];
}

// ---------------------------------------------------------------- drawing
// Stone is drawn flat and lit afterwards by the pass above. Nothing here bakes a
// light direction into a tile, so a wall looks different depending on where in the
// room it is -- which is most of why the place reads as a place.
static int Massive(int x, int y) {
    u8 t = TileGet(x, y);
    return t == T_ROCK || t == T_VEIN;
}

static void DrawStone(int x, int y, int px, int py) {
    int up = Massive(x, y - 1), dn = Massive(x, y + 1);
    int lf = Massive(x - 1, y), rt = Massive(x + 1, y);
    int buried = up && dn && lf && rt;

    DrawRectangle(px, py, TS, TS, buried ? palRockDeep : palRock);

    // Grain. Same seed, same room, every run: a screenshot is a fact.
    u32 h = Hash2(x, y);
    for (int k = 0; k < 3; k++) {
        int gx = (h >> (k * 7)) & 7, gy = (h >> (k * 7 + 3)) & 7;
        Color c = ((h >> (k * 7 + 6)) & 1) ? palRockDeep : palRockLit;
        if (c.r == palRockLit.r) c = (Color){ 74, 71, 92, 255 };
        DrawRectangle(px + gx, py + gy, 1, 1, c);
    }

    if (!up) {                                   // the surface catches everything
        DrawRectangle(px, py, TS, 1, palRockLit);
        DrawRectangle(px, py + 1, TS, 1, palRock);
    }
    if (!dn) DrawRectangle(px, py + TS - 1, TS, 1, palRockDeep);

    // Knock the exposed corners off. Collision stays square -- this is the only
    // reason a room of rectangles does not look like a room of rectangles.
    if (!up && !lf) DrawRectangle(px, py, 1, 1, palBack);
    if (!up && !rt) DrawRectangle(px + TS - 1, py, 1, 1, palBack);
    if (!dn && !lf) DrawRectangle(px, py + TS - 1, 1, 1, palBack);
    if (!dn && !rt) DrawRectangle(px + TS - 1, py + TS - 1, 1, 1, palBack);
}

static void DrawVein(int x, int y, int px, int py) {
    DrawStone(x, y, px, py);
    // A seam running through the stone. Its shape is hashed from where it is, so
    // no two look alike and all of them look like the same mineral.
    u32 h = Hash2(x * 7 + 1, y * 13 + 3);
    int sx = 2 + (h & 3);
    for (int i = 0; i < 6; i++) {
        int yy = py + 1 + i;
        int xx = px + sx + (int)(((h >> (i * 3)) & 3) - 1);
        if (xx < px + 1) xx = px + 1;
        if (xx > px + TS - 2) xx = px + TS - 2;
        DrawRectangle(xx, yy, 1, 1, palVeinHot);
        if ((h >> (i + 12)) & 1) DrawRectangle(xx + 1, yy, 1, 1, palVein);
        else                     DrawRectangle(xx - 1, yy, 1, 1, palVein);
    }
}

static void DrawLedge(int x, int y, int px, int py) {
    int lf = TileGet(x - 1, y) == T_LEDGE, rt = TileGet(x + 1, y) == T_LEDGE;
    // Three pixels of shelf, and nothing at all below it. A one-way surface has to
    // look like a thing you land on top of, or landing on top of it is a surprise.
    DrawRectangle(px, py, TS, 3, palLedge);
    DrawRectangle(px, py, TS, 1, palLedgeLit);
    DrawRectangle(px, py + 2, TS, 1, (Color){ 62, 54, 46, 255 });
    u32 h = Hash2(x * 3, y * 5 + 11);
    if (h & 1) DrawRectangle(px + 2 + (h >> 1 & 3), py + 1, 1, 1, (Color){ 68, 60, 50, 255 });
    // pegs, so you can see the shelf is held up rather than resting on something
    if (((x + y) & 1) == 0) DrawRectangle(px + 3, py + 3, 1, 2, (Color){ 62, 54, 46, 255 });
    if (!lf) { DrawRectangle(px, py, 1, 1, palBack); DrawRectangle(px, py + 2, 1, 1, palBack); }
    if (!rt) { DrawRectangle(px + TS - 1, py, 1, 1, palBack);
               DrawRectangle(px + TS - 1, py + 2, 1, 1, palBack); }
}

static void DrawWater(int x, int y, int px, int py) {
    int surface = !TileWater(TileGet(x, y - 1));
    if (surface) {
        // The wave displaces the surface line only. The body stays put.
        int d = (int)(surfH[x] * 3.0f);
        if (d >  3) d =  3;
        if (d < -3) d = -3;
        DrawRectangle(px, py + d, TS, TS - d, palWater);
        DrawRectangle(px, py + d, TS, 1, palWaterLit);
    } else {
        DrawRectangle(px, py, TS, TS, palWater);
        // a fleck drifting up now and then: the water is not still
        if (((x * 3 + y * 7 + (int)(frameNo / 26)) % 11) == 0)
            DrawRectangle(px + 2 + (x & 3), py + 3, 1, 1, palWaterFleck);
    }
}

static void DrawBush(int x, int y, int px, int py) {
    // A clump on whatever it grows from. Leans away from a body passing, shakes after.
    float dx = (player.x + player.w * 0.5f) - (px + 4.0f);
    int lean = 0;
    if (dx > -12.0f && dx < 12.0f) lean = (dx > 0) ? -1 : 1;
    int shake = bushShake[y][x] ? (((bushShake[y][x] / 2) & 1) ? 1 : -1) : 0;
    int l = lean + shake;
    u32 h = Hash2(x * 5 + 3, y * 9 + 1);
    DrawRectangle(px, py + 5, TS, 3, palBush);
    DrawRectangle(px + 1 + l, py + 3, 6, 2, palBush);
    DrawRectangle(px + 2 + l, py + 2, 4, 1, palBush);
    DrawRectangle(px + 3 + l + (h & 1), py + 1, 2, 1, palBush);
    DrawRectangle(px + 2 + l, py + 3, 1, 1, palBushLit);
    DrawRectangle(px + 5 + l, py + 2, 1, 1, palBushLit);
    DrawRectangle(px + 1, py + 5, 1, 1, palBushLit);
    DrawRectangle(px + 1 + (h >> 2 & 3) + l, py + 4, 1, 1, palBerry);
    DrawRectangle(px + 3 + (h >> 4 & 3), py + 6, 1, 1, palBerry);
}

static void DrawMoss(int x, int y, int px, int py) {
    u32 h = Hash2(x * 11 + 5, y * 17);
    int down = Massive(x, y - 1) || TileGet(x, y - 1) == T_LEDGE;
    for (int i = 0; i < 4; i++) {
        int gx = px + 1 + ((h >> (i * 4)) & 5);
        int len = 2 + ((h >> (i * 4 + 3)) & 3);
        int gy = down ? py : py + TS - len;
        DrawRectangle(gx, gy, 1, len, palMoss);
        DrawRectangle(gx, down ? gy + len - 1 : gy, 1, 1, (Color){ 96, 122, 92, 255 });
    }
}

void RoomDraw(void) {
    // The far wall. Coursed stone, barely a shade off the dark -- invisible until
    // something lights it, which is the point: you learn the room's depth by
    // carrying light into it, not by being shown a diagram of it.
    DrawRectangle(0, ROOM_Y, GW, RH * TS, palBack);
    for (int y = 0; y < RH * TS; y += 16) {
        DrawRectangle(0, ROOM_Y + y, GW, 1, palBackLit);
        int off = ((y / 16) & 1) ? 8 : 0;
        for (int x = off; x < GW; x += 16)
            DrawRectangle(x, ROOM_Y + y, 1, 16, palBackLit);
    }
    for (int y = 0; y < RH; y++)
        for (int x = 0; x < RW; x++) {
            if (tiles[y][x] != T_EMPTY) continue;
            u32 h = Hash2(x + 91, y + 17);
            if ((h & 7) == 0)
                DrawRectangle(x * TS + (h >> 3 & 7), ROOM_Y + y * TS + (h >> 6 & 7), 1, 1,
                              (Color){ 30, 29, 44, 255 });
        }

    for (int y = 0; y < RH; y++) {
        for (int x = 0; x < RW; x++) {
            int px = x * TS, py = ROOM_Y + y * TS;
            switch (tiles[y][x]) {
                case T_ROCK:  DrawStone(x, y, px, py); break;
                case T_VEIN:  DrawVein(x, y, px, py);  break;
                case T_LEDGE: DrawLedge(x, y, px, py); break;
                case T_WATER: DrawWater(x, y, px, py); break;
                case T_BUSH:  DrawBush(x, y, px, py);  break;
                case T_MOSS:  DrawMoss(x, y, px, py);  break;
                default: break;
            }
        }
    }
}

// ---------------------------------------------------------------- bulbs
int BulbCrossed(float oldBottom, float newBottom, float x, int w) {
    for (int i = 0; i < bulbCount; i++) {
        float top = (float)(bulbs[i].y - BULB_H);
        // Same crossing test as a shelf: the feet pass through the dome's top edge
        // this step, and the body overlaps the dome's width.
        if (oldBottom <= top + 0.001f && newBottom > top
            && x < bulbs[i].x + BULB_W / 2 && x + w > bulbs[i].x - BULB_W / 2)
            return i;
    }
    return -1;
}

void BulbsStep(void) {
    for (int i = 0; i < bulbCount; i++) {
        if (bulbs[i].squash > 0) bulbs[i].squash--;
        if (bulbs[i].flash  > 0) bulbs[i].flash--;
    }
}

// A dome. Drawn as rows of an ellipse so it can be pressed flatter for a few frames
// after a landing -- the pad deforms, the body never does.
void BulbsDraw(void) {
    for (int i = 0; i < bulbCount; i++) {
        Bulb *b = &bulbs[i];
        int press = b->squash > 0 ? (b->squash > 5 ? 2 : 1) : 0;
        int H = BULB_H - press;
        int hw = BULB_W / 2 + press;
        int base = ROOM_Y + b->y;
        for (int r = 0; r < H; r++) {
            float h = (r + 0.5f) / (float)H;            // 0 at base .. 1 at crown
            float q = 1.0f - h * h;
            int half = (int)(hw * sqrtf(q > 0 ? q : 0) + 0.5f);
            if (half < 1) half = 1;
            int y = base - 1 - r;
            Color c = palBulb;
            if (r == H - 1) c = palBulbLit;             // the crown catches everything
            if (r == 0)     c = palBulbDeep;
            DrawRectangle(b->x - half, y, half * 2, 1, c);
        }
        // a lit fleck near the crown, offset to one side: it is round, not flat
        DrawRectangle(b->x - 2, base - H + 1, 2, 1, palBulbLit);
        DrawRectangle(b->x + 1, base - 2, 1, 1, palBulbDeep);
    }
}

// ---------------------------------------------------------------- debug labels
// Every maximal horizontal run of standable tiles with something open above it, in
// reading order, tagged letter+digit: A1..A9, B1..B9, ... Letters that look like
// digits at 3x5 (I, O, S, Z) are skipped. Bulbs are ^1, ^2 (the ^ is drawn as a dome). This is for talking about
// the room -- "B3 is too far from B4" -- and it is off unless asked for.
#define SURF_MAX 200
typedef struct { i32 x0, x1, y; u8 shelf; char tag[3]; } Surf;
static Surf surfs[SURF_MAX];
static int  surfCount;
static const char TAG_LETTERS[] = "ABCDEFGHJKLMNPQRTUVWXY";

int  SurfCount(void) { return surfCount; }
void SurfGet(int i, int *x0, int *x1, int *y, int *shelf) {
    *x0 = surfs[i].x0; *x1 = surfs[i].x1; *y = surfs[i].y; *shelf = surfs[i].shelf;
}

static void FindSurfaces(void) {
    surfCount = 0;
    int n = 0;
    for (int y = 1; y < RH; y++) {
        int x = 1;
        while (x < RW - 1 && surfCount < SURF_MAX) {
            u8 t = tiles[y][x], up = tiles[y - 1][x];
            int standable = (TileSolid(t) || TileOneWay(t))
                         && (up == T_EMPTY || up == T_MOSS || up == T_WATER || up == T_BUSH);
            if (!standable) { x++; continue; }
            int shelf = TileOneWay(t), x0 = x;
            while (x < RW - 1) {
                u8 t2 = tiles[y][x], up2 = tiles[y - 1][x];
                int ok = (TileSolid(t2) || TileOneWay(t2)) && TileOneWay(t2) == shelf
                      && (up2 == T_EMPTY || up2 == T_MOSS || up2 == T_WATER || up2 == T_BUSH);
                if (!ok) break;
                x++;
            }
            Surf *s = &surfs[surfCount++];
            s->x0 = x0; s->x1 = x - 1; s->y = y; s->shelf = (u8)shelf;
            s->tag[0] = TAG_LETTERS[(n / 9) % (int)(sizeof TAG_LETTERS - 1)];
            s->tag[1] = (char)('1' + n % 9);
            s->tag[2] = 0;
            n++;
        }
    }
}

// A 3x5 font: digits, the letters used above, and a dome for the bulbs.
static const u16 FONT[] = {
    /* 0 */ 0x7B6F, /* 1 */ 0x2C97, /* 2 */ 0x73E7, /* 3 */ 0x79E7, /* 4 */ 0x5BC9,
    /* 5 */ 0x79CF, /* 6 */ 0x7BCF, /* 7 */ 0x4927, /* 8 */ 0x7BEF, /* 9 */ 0x79EF,
};
static u16 Glyph(char c) {
    // rows top to bottom, 3 bits each, MSB first: value = r0<<12 | r1<<9 | r2<<6 | r3<<3 | r4
    #define G(a,b,c,d,e) ((u16)((a)<<12 | (b)<<9 | (c)<<6 | (d)<<3 | (e)))
    switch (c) {
        case '0': return G(7,5,5,5,7); case '1': return G(2,6,2,2,7); case '2': return G(7,1,7,4,7);
        case '3': return G(7,1,7,1,7); case '4': return G(5,5,7,1,1); case '5': return G(7,4,7,1,7);
        case '6': return G(7,4,7,5,7); case '7': return G(7,1,1,1,1); case '8': return G(7,5,7,5,7);
        case '9': return G(7,5,7,1,7);
        case 'A': return G(2,5,7,5,5); case 'B': return G(6,5,6,5,6); case 'C': return G(7,4,4,4,7);
        case 'D': return G(6,5,5,5,6); case 'E': return G(7,4,7,4,7); case 'F': return G(7,4,7,4,4);
        case 'G': return G(7,4,5,5,7); case 'H': return G(5,5,7,5,5); case 'J': return G(3,1,1,5,7);
        case 'K': return G(5,5,6,5,5); case 'L': return G(4,4,4,4,7); case 'M': return G(5,7,7,5,5);
        case 'N': return G(6,5,5,5,5); case 'P': return G(7,5,7,4,4); case 'Q': return G(7,5,5,7,1);
        case 'R': return G(6,5,6,5,5); case 'T': return G(7,2,2,2,2); case 'U': return G(5,5,5,5,7);
        case 'V': return G(5,5,5,5,2); case 'W': return G(5,5,7,7,5); case 'X': return G(5,5,2,5,5);
        case 'Y': return G(5,5,2,2,2); case '^': return G(0,2,7,7,7);   // a dome: the bulb
        default:  return 0;
    }
    #undef G
}
static void Tag(int x, int y, const char *s, Color col) {
    int n = 0; while (s[n]) n++;
    DrawRectangle(x - 1, y - 1, n * 4 + 1, 7, (Color){ 0, 0, 0, 190 });
    for (int i = 0; s[i]; i++) {
        u16 g = Glyph(s[i]);
        for (int r = 0; r < 5; r++) {
            int bits = (g >> (12 - r * 3)) & 7;
            if (bits & 4) DrawRectangle(x + i * 4,     y + r, 1, 1, col);
            if (bits & 2) DrawRectangle(x + i * 4 + 1, y + r, 1, 1, col);
            if (bits & 1) DrawRectangle(x + i * 4 + 2, y + r, 1, 1, col);
        }
    }
}

void DebugLabelsDraw(void) {
    if (!dbgLabels) return;
    (void)FONT;
    for (int i = 0; i < surfCount; i++) {
        Surf *s = &surfs[i];
        int px = s->x0 * TS + 1, py = ROOM_Y + s->y * TS - 7;
        if (py < ROOM_Y) py = ROOM_Y + s->y * TS + 1;     // no room above: sit on it
        Tag(px, py, s->tag, s->shelf ? (Color){ 255, 220, 150, 255 } : (Color){ 235, 235, 245, 255 });
    }
    for (int i = 0; i < bulbCount; i++) {
        char t[3] = { '^', (char)('1' + i), 0 };
        Tag(bulbs[i].x - 4, ROOM_Y + bulbs[i].y - BULB_H - 8, t, palBulbLit);
    }
    char r[3] = { 'R', (char)('0' + roomIdx), 0 };
    Tag(3, ROOM_Y + 2, r, (Color){ 160, 200, 255, 255 });
}

// For the console: the same table, so a screenshot and a transcript can agree.
void DebugLabelsPrint(void) {
    for (int i = 0; i < surfCount; i++)
        printf("R%d %s %-5s row %2d cols %2d-%2d\n", roomIdx, surfs[i].tag,
               surfs[i].shelf ? "shelf" : "stone", surfs[i].y, surfs[i].x0, surfs[i].x1);
    for (int i = 0; i < bulbCount; i++)
        printf("R%d ^%d bulb  at col %d, base row %d\n", roomIdx, i + 1, bulbs[i].x / TS, bulbs[i].y / TS - 1);
}
