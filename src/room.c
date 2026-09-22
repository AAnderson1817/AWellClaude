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

// The map is in antechamber.c. Its letters:
// '#' stone   '-' shelf   '~' water   '*' seam   ',' moss   'b' bush   'o' bulb
// 'm' the animal's home   'f' a talking plant   's' a stone   'P' start
// 'X' carved stone and 'x' a carved shelf: a backdrop piece draws them   'k' a dead seam
u8  tiles[RH][RW];
u8  roomTiles[ROOM_COUNT][RH][RW];
u8  tileDeco[RH][RW];
int  roomIdx;

// The city, as rectangles of tiles (ROOM_CITY, in antechamber.c); everything else is the
// vault. Resolved into a grid once, since the room asks per tile, per frame.
static u8 zone[RH][RW];
static void ZonesBuild(void) {
    memset(zone, Z_VAULT, sizeof zone);
    for (int i = 0; ROOM_CITY[i].x0 >= 0; i++) {
        const ZRect *r = &ROOM_CITY[i];
        for (int y = r->y0; y <= r->y1; y++)
            for (int x = r->x0; x <= r->x1; x++)
                if (x >= 0 && x < RW && y >= 0 && y < RH) zone[y][x] = Z_CITY;
    }
}
int ZoneAt(int tx, int ty) {
    if (tx < 0) tx = 0;
    if (tx >= RW) tx = RW - 1;
    if (ty < 0) ty = 0;
    if (ty >= RH) ty = RH - 1;
    return zone[ty][tx];
}
static int markBeastX = -1, markBeastY = -1;
static int markPlantN, markPlantX[4], markPlantY[4];
static int markStoneN, markStoneX[4], markStoneY[4];
int  RoomMarkStones(int *txs, int *tys, int max) {
    int n = markStoneN < max ? markStoneN : max;
    for (int i = 0; i < n; i++) { txs[i] = markStoneX[i]; tys[i] = markStoneY[i]; }
    return n;
}
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
#define LATT_O 0.835f      // attenuation per orthogonal step (0.796 before the lighting pass: too short a reach)
#define LATT_D 0.748f      // per diagonal step
#define LPASS  48          // relaxation passes; the grid is 5280 cells, done once

// Two bakes, two colours. Warm is flame and flame is the hunters': the seams in raw rock,
// your lamp, the fire. Cool is the city's own light: its glass, the face, the native. You
// learn who made a thing by what colour it gives off, so the two never mix in the bake.
static f32 lstatW[RH][RW], lstatC[RH][RW];   // baked, never changes
static f32 lnowW[RH][RW],  lnowC[RH][RW];    // baked + whatever is moving
static Color lpix[(RH + 1) * (RW + 1)];   // ambient + light, multiplied over the frame
static Color gpix[(RH + 1) * (RW + 1)];   // light only, added back on top
static Texture2D lightTex, glowTex;
// The new look reads the bake once per room: warm and cool at tile corners, the water, and
// in the alpha, what each tile is (stone, shelf, air) for the rim and the shadows.
static Color bpix[(RH + 1) * (RW + 1)];
static Texture2D bakeTex;
Texture2D LightBakeTexture(void) { return bakeTex; }

// This frame's moving lights, for the composite: the new look draws them per pixel.
#define PT_MAX 16
static struct { f32 x, y, R, peak; int cool; } pts[PT_MAX];
static int ptCount;
int LightPoints(float *pos4, float *col4, int max) {
    int n = ptCount < max ? ptCount : max;
    for (int i = 0; i < max; i++) {
        pos4[i*4] = i < n ? pts[i].x : 0; pos4[i*4+1] = i < n ? pts[i].y : 0;
        pos4[i*4+2] = i < n ? pts[i].R * TS : 0; pos4[i*4+3] = i < n ? pts[i].peak : 0;
        col4[i*4] = i < n ? (f32)pts[i].cool : 0; col4[i*4+1] = col4[i*4+2] = col4[i*4+3] = 0;
    }
    return n;
}

// Cool where nothing reaches, and a shade less cool near the ceiling, so the room
// feels like it is under something rather than sealed inside it.
static const f32 AMB_R = 0.118f, AMB_G = 0.130f, AMB_B = 0.222f;
static const f32 WARM_R = 1.00f, WARM_G = 0.815f, WARM_B = 0.560f;
static const f32 COOL_R = 0.60f, COOL_G = 0.96f, COOL_B = 0.76f;
#define GLOW 0.38f
#define GLOW_CAP 0.60f

static int Opaque(int x, int y) { return (tileFlags[TileGet(x, y)] & TF_OPAQUE) != 0; }

