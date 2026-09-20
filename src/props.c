// props.c -- the dressing that answers you.
//
// A second text grid per room, one letter per thing, drawn behind or in front of the
// tiles. Nothing here is read by a rule and nothing here is a verb; every prop exists
// because it answers something you do -- passing, landing, setting the lamp down -- and
// the answer is the whole point (L6). The set pieces are text sprites: rows of palette
// letters in this file, like the maps. Nothing streams.
#include "aw.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

// 'D' the door (anchor: top-left)   'L' a loose lintel, dust from its underside (4 wide)
// 'r' a rope, hangs to the first floor below   'c' an iron chain with a glass lamp at its end
// 'R' a root, likewise   'B' bedroll (2 wide)   'F' the cold fire   'K' a pack and a dead lamp
// 'C' the cairn   'X' bones   'P' a pot   'N' a banner on a wall face (hangs 3)
// 'U' a balustrade over a wall tile   'A' a capital (3 wide)   'a' a base (3 wide)   'G' grate bars
static const char *PROPS[ROOM_COUNT][RH] = {
    { // 0: the vault mouth
        "........................................",
        "........L................RRR......L.....",
        ".........RR.................c...........",
        "........................................",
        "........................................",
        "........................................",
        "........................................",
        ".........R..............................",
        "........................................",
        ".D......................................",
        ".......r.....................X..........",
        "........................................",
        "........................................",
        "..................A...........UP......P.",
        "........................................",
        "....r....................P....N.........",
        "........................................",
        "........................................",
        "........................................",
        ".............B.CFKa.....................",
        ".....................GGGGGG.............",
        "........................................",
    },
    { // 1: below -- dressed in the next build
        "........................................",
        "........................................",
        "........................................",
        "........................................",
        "........................................",
        "........................................",
        "........................................",
        "........................................",
        "........................................",
        "........................................",
        "........................................",
        "........................................",
        "........................................",
        "........................................",
        "........................................",
        "........................................",
        "........................................",
        "........................................",
        "........................................",
        "........................................",
        "........................................",
        "........................................",
    },
};


#define PROP_MAX 96
typedef struct {
    u8  kind; i16 tx, ty; u8 len;      // len: tiles a hanging thing hangs, or a width
    f32 a, av;                         // a pendulum's angle and its rate; a banner's amplitude
    f32 x, y, vy;                      // a falling pot, in room px
    i16 timer; u8 state, n;            // small state machines; n counts a pot's knocks
    f32 phase;
} Prop;
static Prop props[PROP_MAX];
static int  propCount;
static int  fireLit[ROOM_COUNT];       // the one change that persists: your flame, lent
static long age;                       // frames since this room was entered

static u32 rng = 0x9E3779B9u;
static float Rnd(void) { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return (float)(rng & 0xFFFF) / 65535.0f - 0.5f; }

int  PropsAge(void) { return (int)age; }
int  PropFireLit(int room) { return room >= 0 && room < ROOM_COUNT ? fireLit[room] : 0; }
void PropsReset(void) { memset(fireLit, 0, sizeof fireLit); }

int PropsViews(PropView *out, int max) {
    if (!out || max <= 0) return 0;
    int count = propCount < max ? propCount : max;
    for (int i = 0; i < count; i++) {
        const Prop *p = &props[i];
        out[i] = (PropView){ .kind = p->kind, .tx = p->tx, .ty = p->ty, .len = p->len,
            .state = p->state, .timer = p->timer, .n = p->n, .fireLit = PropFireLit(roomIdx),
            .x = p->x, .y = p->y, .angle = p->a, .phase = p->phase };
    }
    return count;
}

// ---------------------------------------------------------------- sprites
// Rows of palette letters. '.' is nothing.
Color SpriteInk(char c) {
    switch (c) {
        case 'l': return palAshlarLit;  case 's': return palAshlar;   case 'k': return palRockDeep;
        case 'd': return palDoor;       case 'g': return palDoorGroove; case 'G': return palCityGlass;
        case 'i': return palIron;       case 'w': return palBone;     case 'b': return palRockDeep;
        default:  return (Color){ 0, 0, 0, 0 };
    }
}
void DrawSprite(const Sprite *s, int px, int py) {
    for (int y = 0; y < s->h; y++) {
        const char *row = s->rows[y];
        int x = 0;
        while (x < s->w) {
            char c = row[x];
            if (c == '.') { x++; continue; }
            int x0 = x;
            while (x < s->w && row[x] == c) x++;          // runs, so a 40x48 door is not 1920 quads
            DrawRectangle(px + x0, py + y, x - x0, 1, SpriteInk(c));
        }
    }
}

// The door: a round-headed portal of the city's making, five tiles wide, six tall. Its face
// is a spiral groove crossing six petal veins, with a glass hub. The spiral is the same
// formula the glint follows below, so the glint rides the groove.
static const char *const DOOR_ROWS[48] = {
    "........................................",
    "..............llllllllllll..............",
    "...........lllsssssssssssslll...........",
    "..........llssssssssssssssssll..........",
    "........llsssskkkkkkkkkkkkssssll........",
    ".......llssskkkddddddddddkkksssll.......",
    "......lsssskddddddddddddddddkssssl......",
    ".....lssskkddddddddddddddddddkksssl.....",
    "....llsskkddddddddddddddddddddkkssll....",
    "....lsskkddddddddddddddddddddddkkssl....",
    "...lssskdddddddddddgggggddddddddksssl...",
    "..llsskddddddddddddgddddggdddddddkssll..",
    "..lsskdddddddddddddgddddddgdddddddkssl..",
    "..lsskdddddddddddddgdddddddggdddddkssl..",
    ".lsskkdddddddddddddgddddddddggddddkkssl.",
    ".lsskdddddddddddddggddddddddddgddddkssl.",
    ".lsskdddddddddggggdggggdddddddggdddkssl.",
    ".lsskdddddddggdddddgdddggddddddgdddkssl.",
    ".lsskddggddggddddddgddddggdddddggddkssl.",
    ".lsskddddgggdddddddgdddddgdddggdgddkssl.",
    "lsskddddddggdddddddgddddddgdggddgdddkssl",
    "lsskdddddgddggdddgggddddddggdddddgddkssl",
    "liskdddddgddddggggdgggddggdgdddddgddksil",
    "lsskddddgddddddggddddgggdddgdddddgddkssl",
    "lsskddddgdddddggddGGGgdddddgdddddgddkssl",
    "lsskddddgdddddgdddGGGgdddddgdddddgddkssl",
    "lsskddddgdddddgdddGGGddddddgdddddgddkssl",
    "lsskddddgdddddgdgdddddgdddggdddddgddkssl",
    "liskddddgdddddggdddddddggggdddddgdddksil",
    "lsskddddgddgggdgdddgdddddgggddddgdddkssl",
    "lsskdddddggdddddgddgddddggddggddgdddkssl",
    "lsskddddggdddddddgggdggggddddgggddddkssl",
    "lsskdddgddgddddddddggdddddddddggddddkssl",
    "lsskddddddggdddddddgdddddddddggdddddkssl",
    "liskdddddddggddddddgdddddddddgddddddksil",
    "lsskddddddddggdddddgdddddddggdddddddkssl",
    "lsskdddddddddgggdddgdddddggdddddddddkssl",
    "lsskdddddddddddgggggggggggddddddddddkssl",
    "lsskdddddddddddddddgddddddddddddddddkssl",
    "lsskdddddddddddddddgddddddddddddddddkssl",
    "liskddddddddddddddddddddddddddddddddksil",
    "lsskddddddddddddddddddddddddddddddddkssl",
    "lsskddddddddddddddddddddddddddddddddkssl",
    "lsskddddddddddddddddddddddddddddddddkssl",
    "lsskddddddddddddddddddddddddddddddddkssl",
    "lsskddddddddddddddddddddddddddddddddkssl",
    "lsskkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkssl",
    "llllllllllllllllllllllllllllllllllllllll",
};
static const Sprite DOOR = { 40, 48, DOOR_ROWS };
#define DOOR_HUB_X 19.5f
#define DOOR_HUB_Y 25.5f
#define DOOR_GLINT_STEPS 240

// ---------------------------------------------------------------- init
static int Open(int tx, int ty) {
    u8 t = TileGet(tx, ty);
    return !TileSolid(t) && !TileOneWay(t);
}
static int HangLength(int tx, int ty) {
    int n = 0;
    while (n < 6 && ty + n < RH && Open(tx, ty + n)) n++;
    return n < 1 ? 1 : n;
}