// Relaxation, then the faces: stone takes the light off the air beside it. This is the
// step that draws the shape of the room -- an edge lit from one side and dark on the other.
static void Relax(f32 l[RH][RW]) {
    for (int p = 0; p < LPASS; p++) {
        // Alternating scan direction so a value can travel the width of the room in
        // far fewer passes than it would crawling one cell at a time.
        int rev = p & 1;
        for (int i = 0; i < RH; i++) {
            int y = rev ? RH - 1 - i : i;
            for (int j = 0; j < RW; j++) {
                int x = rev ? RW - 1 - j : j;
                if (Opaque(x, y)) continue;
                f32 best = l[y][x];
                for (int dy = -1; dy <= 1; dy++)
                    for (int dx = -1; dx <= 1; dx++) {
                        if (!dx && !dy) continue;
                        int nx = x + dx, ny = y + dy;
                        if (nx < 0 || nx >= RW || ny < 0 || ny >= RH) continue;
                        if (Opaque(nx, ny)) continue;
                        f32 c = l[ny][nx] * ((dx && dy) ? LATT_D : LATT_O);
                        if (TileWater(tiles[y][x])) c *= 0.78f;   // light dies fast under
                        if (c > best) best = c;
                    }
                l[y][x] = best;
            }
        }
    }
    static f32 face[RH][RW];
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
                    f32 c = l[ny][nx] * ((dx && dy) ? 0.62f : 0.80f);
                    if (c > best) best = c;
                }
            face[y][x] = best;
        }
    for (int y = 0; y < RH; y++)
        for (int x = 0; x < RW; x++)
            if (Opaque(x, y)) l[y][x] = face[y][x];
}