void PropsInit(void) {
    propCount = 0; age = 0;
    for (int y = 0; y < RH; y++)
        for (int x = 0; x < RW && propCount < PROP_MAX; x++) {
            char c = PROPS[roomIdx][y][x];
            int kind = PR_NONE, len = 1;
            switch (c) {
                case 'D': kind = PR_DOOR; break;
                case 'L': kind = PR_LINTEL; len = 4; break;
                case 'r': kind = PR_ROPE;      len = HangLength(x, y); break;
                case 'c': kind = PR_CHAINLAMP; len = HangLength(x, y); break;
                case 'R': kind = PR_ROOT;      len = HangLength(x, y); break;
                case 'B': kind = PR_BEDROLL; len = 2; break;
                case 'F': kind = PR_FIRE; break;
                case 'K': kind = PR_PACK; break;
                case 'C': kind = PR_CAIRN; break;
                case 'X': kind = PR_BONES; break;
                case 'P': kind = PR_POT; break;
                case 'N': kind = PR_BANNER; len = 3; break;
                case 'U': kind = PR_BALUSTRADE; break;
                case 'A': kind = PR_CAPITAL; len = 3; break;
                case 'a': kind = PR_BASE; len = 3; break;
                case 'G': kind = PR_GRATE; break;
                default: break;
            }
            if (!kind) continue;
            Prop *p = &props[propCount++];
            memset(p, 0, sizeof *p);
            p->kind = (u8)kind; p->tx = (i16)x; p->ty = (i16)y; p->len = (u8)len;
            p->phase = (f32)(Hash2(x, y) % 628) / 100.0f;
            p->x = x * TS + 1.5f; p->y = (y + 1) * TS - 6.0f;     // where a pot sits
        }
}

// ---------------------------------------------------------------- step
static f32 PCX(void) { return player.x + player.w * 0.5f; }
static f32 PCY(void) { return player.y + player.h * 0.5f; }
static int Landed(void) { return player.landImpact == 7; }        // the first frame of a hard landing

// A hanging thing is a rigid pendulum: excited where the body passes through it, and by a
// landing near its foot. Rigid is wrong for a rope and right at eight pixels a tile.
static void Swing(Prop *p, f32 gain, int sfx) {
    f32 ax = p->tx * TS + 4.0f, ay = p->ty * (f32)TS, L = p->len * (f32)TS;
    p->av += -p->a * 0.018f;
    p->av *= 0.965f;
    p->a  += p->av;
    f32 tipx = ax + sinf(p->a) * L;
    int over = player.x < fmaxf(ax, tipx) + 4 && player.x + player.w > fminf(ax, tipx) - 4
            && player.y + player.h > ay && player.y < ay + L;
    if (over && fabsf(player.vx) + fabsf(player.vy) > 0.25f) {
        p->av += player.vx * gain + (player.vy > 0 ? 0.0f : player.vy * gain * 0.3f);
        if (p->timer <= 0 && fabsf(player.vx) > 0.35f && sfx >= 0) {
            Sfx(sfx, 0.5f + fabsf(player.vx) * 0.3f, 0.9f + Rnd() * 0.2f, ax / GW);
            p->timer = 22 + (int)(Rnd() * 8);
        }
    }
    if (Landed() && fabsf(PCX() - ax) < 3 * TS && player.y + player.h > ay + L - 4 && player.y < ay + L + 20)
        p->av += (PCX() < ax ? 0.04f : -0.04f);
    if (p->av >  0.12f) p->av =  0.12f;
    if (p->av < -0.12f) p->av = -0.12f;
    if (p->timer > 0) p->timer--;
}

static void Dust(f32 x0, f32 x1, f32 y, int n) {
    for (int i = 0; i < n; i++) FxBurst(FX_DUST, x0 + (x1 - x0) * (Rnd() + 0.5f), y, 1, 0.15f, -0.15f);
}

void PropsStep(void) {
    age++;
    f32 lx, ly; int lamp = LampPos(&lx, &ly);
    f32 bx, by; int beast = LifeBeastPos(&bx, &by);
    for (int i = 0; i < propCount; i++) {
        Prop *p = &props[i];
        f32 px = p->tx * (f32)TS, py = p->ty * (f32)TS;
        switch (p->kind) {
        case PR_DOOR: {
            f32 cx = px + 20, foot = py + 48;
            // The closing just happened: the lintel sheds for the first three seconds.
            if (age < 180 && (age % 9) == 0) Dust(px + 2, px + 30, py, 1);
            if (Landed() && fabsf(PCX() - cx) < 4 * TS && player.y + player.h > foot - 12) {
                Dust(px + 2, px + 30, py, 4);
                Sfx(SFX_GRIT, 0.5f, 1.0f + Rnd() * 0.1f, cx / GW);
            }
            // Your lamp near: a glint runs once around the spiral, then not again until you leave.
            f32 d = lamp ? hypotf(lx - (px + DOOR_HUB_X), ly - (py + DOOR_HUB_Y)) : 1e9f;
            if (p->state == 0 && d < 3 * TS) { p->state = 1; p->timer = 0; }
            else if (p->state == 1) { if (++p->timer >= DOOR_GLINT_STEPS) p->state = 2; }
            else if (p->state == 2 && d > 5 * TS) p->state = 0;
        } break;
        case PR_LINTEL:
            if (Landed() && PCX() > px - 2 * TS && PCX() < px + (p->len + 2) * TS && player.y > py && player.y < py + 12 * TS) {
                Dust(px, px + p->len * TS, py + TS, 3 + (int)(player.vy * 1.5f));
                Sfx(SFX_GRIT, 0.35f, 1.1f + Rnd() * 0.1f, (px + p->len * 4) / GW);
            }
            break;
        case PR_ROPE:      Swing(p, 0.028f, SFX_CREAK); break;
        case PR_CHAINLAMP: Swing(p, 0.020f, SFX_CLINK); break;
        case PR_ROOT:      Swing(p, 0.022f, -1); break;
        case PR_BEDROLL: {
            int on = player.onGround && player.x + player.w > px && player.x < px + p->len * TS
                  && fabsf(player.y + player.h - (py + TS)) < 2.0f;
            if (on && !p->state) Sfx(SFX_RUSTLE, 0.35f, 0.75f + Rnd() * 0.1f, (px + 8) / GW);
            p->state = (u8)on;
        } break;
        case PR_FIRE: {
            f32 cx = px + 4, cy = py + 6;
            if (!fireLit[roomIdx]) {
                int near = lamp && fabsf(lx - cx) < 2 * TS && fabsf(ly - cy) < 2 * TS;
                p->timer = near ? p->timer + 1 : 0;
                if (p->timer >= 120) {
                    fireLit[roomIdx] = 1; p->timer = 0;
                    Sfx(SFX_CATCH, 0.8f, 1.0f, cx / GW);
                    FxBurst(FX_SPARK, cx, cy - 2, 9, 0.9f, 1.2f);
                }
            } else {
                p->phase += 0.21f + Rnd() * 0.06f;
                if (--p->timer <= 0) { Sfx(SFX_CRACKLE, 0.2f + Rnd() * 0.12f, 0.9f + Rnd() * 0.3f, cx / GW); p->timer = 180 + (int)((Rnd() + 0.5f) * 360); }
                if ((frameNo % 7) == 0 && Rnd() > 0.1f) FxBurst(FX_SPARK, cx + Rnd() * 3, cy - 3, 1, 0.3f, 0.7f);
            }
        } break;
        case PR_BONES:
            if (Landed() && fabsf(PCX() - (px + 4)) < 3 * TS && fabsf(player.y + player.h - (py + TS)) < 4) {
                p->timer = 20; Sfx(SFX_RATTLE, 0.5f, 0.95f + Rnd() * 0.1f, px / GW);
            }
            if (p->timer > 0) p->timer--;
            break;
        case PR_POT:
            if (p->state == POT_REST) {
                f32 cx = px + 4;
                int knock = (Landed() && fabsf(PCX() - cx) < 12 && fabsf(player.y + player.h - (py + TS)) < 4)
                         || (fabsf(PCX() - cx) < 6 && fabsf(player.y + player.h - (py + TS)) < 4 && fabsf(player.vx) > 0.4f && p->timer <= 0);
                if (knock) {
                    p->timer = 26; p->n++;
                    Sfx(SFX_POT, 0.55f, 0.95f + Rnd() * 0.15f, cx / GW);
                    int edgeL = Open(p->tx - 1, p->ty + 1), edgeR = Open(p->tx + 1, p->ty + 1);
                    if (p->n >= 2 && (edgeL || edgeR)) {
                        p->state = POT_FALLING; p->vy = 0;
                        p->a = edgeR && !edgeL ? 0.35f : (edgeL && !edgeR ? -0.35f : (PCX() < cx ? 0.35f : -0.35f));
                    }
                }
                if (p->timer > 0) p->timer--;
            } else if (p->state == POT_FALLING) {
                p->vy += 0.185f; if (p->vy > 2.9f) p->vy = 2.9f;
                p->x += p->a; p->y += p->vy;
                u8 below = TileAtPx(p->x + 2.5f, p->y + 6.0f);
                if (TileSolid(below) || (TileOneWay(below) && p->vy > 0)) {
                    p->state = POT_GONE;
                    FxBurst(FX_SHARD, p->x + 2.5f, p->y + 5.0f, 9, 1.1f, 0.9f);
                    FxBurst(FX_DUST, p->x + 2.5f, p->y + 5.0f, 4, 0.7f, 0.2f);
                    Sfx(SFX_SHATTER, 0.7f, 0.95f + Rnd() * 0.1f, p->x / GW);
                } else if (p->y > RH * TS + 8) p->state = POT_GONE;
            }
            break;
        case PR_BANNER: {
            f32 cx = px + 3;
            f32 excite = 0;
            if (fabsf(PCX() - cx) < 14 && player.y + player.h > py - 8 && player.y < py + 30)
                excite = fabsf(player.vx) * 1.6f + (player.onGround ? 0 : fabsf(player.vy) * 0.6f);
            if (beast && fabsf(bx - cx) < 10 && fabsf(by - py) < 20) excite = fmaxf(excite, 0.5f);
            if (excite > p->a) {
                if (p->a < 0.25f && excite > 0.8f) Sfx(SFX_FLAP, 0.35f, 0.9f + Rnd() * 0.2f, cx / GW);
                p->a = fminf(excite, 2.2f);
            }
            p->a *= 0.965f;
            p->phase += 0.22f + p->a * 0.05f;
        } break;
        default: break;
        }
    }
}