static void LightBake(void) {
    memset(lstatW, 0, sizeof lstatW);
    memset(lstatC, 0, sizeof lstatC);
    // A seam lights the open space around it, not itself: light starts where air is.
    // A seam in the city is one of its glass lamps and lights cool; in raw rock, warm.
    for (int y = 0; y < RH; y++)
        for (int x = 0; x < RW; x++) {
            if (!(tileFlags[tiles[y][x]] & TF_EMIT)) continue;
            f32 (*l)[RW] = ZoneAt(x, y) == Z_CITY ? lstatC : lstatW;
            for (int dy = -1; dy <= 1; dy++)
                for (int dx = -1; dx <= 1; dx++) {
                    int nx = x + dx, ny = y + dy;
                    if (nx < 0 || nx >= RW || ny < 0 || ny >= RH) continue;
                    if (Opaque(nx, ny)) continue;
                    if (l[ny][nx] < 1.0f) l[ny][nx] = 1.0f;
                }
        }
    // The openings to the city are its light, pouring in: strong, so it reaches across the
    // hall. Seeded on every open tile whose middle is inside one.
    for (int y = 0; y < RH; y++)
        for (int x = 0; x < RW; x++) {
            if (Opaque(x, y) || TileWater(tiles[y][x])) continue;
            int x0, x1, py = y * TS + TS / 2, px = x * TS + TS / 2;
            if (WindowSpan(py, &x0, &x1) && px >= x0 && px < x1 && lstatC[y][x] < 1.25f) lstatC[y][x] = 1.25f;
            if (GrilleSpan(py, &x0, &x1) && px >= x0 && px < x1 && lstatC[y][x] < 0.9f) lstatC[y][x] = 0.9f;
        }
    // The door's own light: wherever its glass and channels cover a tile, their colour.
    for (int y = 0; y < RH; y++)
        for (int x = 0; x < RW; x++) {
            f32 g = BackdropGlow(x, y) * 0.7f;
            if (g > 0.05f && !Opaque(x, y) && lstatC[y][x] < g) lstatC[y][x] = g;
        }
    // And the hall is never quite dark: the city's cold is in the air of the whole of it,
    // enough to see the colossus by and not enough to see the floor.
    for (int y = 0; y < RH; y++)
        for (int x = 0; x < RW; x++)
            if (!Opaque(x, y) && !TileWater(tiles[y][x]) && ZoneAt(x, y) == Z_CITY && lstatC[y][x] < 0.16f) lstatC[y][x] = 0.16f;
    for (int i = 0; i < bulbCount; i++) {
        int bx = bulbs[i].x / TS, by = (bulbs[i].y - 1) / TS;
        if (bx >= 0 && bx < RW && by >= 0 && by < RH && !Opaque(bx, by) && lstatW[by][bx] < 0.42f)
            lstatW[by][bx] = 0.42f;
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
    Relax(lstatW);
    Relax(lstatC);
    // A seam's own face is fully lit, in its own colour.
    for (int y = 0; y < RH; y++)
        for (int x = 0; x < RW; x++)
            if (tileFlags[tiles[y][x]] & TF_EMIT) {
                if (ZoneAt(x, y) == Z_CITY) lstatC[y][x] = 1.0f; else lstatW[y][x] = 1.0f;
            }
    // the bake for the new look, at tile corners, with the tile codes in the alpha
    for (int j = 0; j <= RH; j++)
        for (int i = 0; i <= RW; i++) {
            f32 aw = 0, ac = 0; int n = 0, wn = 0;
            for (int dy = -1; dy <= 0; dy++)
                for (int dx = -1; dx <= 0; dx++) {
                    int x = i + dx, y = j + dy;
                    if (x < 0 || x >= RW || y < 0 || y >= RH) continue;
                    aw += lstatW[y][x]; ac += lstatC[y][x]; n++;
                    if (TileWater(tiles[y][x])) wn++;
                }
            f32 w = n ? aw / n : 0, c = n ? ac / n : 0, wf = n ? (f32)wn / n : 0;
            u8 code = 0;
            if (i < RW && j < RH) {
                u8 f = tileFlags[tiles[j][i]];
                code = (f & TF_OPAQUE) ? ((f & TF_EMIT) ? 255 : 210)      // stone; a seam is stone that shines
                     : (f & TF_ONEWAY) ? 128 : (f & TF_WATER) ? 40 : 0;   // shelf; water; air
            }
            bpix[j * (RW + 1) + i] = (Color){ (u8)(fminf(w * 0.5f, 1.0f) * 255), (u8)(fminf(c * 0.5f, 1.0f) * 255),
                                              (u8)(wf * 255), code };
        }
    UpdateTexture(bakeTex, bpix);
}

// The body carries a little light of its own -- enough to find yourself by, not
// enough to see the room with. Occluded properly, or it shines through walls.
static void AddPoint(f32 px, f32 py, f32 R, f32 PEAK, int cool) {
    // Only what can reach the view: the composite has room for sixteen, and the room holds
    // more lights than that.
    f32 reach = R * TS + 2.0f;
    if (px + reach < camX || px - reach > camX + GW || py + reach < camY - ROOM_Y || py - reach > camY + GH) return;
    if (LOOK_NEW) {
        // Kept whole for the composite, which lights and shadows it per pixel. If there are
        // too many, the weakest goes.
        int k = ptCount;
        if (k >= PT_MAX) {
            k = 0;
            for (int i = 1; i < PT_MAX; i++) if (pts[i].peak * pts[i].R < pts[k].peak * pts[k].R) k = i;
            if (pts[k].peak * pts[k].R >= PEAK * R) return;
        } else ptCount++;
        pts[k].x = px; pts[k].y = py; pts[k].R = R; pts[k].peak = PEAK; pts[k].cool = cool;
        return;
    }
    f32 (*lnow)[RW] = cool ? lnowC : lnowW;
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
    AddPoint(player.x + player.w * 0.5f, player.y + player.h * 0.5f, 5.4f, 0.48f, 0);
}
void LightAddPoint(f32 px, f32 py, f32 R, f32 peak)     { AddPoint(px, py, R, peak, 0); }
void LightAddPointCool(f32 px, f32 py, f32 R, f32 peak) { AddPoint(px, py, R, peak, 1); }

void LightStep(void) {
    memcpy(lnowW, lstatW, sizeof lnowW);
    memcpy(lnowC, lstatC, sizeof lnowC);
    ptCount = 0;
    AddAura();
    LifeLights();
    ItemsLight();
    PropsLight();
    BackdropLights();
    HallLights();
    // A bulb that has just been landed on throws light for a moment; more, and further,
    // when the landing was timed. That is the only tell there is, and it is enough.
    for (int i = 0; i < bulbCount; i++)
        if (bulbs[i].flash > 0) {
            f32 t = bulbs[i].flash / (bulbs[i].timed ? 26.0f : 22.0f);
            AddPoint((f32)bulbs[i].x, (f32)bulbs[i].y - 3.0f,
                     bulbs[i].timed ? 5.4f : 4.2f, t * (bulbs[i].timed ? 0.90f : 0.55f), 0);
        }
    if (LOOK_NEW) return;             // the composite does the rest, per pixel
    // The grid is sampled at tile CORNERS: (RW+1) x (RH+1) values, drawn back over
    // the room half a tile out on every side so each texel centre lands exactly on
    // its corner. Bilinear does the rest, and the falloff comes out smooth.
    for (int j = 0; j <= RH; j++) {
        for (int i = 0; i <= RW; i++) {
            f32 accW = 0.0f, accC = 0.0f;
            int n = 0;
            for (int dy = -1; dy <= 0; dy++)
                for (int dx = -1; dx <= 0; dx++) {
                    int x = i + dx, y = j + dy;
                    if (x < 0 || x >= RW || y < 0 || y >= RH) continue;
                    accW += lnowW[y][x]; accC += lnowC[y][x]; n++;
                }
            f32 vW = n ? accW / n : 0.0f, vC = n ? accC / n : 0.0f;
            int wn = 0;
            for (int dy = -1; dy <= 0; dy++)
                for (int dx = -1; dx <= 0; dx++) {
                    int x = i + dx, y = j + dy;
                    if (x >= 0 && x < RW && y >= 0 && y < RH && TileWater(tiles[y][x])) wn++;
                }
            f32 wf = n ? (f32)wn / n : 0.0f;          // how much of this corner is under
            f32 h = 1.0f - (f32)j / (f32)RH;          // a shade more sky near the ceiling
            f32 wW = powf(vW, 1.55f), wC = powf(vC, 1.55f);
            f32 r = AMB_R * (0.88f + 0.26f * h) + wW * WARM_R + wC * COOL_R;
            f32 g = AMB_G * (0.88f + 0.24f * h) + wW * WARM_G + wC * COOL_G;
            f32 b = AMB_B * (0.92f + 0.22f * h) + wW * WARM_B + wC * COOL_B;
            r *= 1.0f - 0.40f * wf;                   // under the water everything goes cold
            g *= 1.0f - 0.10f * wf;
            if (r > 1.0f) r = 1.0f;
            if (g > 1.0f) g = 1.0f;
            if (b > 1.0f) b = 1.0f;
            lpix[j * (RW + 1) + i] = (Color){ (u8)(r * 255), (u8)(g * 255), (u8)(b * 255), 255 };
            // The additive half. Squared, so it stays off everywhere except close in.
            // Capped: past this a lamp in hand blew every channel past 255 and the cast
            // wrapped, red first -- a cyan blotch where the light was strongest.
            f32 qW = vW * vW * GLOW, qC = vC * vC * GLOW;
            if (qW + qC > GLOW_CAP) { f32 k = GLOW_CAP / (qW + qC); qW *= k; qC *= k; }
            f32 gr = qW * WARM_R + qC * COOL_R, gg = qW * WARM_G + qC * COOL_G, gb = qW * WARM_B + qC * COOL_B;
            gpix[j * (RW + 1) + i] = (Color){ (u8)(gr * 255), (u8)(gg * 255), (u8)(gb * 255), 255 };
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
static void ParseRoom(u8 dst[RH][RW]) {
    bulbCount = 0; markBeastX = markBeastY = -1; markPlantN = 0; markStoneN = 0;
    for (int y = 0; y < RH; y++) {
        for (int x = 0; x < RW; x++) {
            char c = ROOM_MAP[y][x];
            u8 t = T_EMPTY;
            tileDeco[y][x] = 0;
            switch (c) {
                case '#': t = T_ROCK;  break;
                case 'X': t = T_ROCK;  tileDeco[y][x] = TD_CARVED; break;
                case 'x': t = T_LEDGE; tileDeco[y][x] = TD_CARVED; break;
                case 'k': t = T_ROCK;  tileDeco[y][x] = TD_DEAD; break;
                case '-': t = T_LEDGE; break;
                case '*': t = T_VEIN;  break;
                case ',': t = T_MOSS;  break;
                case '~': t = T_WATER; break;
                case 'b': t = T_BUSH;  break;
                case 'm': markBeastX = x; markBeastY = y; break;
                case 's': if (markStoneN < 4) { markStoneX[markStoneN] = x; markStoneY[markStoneN] = y; markStoneN++; } break;
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
    ParseRoom(tiles);               // sets the bulbs too
    ZonesBuild();
    FindSurfaces();
    PropsInit();
    AirInit();
    LightBake();
    memset(surfH, 0, sizeof surfH);
    memset(surfV, 0, sizeof surfV);
    FxInit();
    LifeInit();
    HallInit();
    AudioAmbience(idx);          // a no-op until the device is up
}

void RoomLoad(void) {
    // A map row that is one character short reads its last column as the string
    // terminator and quietly opens a hole in the wall. Editing these strings by
    // hand did exactly that once, and nothing downstream noticed.
    for (int y = 0; y < RH; y++) {
        int n = 0, m = 0;
        while (ROOM_MAP[y][n]) n++;
        while (ROOM_PROPS[y][m]) m++;
        if (n != RW) TraceLog(LOG_ERROR, "map row %d is %d wide, expected %d", y, n, RW);
        if (m != RW) TraceLog(LOG_ERROR, "props row %d is %d wide, expected %d", y, m, RW);
    }
    ZonesBuild();
    Image im = GenImageColor(RW + 1, RH + 1, WHITE);
    lightTex = LoadTextureFromImage(im);
    glowTex  = LoadTextureFromImage(im);
    bakeTex  = LoadTextureFromImage(im);
    SetTextureFilter(bakeTex, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(bakeTex, TEXTURE_WRAP_CLAMP);
    UnloadImage(im);
    SetTextureFilter(lightTex, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(lightTex, TEXTURE_WRAP_CLAMP);
    SetTextureFilter(glowTex, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(glowTex, TEXTURE_WRAP_CLAMP);
    // items[0] is the lamp (added by main after this); stones follow
    ParseRoom(roomTiles[0]);                                             // finds P
    int xs[4], ys[4], n = RoomMarkStones(xs, ys, 4);
    for (int i = 0; i < n; i++) ItemsAdd(IT_STONE, 0, xs[i], ys[i]);
    memcpy(tiles, roomTiles[0], sizeof tiles);
    BackdropInit();                     // the far wall and the stone, painted once, from the tiles
    RoomEnter(0);                       // (after: the bake seeds the door's light from the picture)
}

void RoomRelight(void) { LightBake(); }

// ---------------------------------------------------------------- the camera
// The room is six screens. The view is always exactly one of them, composed as a screen
// (L11), and when your centre leaves it the view slides to the next: a third of a second,
// eased at both ends, and the world keeps running under it. It never follows you inside a
// screen. Going back needs a little more than crossing the line -- a jump that pokes over
// the top edge and comes down again, or a step back and forth at a side, must not slide the
// view there and back.
#define SCR_W (SW * TS)
#define SCR_H (SH * TS)
#define HYST_X 5.0f         // px past a side edge before the view goes
#define HYST_UP 18.0f       // px past the top edge: more, because jumps go up and come back
#define HYST_DN 2.0f
#define SLIDE_T 22          // frames
f32 camX, camY;
static int scrX, scrY, slideT;
static f32 fromX, fromY;

static int ClampI(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

void CameraInit(void) {
    f32 cx = player.x + player.w * 0.5f, cy = player.y + player.h * 0.5f;
    scrX = ClampI((int)floorf(cx / SCR_W), 0, RW / SW - 1);
    scrY = ClampI((int)floorf(cy / SCR_H), 0, RH / SH - 1);
    camX = (f32)(scrX * SCR_W); camY = (f32)(scrY * SCR_H);
    slideT = 0;
}

void CameraStep(void) {
    f32 cx = player.x + player.w * 0.5f, cy = player.y + player.h * 0.5f;
    int sx = scrX, sy = scrY;
    if (cx < sx * SCR_W - HYST_X) sx--;
    else if (cx >= (sx + 1) * SCR_W + HYST_X) sx++;
    if (cy < sy * SCR_H - HYST_UP) sy--;
    else if (cy >= (sy + 1) * SCR_H + HYST_DN) sy++;
    sx = ClampI(sx, 0, RW / SW - 1);
    sy = ClampI(sy, 0, RH / SH - 1);
    if (sx != scrX || sy != scrY) {
        fromX = camX; fromY = camY;      // from wherever it is, even mid-slide
        scrX = sx; scrY = sy; slideT = SLIDE_T;
    }
    f32 tx = (f32)(scrX * SCR_W), ty = (f32)(scrY * SCR_H);
    if (slideT > 0) {
        slideT--;
        f32 t = 1.0f - (f32)slideT / SLIDE_T, e = t * t * (3.0f - 2.0f * t);
        camX = fromX + (tx - fromX) * e; camY = fromY + (ty - fromY) * e;
    } else { camX = tx; camY = ty; }
}

// Whole pixels, always: a view between pixels would shimmer every edge in the room.
void WorldBegin(void) {
    Camera2D c = { .offset = { 0, 0 }, .target = { roundf(camX), roundf(camY) }, .rotation = 0.0f, .zoom = 1.0f };
    BeginMode2D(c);
}
void WorldEnd(void) { EndMode2D(); }

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

// Raw rock is built from quarter pieces, each chosen by the two neighbours it faces and the
// one diagonal between them: the inside, a top (the surface you stand on), a side (a cliff
// face), an underside (ragged, with drips), an outer corner, an inner corner. Authored for
// the left quarters; the right quarters are the same pieces mirrored.
static const Sprite *const RK_I[4] = { &SPR_RK_I1, &SPR_RK_I2, &SPR_RK_I3, &SPR_RK_I4 };
static const Sprite *const RK_T[3] = { &SPR_RK_T1, &SPR_RK_T2, &SPR_RK_T3 };
static void RockQuarter(int x, int y, int px, int py, int right, int bottom) {
    int hx = right ? 1 : -1, vy = bottom ? 1 : -1;
    int V = Massive(x, y + vy), H = Massive(x + hx, y), D = Massive(x + hx, y + vy);
    u32 h = Hash2(x * 4 + right * 2 + bottom, y * 7 + 1);
    const Sprite *q;
    if (V && H)       q = D ? RK_I[h & 3] : (bottom ? &SPR_RK_NB : &SPR_RK_NT);
    else if (!V && H) q = bottom ? ((h & 1) ? &SPR_RK_B1 : &SPR_RK_B2) : RK_T[h % 3];
    else if (V && !H) q = (h & 1) ? &SPR_RK_L1 : &SPR_RK_L2;
    else              q = bottom ? &SPR_RK_OB : &SPR_RK_OT;
    DrawSpriteTag(q, px + right * 4, py + bottom * 4, right, TAG_STONE);
}

// Stone the far-wall picture already holds (backdrop.c paints it once): all of the city's
// masonry, and raw rock buried on every side. The room draws only the rest -- the rock's
// edges, which are pieces chosen by their neighbours.
int TileBaked(int x, int y) {
    if (x < 0 || x >= RW || y < 0 || y >= RH) return 0;
    if (tiles[y][x] != T_ROCK || (tileDeco[y][x] & TD_CARVED)) return 0;
    if (ZoneAt(x, y) == Z_CITY) return 1;
    return Massive(x, y - 1) && Massive(x, y + 1) && Massive(x - 1, y) && Massive(x + 1, y)
        && Massive(x - 1, y - 1) && Massive(x + 1, y - 1) && Massive(x - 1, y + 1) && Massive(x + 1, y + 1);
}

static void DrawStone(int x, int y, int px, int py) {
    int up = Massive(x, y - 1), dn = Massive(x, y + 1);
    int lf = Massive(x - 1, y), rt = Massive(x + 1, y);
    if (ZoneAt(x, y) == Z_CITY) {
        // Dressed stone: ashlar in running bond, a lit cap where it is open above, a shadowed
        // foot where it is open below. Built things have straight edges.
        DrawSpriteTag((Hash2(x * 3 + 1, y * 5 + 2) & 1) ? &SPR_ASH2 : &SPR_ASH1, px, py, 0, TAG_STONE);
        Color cap = PAL[PL_STONEH], foot = PAL[PL_DEEP], side = PAL[PL_STONEL], back = PAL[PL_DARK];
        cap.a = foot.a = side.a = back.a = TAG_STONE;
        if (!up) DrawRectangle(px, py, TS, 1, cap);
        if (!dn) DrawRectangle(px, py + TS - 1, TS, 1, foot);
        if (!lf) DrawRectangle(px, py + (up ? 0 : 1), 1, TS - 1 - (up ? 0 : 1), side);
        if (!rt) DrawRectangle(px + TS - 1, py + (up ? 0 : 1), 1, TS - 1 - (up ? 0 : 1), back);
        return;
    }
    if (up && dn && lf && rt && Massive(x - 1, y - 1) && Massive(x + 1, y - 1) && Massive(x - 1, y + 1) && Massive(x + 1, y + 1)) {
        // Buried: nothing will see its edges, and there is a lot of it. Strata, and a crack
        // or a pebble now and then, so a great mass of it is still rock and not a hole.
        Color c = PAL[PL_STONEL]; c.a = TAG_STONE; DrawRectangle(px, py, TS, TS, c);
        u32 h = Hash2(x, y);
        Color k = PAL[PL_STONE]; k.a = TAG_STONE;
        Color l = PAL[PL_STONEH]; l.a = TAG_STONE;
        int sy = (int)((Hash2(x / 3, y) >> 4) & 7);                       // a stratum line, wandering
        DrawRectangle(px, py + sy, TS, 1, k);
        DrawRectangle(px + (h & 7), py + (h >> 3 & 7), 1 + (h >> 6 & 1), 1, k);
        if ((h >> 9 & 3) == 0) DrawRectangle(px + (h >> 11 & 7), py + (h >> 14 & 7), 2, 1, l);
        if ((h >> 17 & 7) == 0) {                                          // a crack, three steps
            int cx = px + (h >> 20 & 7), cy = py + (h >> 23 & 3);
            DrawRectangle(cx, cy, 1, 2, k); DrawRectangle(cx + 1, cy + 2, 1, 2, k); DrawRectangle(cx, cy + 4, 1, 2, k);
        }
        return;
    }
    RockQuarter(x, y, px, py, 0, 0); RockQuarter(x, y, px, py, 1, 0);
    RockQuarter(x, y, px, py, 0, 1); RockQuarter(x, y, px, py, 1, 1);
}

static void DrawVein(int x, int y, int px, int py) {
    DrawStone(x, y, px, py);
    if (ZoneAt(x, y) == Z_CITY) {
        // In the city a seam is one of their lamps: a pane of glass in the stone, in an iron
        // frame, still lit after all this time. A glint crosses it now and then.
        DrawSpriteTag(&SPR_GLASS, px, py, 0, TAG_STONE);
        u32 h = Hash2(x * 7 + (int)(frameNo / 13), y);
        Color g = PAL[PL_CITYH]; g.a = TAG_STONE;
        if ((h & 3) == 0) DrawRectangle(px + 2 + (h >> 2 & 3), py + 2 + (h >> 4 & 3), 1, 1, g);
        return;
    }
    // In raw rock, a seam is a vein of crystal that burns with its own light.
    DrawSpriteTag((Hash2(x, y * 3) & 1) ? &SPR_VEIN2 : &SPR_VEIN1, px, py, Hash2(y, x) & 1, TAG_STONE);
}

// A seam the city has drunk: the crystal still there in the joint, black. It gives nothing.
static void DrawDeadSeam(int x, int y, int px, int py) {
    const Sprite *v = (Hash2(x, y * 3) & 1) ? &SPR_VEIN2 : &SPR_VEIN1;
    int flip = Hash2(y, x) & 1;
    Color k = PAL[PL_VOID], g = PAL[PL_DARK]; k.a = g.a = TAG_STONE;
    for (int j = 0; j < v->h; j++)
        for (int i = 0; i < v->w; i++) {
            char c = v->rows[j][flip ? v->w - 1 - i : i];
            if (c == 'a') DrawRectangle(px + i, py + j, 1, 1, k);
            else if (c == 'A') DrawRectangle(px + i, py + j, 1, 1, g);
        }
}

static void DrawLedge(int x, int y, int px, int py) {
    int lf = TileGet(x - 1, y) == T_LEDGE, rt = TileGet(x + 1, y) == T_LEDGE;
    int city = ZoneAt(x, y) == Z_CITY;
    // Three pixels of shelf and nothing below but what holds it up. A one-way surface has to
    // look like a thing you land on top of, or landing on top of it is a surprise.
    const Sprite *end = city ? &SPR_CORNICE_END : &SPR_PLANK_END;
    const Sprite *mid = city ? &SPR_CORNICE : ((Hash2(x * 3, y * 5 + 11) & 1) ? &SPR_PLANK2 : &SPR_PLANK1);
    if (!lf)      DrawSpriteTag(end, px, py, 0, TAG_STONE);
    else if (!rt) DrawSpriteTag(end, px, py, 1, TAG_STONE);
    else          DrawSpriteTag(mid, px, py, 0, TAG_STONE);
    if (!city && ((x + y) & 1) == 0) {                      // a peg under a plank, and its shadow
        DrawRectangle(px + 3, py + 3, 1, 2, PAL[PL_WARMD]);
        DrawRectangle(px + 4, py + 3, 1, 1, PAL[PL_DARK]);
    }
}

static void DrawMoss(int x, int y, int px, int py) {
    int hanging = Massive(x, y - 1) || TileGet(x, y - 1) == T_LEDGE;
    u32 h = Hash2(x * 11 + 5, y * 17);
    if (ZoneAt(x, y) == Z_CITY) { DrawSpriteEx(&SPR_LICHEN, px, py, h & 1); return; }
    if (hanging) DrawSpriteEx((h & 1) ? &SPR_MOSS_H2 : &SPR_MOSS_H1, px, py, (h >> 1) & 1);
    else         DrawSpriteEx(&SPR_MOSS_F1, px, py, h & 1);
}

static void DrawWater(int x, int y, int px, int py) {
    int surface = !TileWater(TileGet(x, y - 1));
    if (surface) {
        // The wave displaces the surface line only. The body stays put.
        int d = (int)(surfH[x] * 3.0f);
        if (d >  3) d =  3;
        if (d < -3) d = -3;
        DrawRectangle(px, py + d, TS, 3, PAL[PL_WATER]);
        DrawRectangle(px, py + d, TS, 1, PAL[PL_WATERL]);
    } else {
        // No fill below the surface: what is behind the water shows through it, and the
        // composite puts it in the water's own blues by how bright it is. Stone under the
        // water is seen as a paler shape in the dark blue.
        // a fleck drifting up now and then: the water is not still
        if (((x * 3 + y * 7 + (int)(frameNo / 26)) % 11) == 0)
            DrawRectangle(px + 2 + (x & 3), py + 3, 1, 1, PAL[PL_WATERL]);
    }
}

static void DrawBush(int x, int y, int px, int py) {
    // A clump on whatever it grows from. Leans away from a body passing, shakes after:
    // the crown moves, the base where it grows does not.
    float dx = (player.x + player.w * 0.5f) - (px + 4.0f);
    int lean = 0;
    if (dx > -12.0f && dx < 12.0f) lean = (dx > 0) ? -1 : 1;
    int shake = bushShake[y][x] ? (((bushShake[y][x] / 2) & 1) ? 1 : -1) : 0;
    int l = lean + shake, flip = Hash2(x * 5 + 3, y * 9 + 1) & 1;
    DrawSpriteRows(&SPR_BUSH, px + l, py, flip, 0, 3, -1);
    DrawSpriteRows(&SPR_BUSH, px + (l > 0 ? (l + 1) / 2 : l / 2), py, flip, 3, 6, -1);
    DrawSpriteRows(&SPR_BUSH, px, py, flip, 6, 8, -1);
}


void RoomDraw(void) {
    // The far wall, under everything, and the colossus, the window and the rest on it:
    // painted once (backdrop.c). Barely a shade off the dark where nothing lights it --
    // you learn the room's depth by carrying light into it.
    BackdropDraw();
    PropsDrawBack();      // the door, the camp: in the wall and on the floor, behind the stone
    HallDrawBack();       // the mural, the dead lamps' recess

    int vx0 = (int)floorf(camX / TS) - 1, vx1 = vx0 + SW + 2;
    int vy0 = (int)floorf(camY / TS) - 1, vy1 = vy0 + SH + 2;
    if (vx0 < 0) vx0 = 0;
    if (vx1 > RW - 1) vx1 = RW - 1;
    if (vy0 < 0) vy0 = 0;
    if (vy1 > RH - 1) vy1 = RH - 1;

    for (int y = vy0; y <= vy1; y++) {
        for (int x = vx0; x <= vx1; x++) {
            int px = x * TS, py = ROOM_Y + y * TS;
            if (tileDeco[y][x] & TD_CARVED) continue;       // the backdrop piece draws it
            switch (tiles[y][x]) {
                case T_ROCK:  if (!TileBaked(x, y)) DrawStone(x, y, px, py);
                              if (tileDeco[y][x] & TD_DEAD) DrawDeadSeam(x, y, px, py);
                              break;
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
            Color c = PAL[PL_ACCENT];
            if (r == H - 1) c = PAL[PL_ACCENT];         // the crown
            if (r == 0)     c = PAL[PL_WARMD];
            DrawRectangle(b->x - half, y, half * 2, 1, c);
        }
        // a lit fleck near the crown, offset to one side: it is round, not flat
        DrawRectangle(b->x - 2, base - H + 1, 2, 1, PAL[PL_BONE]);
        DrawRectangle(b->x + 1, base - 2, 1, 1, PAL[PL_WARMD]);
    }
}

// ---------------------------------------------------------------- debug labels
// Every maximal horizontal run of standable tiles with something open above it, in
// reading order, tagged letter+digit: A1..A9, B1..B9, ... Letters that look like
// digits at 3x5 (I, O, S, Z) are skipped. Bulbs are ^1, ^2 (the ^ is drawn as a dome). This is for talking about
// the room -- "B3 is too far from B4" -- and it is off unless asked for.
#define SURF_MAX 600
typedef struct { i32 x0, x1, y; u8 shelf; char tag[4]; } Surf;
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
            // A1..Y9, then AA1..: past 198 surfaces a second letter
            int L = (int)(sizeof TAG_LETTERS - 1), k = n / 9, c = 0;
            if (k >= L) s->tag[c++] = TAG_LETTERS[(k / L - 1) % L];
            s->tag[c++] = TAG_LETTERS[k % L];
            s->tag[c++] = (char)('1' + n % 9);
            s->tag[c] = 0;
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
    char r[3] = { (char)('A' + scrX), (char)('1' + scrY), 0 };     // which screen: A1 top left .. C2
    Tag((int)roundf(camX) + 3, (int)roundf(camY) + ROOM_Y + 2, r, (Color){ 160, 200, 255, 255 });
}

// For the console: the same table, so a screenshot and a transcript can agree.
void DebugLabelsPrint(void) {
    for (int i = 0; i < surfCount; i++)
        printf("R%d %s %-5s row %2d cols %2d-%2d\n", roomIdx, surfs[i].tag,
               surfs[i].shelf ? "shelf" : "stone", surfs[i].y, surfs[i].x0, surfs[i].x1);
    for (int i = 0; i < bulbCount; i++)
        printf("R%d ^%d bulb  at col %d, base row %d\n", roomIdx, i + 1, bulbs[i].x / TS, bulbs[i].y / TS - 1);
}