// ---------------------------------------------------------------- light
void PropsLight(void) {
    for (int i = 0; i < propCount; i++) {
        Prop *p = &props[i];
        if (p->kind == PR_FIRE && fireLit[roomIdx]) {
            f32 f = 0.85f + 0.15f * sinf(p->phase) + Rnd() * 0.08f;
            LightAddPoint(p->tx * TS + 4.0f, p->ty * TS + 3.0f, 5.2f, 0.62f * f);
        } else if (p->kind == PR_CHAINLAMP) {
            f32 L = p->len * (f32)TS - 3.0f;
            f32 gx = p->tx * TS + 4.0f + sinf(p->a) * L, gy = p->ty * TS + cosf(p->a) * L;
            LightAddPointCool(gx, gy, 6.2f, 0.72f + 0.05f * sinf(frameNo * 0.11f + p->phase));
        }
    }
}

// ---------------------------------------------------------------- draw
static void DrawPot(int x, int y, int tilt) {
    DrawRectangle(x + 1 + tilt, y, 3, 1, palClayLit);          // rim
    DrawRectangle(x + 1 + tilt, y + 1, 3, 1, palRockDeep);     // the dark mouth
    DrawRectangle(x, y + 2, 5, 3, palClay);
    DrawRectangle(x + 1, y + 5, 3, 1, palClay);
    DrawRectangle(x, y + 2, 1, 2, palClayLit);
}

void PropsDrawBack(void) {
    for (int i = 0; i < propCount; i++) {
        Prop *p = &props[i];
        int px = p->tx * TS, py = ROOM_Y + p->ty * TS;
        switch (p->kind) {
        case PR_DOOR: {
            DrawSprite(&DOOR, px, py);
            if (p->state == 1) {
                f32 t = p->timer * (14.2f / DOOR_GLINT_STEPS);        // the groove's parameter, 0..14.2
                f32 r = 2.0f + 0.95f * t;
                int gx = (int)(DOOR_HUB_X + r * cosf(t)), gy = (int)(DOOR_HUB_Y - r * sinf(t));
                DrawRectangle(px + gx, py + gy, 1, 1, palCityGlassLit);
                DrawRectangle(px + gx - 1, py + gy, 3, 1, (Color){ palCityGlass.r, palCityGlass.g, palCityGlass.b, 110 });
            }
        } break;
        case PR_BEDROLL: {
            // canvas, not their dyed cloth: the hunters' things are brown and grey
            int dent = p->state ? 1 : 0;
            DrawRectangle(px, py + 4 + dent, 16, 4 - dent, palLedge);
            DrawRectangle(px + 1, py + 4 + dent, 14, 1, palLedgeLit);
            DrawRectangle(px + 4, py + 5 + dent, 1, 2, palLedgeLit);
            DrawRectangle(px + 11, py + 5 + dent, 1, 2, palLedgeLit);
            DrawRectangle(px, py + 7, 16, 1, palRockDeep);
        } break;
        case PR_PACK: {
            DrawRectangle(px, py + 2, 5, 6, palRope);                  // the pack: oiled canvas
            DrawRectangle(px + 1, py + 1, 3, 1, palRope);
            DrawRectangle(px + 1, py + 3, 3, 1, palRockDeep);          // the strap
            DrawRectangle(px + 5, py + 4, 3, 4, palIron);              // the dead lamp beside it
            DrawRectangle(px + 6, py + 5, 1, 2, palRockDeep);          // dark glass
            f32 lx, ly;
            if (LampPos(&lx, &ly) && fabsf(lx - (px + 6)) < 40 && fabsf(ly - (p->ty * TS + 5)) < 24 && ((frameNo / 3) & 3) == 0)
                DrawRectangle(px + 6, py + 5, 1, 1, palLampGlass);    // a glint in the glass, your light in it
        } break;
        case PR_CAIRN:
            DrawRectangle(px + 1, py + 6, 6, 2, palStone);
            DrawRectangle(px + 2, py + 4, 5, 2, palStone);
            DrawRectangle(px + 2, py + 2, 4, 2, palStone);
            DrawRectangle(px + 3, py + 1, 2, 1, palStoneLit);
            DrawRectangle(px + 2, py + 4, 5, 1, palStoneLit);
            DrawRectangle(px + 1, py + 7, 6, 1, palStoneDeep);
            break;
        case PR_BONES: {
            int tip = p->timer > 0 ? (((p->timer / 3) & 1) ? 1 : 0) : 0;
            DrawRectangle(px + 5 - tip, py, 3, 3, palBone);            // the skull, against the wall
            DrawRectangle(px + 6 - tip, py + 1, 1, 1, palRockDeep);
            DrawRectangle(px + 5, py + 3, 2, 3, palBone);              // ribs
            DrawRectangle(px + 4, py + 4, 1, 1, palBone);
            DrawRectangle(px + 1, py + 6, 6, 1, palBone);              // legs, stretched out
            DrawRectangle(px, py + 7, 2, 1, palBone);
        } break;
        case PR_BALUSTRADE:
            DrawRectangle(px, py, 8, 1, palAshlarLit);
            DrawRectangle(px + 1, py + 1, 1, 6, palRockDeep);
            DrawRectangle(px + 3, py + 1, 2, 6, palRockDeep);
            DrawRectangle(px + 6, py + 1, 1, 6, palRockDeep);
            break;
        default: break;
        }
    }
}

void PropsDrawFront(void) {
    for (int i = 0; i < propCount; i++) {
        Prop *p = &props[i];
        int px = p->tx * TS, py = ROOM_Y + p->ty * TS;
        switch (p->kind) {
        case PR_ROPE: case PR_ROOT: case PR_CHAINLAMP: {
            f32 L = p->len * (f32)TS, ax = px + 4, ay = py;
            f32 s = sinf(p->a), c = cosf(p->a);
            if (p->kind == PR_ROPE) {
                DrawRectangle((int)ax - 1, (int)ay, 3, 2, palRope);                     // the knot
                for (int k = 2; k < (int)L; k++) DrawRectangle((int)(ax + s * k), (int)(ay + c * k), 1, 1, palRope);
                int tx = (int)(ax + s * L), ty = (int)(ay + c * L);
                DrawRectangle(tx - 1, ty - 1, 1, 1, palRope); DrawRectangle(tx + 1, ty - 1, 1, 1, palRope);   // frayed
            } else if (p->kind == PR_ROOT) {
                u32 h = Hash2(p->tx * 5, p->ty * 3);
                for (int k = 0; k < (int)L; k++) {
                    f32 f = (f32)k / L;
                    int x = (int)(ax + s * k * (0.4f + 0.6f * f)) + (((h >> (k & 15)) & 1) ? 0 : 0);
                    DrawRectangle(x, (int)(ay + k), 1, 1, palMoss);
                    if (((h >> ((k * 5) & 31)) & 7) == 0) DrawRectangle(x + (((h >> k) & 1) ? 1 : -1), (int)(ay + k), 1, 1, palRope);
                }
            } else {
                for (int k = 0; k < (int)L - 4; k += 2) DrawRectangle((int)(ax + s * k), (int)(ay + c * k), 1, 1, palIron);
                int gx = (int)(ax + s * (L - 3)) - 2, gy = (int)(ay + c * (L - 3)) - 2;
                DrawRectangle(gx, gy, 4, 1, palIron);
                DrawRectangle(gx, gy + 1, 4, 3, palCityGlass);
                DrawRectangle(gx + 1 + ((frameNo / 9) & 1), gy + 2, 1, 1, palCityGlassLit);
                DrawRectangle(gx, gy + 4, 4, 1, palIron);
            }
        } break;
        case PR_FIRE: {
            // a ring of stones with charcoal in it; lit, three tongues that never hold still
            DrawRectangle(px, py + 6, 2, 2, palStone); DrawRectangle(px + 6, py + 6, 2, 2, palStone);
            DrawRectangle(px + 3, py + 7, 3, 1, palStone);
            DrawRectangle(px + 2, py + 6, 4, 1, palRockDeep);
            if (fireLit[roomIdx]) {
                for (int k = 0; k < 3; k++) {
                    int h = 2 + (int)(1.6f + 1.4f * sinf(p->phase * (1.0f + 0.3f * k) + k * 2.1f));
                    int x = px + 2 + k * 2 + (((int)(p->phase * 3) + k) & 1 ? 0 : 0);
                    DrawRectangle(x, py + 6 - h, 1, h, palFlame);
                    if (h > 2) DrawRectangle(x, py + 6 - h + 1, 1, h - 2, palFlameHot);
                }
                DrawRectangle(px + 2, py + 5, 4, 1, palEmber);
            }
        } break;
        case PR_POT:
            if (p->state == POT_REST) DrawPot(px + 1, py + 2, p->timer > 0 ? (((p->timer / 4) & 1) ? 1 : -1) : 0);
            else if (p->state == POT_FALLING) DrawPot((int)p->x, ROOM_Y + (int)p->y, ((frameNo / 3) & 1) ? 1 : -1);
            break;
        case PR_BANNER: {
            DrawRectangle(px + 1, py, 5, 1, palIron);                                  // the peg
            for (int r = 1; r < p->len * TS - 2; r++) {
                int dx = (int)lroundf(sinf(p->phase + r * 0.42f) * p->a * (0.3f + 0.7f * r / (p->len * TS)));
                DrawRectangle(px + 1 + dx, py + r, 5, 1, (r % 5 == 2) ? palClothLit : palCloth);
            }
        } break;
        case PR_CAPITAL:
            DrawRectangle(px - 1, py, p->len * TS + 2, 2, palAshlarLit);
            DrawRectangle(px - 1, py + 2, p->len * TS + 2, 1, palAshlar);
            DrawRectangle(px, py + 4, p->len * TS, 1, palRockDeep);
            break;
        case PR_BASE:
            DrawRectangle(px - 1, py + 5, p->len * TS + 2, 1, palAshlarLit);
            DrawRectangle(px - 1, py + 6, p->len * TS + 2, 2, palAshlar);
            break;
        case PR_GRATE:
            DrawRectangle(px, py, 8, 3, palRockDeep);
            DrawRectangle(px, py, 8, 1, palIron);
            DrawRectangle(px + 1, py, 1, 3, palIron); DrawRectangle(px + 4, py, 1, 3, palIron); DrawRectangle(px + 7, py, 1, 3, palIron);
            break;
        default: break;
        }
    }
}
