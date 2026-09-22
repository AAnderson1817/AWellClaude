// life.c -- what lives here. Bushes that rustle, birds that would rather be elsewhere
// when you arrive, a long low animal that patrols one ledge and stops to look at you,
// and a plant whose fruit talk to you in a language you will never learn.
//
// None of it can be hurt and none of it hurts you. Nothing here counts anything or
// opens anything. It is here so the rooms are somewhere, not just somewhere to jump.
// Flat arrays, fixed sizes, switch statements. No entity base class, no dispatch.
#include "aw.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

static u32 rng;
static float Rnd(void) {                       // 0..1
    rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
    return (float)(rng & 0xFFFFFFu) / 16777215.0f;
}
static float Dist(float ax, float ay, float bx, float by) { float dx = ax - bx, dy = ay - by; return sqrtf(dx * dx + dy * dy); }
static float PlayerCX(void) { return player.x + player.w * 0.5f; }
static float PlayerCY(void) { return player.y + player.h * 0.5f; }

int lifeBirdsStartled, lifePlantPhrases, lifeBeastTurns, lifeRustles;

// ---------------------------------------------------------------- bushes
// Tiles, drawn by the room; the life they have is here: a shake when a body pushes
// through, and a sound.
u8 bushShake[RH][RW];
static int rustleCool;

static void BushStep(void) {
    for (int y = 0; y < RH; y++)
        for (int x = 0; x < RW; x++)
            if (bushShake[y][x]) bushShake[y][x]--;
    if (rustleCool > 0) rustleCool--;
    if (fabsf(player.vx) < 0.3f && fabsf(player.vy) < 0.5f) return;
    int tx0 = (int)floorf(player.x / TS), tx1 = (int)floorf((player.x + player.w - 1) / TS);
    int ty0 = (int)floorf(player.y / TS), ty1 = (int)floorf((player.y + player.h - 1) / TS);
    for (int ty = ty0; ty <= ty1; ty++)
        for (int tx = tx0; tx <= tx1; tx++) {
            if (TileGet(tx, ty) != T_BUSH) continue;
            if (bushShake[ty][tx] < 8) bushShake[ty][tx] = 14;
            if (rustleCool == 0) {
                Sfx(SFX_RUSTLE, 0.5f + 0.5f * fabsf(player.vx) / 1.45f, 0.9f + AudioRnd() * 0.2f, 0.5f);
                rustleCool = 16; lifeRustles++;
            }
        }
}

// ---------------------------------------------------------------- perches
// Where a bird may sit: the two ends of every standable run that is not the floor and
// not under water, and the top of every bush. Derived from the map, never authored.
#define PERCH_MAX 400
typedef struct { i32 x, y; } Perch;
static Perch perches[PERCH_MAX];
static int perchCount;

static void FindPerches(void) {
    perchCount = 0;
    int n = SurfCount();
    for (int i = 0; i < n && perchCount < PERCH_MAX - 1; i++) {
        int x0, x1, y, shelf; SurfGet(i, &x0, &x1, &y, &shelf);
        if (y >= RH - 3) continue;                            // not the floor
        if (TileGet(x0, y - 1) == T_WATER) continue;
        perches[perchCount++] = (Perch){ x0 * TS + 2, y * TS };
        if (x1 > x0) perches[perchCount++] = (Perch){ x1 * TS + 4, y * TS };
    }
    for (int y = 1; y < RH && perchCount < PERCH_MAX; y++)
        for (int x = 1; x < RW - 1 && perchCount < PERCH_MAX; x++)
            if (tiles[y][x] == T_BUSH) perches[perchCount++] = (Perch){ x * TS + 3, y * TS + 3 };
}

// ---------------------------------------------------------------- birds
#define BIRD_MAX 8
enum { B_PERCH, B_FLY };
typedef struct {
    f32 x, y, vx, vy;
    int state, timer, perch, target, facing, flap, alive;
    f32 pitch;
} Bird;
static Bird birds[BIRD_MAX];

static int PickPerch(float awayX, float awayY, int notThis) {
    // Somewhere far from the thing that startled it. The farthest if nowhere is far.
    int best = -1; float bestD = -1;
    int tries = 0;
    while (tries++ < 24) {
        int k = (int)(Rnd() * perchCount) % perchCount;
        if (k == notThis) continue;
        float d = Dist(perches[k].x, perches[k].y, awayX, awayY);
        if (d > 96.0f) return k;
        if (d > bestD) { bestD = d; best = k; }
    }
    return best;
}

static void BirdsInit(void) {
    memset(birds, 0, sizeof birds);
    if (perchCount == 0) return;
    int want = 4;      // over six screens. It was 3 in one, then 2: the user found them a bit much
    float sx = player.x, sy = player.y;
    for (int i = 0; i < want && i < BIRD_MAX; i++) {
        int p = PickPerch(sx, sy, -1);
        if (p < 0) break;
        Bird *b = &birds[i];
        b->alive = 1; b->state = B_PERCH; b->perch = p;
        b->x = perches[p].x; b->y = perches[p].y;
        b->facing = Rnd() < 0.5f ? -1 : 1;
        b->timer = (int)(Rnd() * 200);
        b->pitch = 0.85f + Rnd() * 0.4f;
    }
}

static void BirdStartle(Bird *b, float fromX, float fromY) {
    int t = PickPerch(fromX, fromY, b->perch);
    if (t < 0) return;
    b->target = t; b->state = B_FLY; b->flap = 0;
    b->vy = -1.2f; b->vx = (perches[t].x > b->x ? 0.6f : -0.6f);
    SfxAt(SFX_WING, 0.8f, 0.9f + AudioRnd() * 0.2f, b->x, b->y);
    AirPush(b->x, b->y - 2.0f, 0.0f, 1.2f, 7.0f);
    lifeBirdsStartled++;
}

static void BirdsStep(void) {
    float px = PlayerCX(), py = PlayerCY();
    for (int i = 0; i < BIRD_MAX; i++) {
        Bird *b = &birds[i];
        if (!b->alive) continue;
        switch (b->state) {
        case B_PERCH: {
            float dx = fabsf(px - b->x), dy = fabsf(py - b->y);
            // Too close, or something heavy just landed nearby: elsewhere, now.
            if ((dx < 44 && dy < 36) || (player.landImpact == 7 && dx < 90 && dy < 60)) {
                BirdStartle(b, px, py);
                break;
            }
            // Every six to eighteen seconds a bird does one thing, and most often that is
            // nothing much. (It was every two to six, and half of those were a song: the
            // room was busy, and the user asked for calm.)
            if (--b->timer <= 0) {
                b->timer = 360 + (int)(Rnd() * 720);
                float r = Rnd();
                if (r < 0.55f) b->facing = -b->facing;                 // look the other way
                else if (r < 0.72f && dx > 70)                          // a small song, when you are not near
                    SfxAt(SFX_CHIRP, 0.5f + Rnd() * 0.3f, b->pitch, b->x, b->y);
                else if (r < 0.77f) BirdStartle(b, b->x + (Rnd() - 0.5f) * 40, b->y);   // restless
            }
        } break;
        case B_FLY: {
            Perch *t = &perches[b->target];
            float dx = t->x - b->x, dy = t->y - b->y, d = sqrtf(dx * dx + dy * dy);
            float far = d > 60 ? 1.0f : d / 60.0f;
            float wantX = dx * 0.09f; if (wantX > 1.7f) wantX = 1.7f; if (wantX < -1.7f) wantX = -1.7f;
            float aimY = t->y - 16.0f * far;                           // climb first, settle late
            float wantY = (aimY - b->y) * 0.09f; if (wantY > 1.2f) wantY = 1.2f; if (wantY < -1.4f) wantY = -1.4f;
            b->vx += (wantX - b->vx) * 0.14f;
            b->vy += (wantY - b->vy) * 0.12f;
            float nx = b->x + b->vx, ny = b->y + b->vy;
            if (TileSolid(TileAtPx(nx, ny - 1))) { ny = b->y - 1.0f; b->vy = -0.6f; }   // not through stone
            if (nx < 4) nx = 4;
            if (nx > RW * TS - 4) nx = RW * TS - 4;
            if (ny < 3) ny = 3;
            b->x = nx; b->y = ny;
            if (fabsf(b->vx) > 0.2f) b->facing = b->vx < 0 ? -1 : 1;
            b->flap++;
            if (d < 2.5f) {
                b->x = t->x; b->y = t->y; b->perch = b->target;
                b->state = B_PERCH; b->timer = 300 + (int)(Rnd() * 600);
            }
        } break;
        }
    }
}

static void BirdsDraw(void) {
    for (int i = 0; i < BIRD_MAX; i++) {
        Bird *b = &birds[i];
        if (!b->alive) continue;
        int x = (int)floorf(b->x), y = ROOM_Y + (int)floorf(b->y);
        int flip = b->facing < 0;
        if (b->state == B_PERCH) {
            // mostly still; now and then it drops its head to look at something
            int look = ((frameNo + i * 53) / 70) % 7 == 0;
            DrawSpriteEx(look ? &SPR_BIRD_LOOK : &SPR_BIRD_PERCH, x - 4, y - 6, flip);
        } else {
            DrawSpriteEx(((b->flap / 4) & 1) ? &SPR_BIRD_UP : &SPR_BIRD_DOWN, x - 3, y - 3, flip);
        }
    }
}

static void BirdsDrawEyes(void) {
    for (int i = 0; i < BIRD_MAX; i++) {
        Bird *b = &birds[i];
        if (!b->alive || b->state != B_PERCH) continue;
        int look = ((frameNo + i * 53) / 70) % 7 == 0;
        int x = (int)floorf(b->x), y = ROOM_Y + (int)floorf(b->y);
        DrawRectangle(x - 4 + (b->facing < 0 ? 1 : 6), y - 6 + (look ? 2 : 1), 1, 1, PAL[PL_BONE]);
    }
}

// ---------------------------------------------------------------- the animal
// Long and low. It has one ledge and it walks it, stops, sits, and when you are near it
// stands still and watches you. It never comes to you and never runs from you.
enum { M_WALK, M_PAUSE, M_SIT, M_WATCH };
typedef struct {
    f32 x, y;               // x = left of the body, y = the ground under its feet
    int dir, state, timer, alive, blink;
    f32 x0, x1;             // the ledge, in pixels
    f32 legT;
    f32 tail[5];            // vertical lag of each tail segment
    f32 headLift;
} Beast;
static Beast beast;
#define BEAST_W 10

static void BeastInit(void) {
    memset(&beast, 0, sizeof beast);
    int tx, ty;
    if (!RoomMarkBeast(&tx, &ty)) return;
    // Its ledge is the standable run it was set down on.
    int n = SurfCount();
    for (int i = 0; i < n; i++) {
        int x0, x1, y, shelf; SurfGet(i, &x0, &x1, &y, &shelf);
        if (y == ty + 1 && tx >= x0 && tx <= x1) {
            beast.x0 = x0 * TS + 1.0f; beast.x1 = (x1 + 1) * TS - BEAST_W - 1.0f;
            beast.y = y * TS; beast.x = tx * TS; beast.alive = 1;
            beast.dir = 1; beast.state = M_PAUSE; beast.timer = 60; beast.blink = 100;
            return;
        }
    }
}

int LifeBeastPos(f32 *x, f32 *y) {
    if (!beast.alive) return 0;
    *x = beast.x + BEAST_W * 0.5f; *y = beast.y;
    return 1;
}

static void BeastStep(void) {
    if (!beast.alive) return;
    float cx = beast.x + BEAST_W * 0.5f, px = PlayerCX(), py = PlayerCY();
    float dx = px - cx, dy = py - beast.y;
    int near = fabsf(dx) < 46 && dy > -30 && dy < 12;
    if (near && beast.state != M_WATCH && beast.state != M_SIT) {
        beast.state = M_WATCH; beast.timer = 0;
        beast.dir = dx < 0 ? -1 : 1;
    }
    switch (beast.state) {
    case M_WALK:
        beast.x += beast.dir * 0.55f;
        beast.legT += 0.16f;
        if (beast.x <= beast.x0) { beast.x = beast.x0; beast.dir = 1; lifeBeastTurns++; }
        if (beast.x >= beast.x1) { beast.x = beast.x1; beast.dir = -1; lifeBeastTurns++; }
        if (((int)(beast.legT * 2) & 1) != ((int)((beast.legT - 0.16f) * 2) & 1) && fabsf(dx) < 130)
            SfxAt(SFX_PAD, 0.35f, 0.9f + AudioRnd() * 0.2f, cx, beast.y);
        if (--beast.timer <= 0) {
            float r = Rnd();
            beast.state = r < 0.7f ? M_PAUSE : M_SIT;
            beast.timer = beast.state == M_SIT ? 600 + (int)(Rnd() * 900) : 120 + (int)(Rnd() * 300);
            if (beast.state == M_SIT && Rnd() < 0.5f) SfxAt(SFX_CHIRR, 0.45f, 0.95f + AudioRnd() * 0.1f, cx, beast.y);
        }
        break;
    case M_PAUSE:
        if (--beast.timer <= 0) {
            beast.state = M_WALK; beast.timer = 60 + (int)(Rnd() * 180);
            if (Rnd() < 0.4f) beast.dir = -beast.dir;
        }
        break;
    case M_SIT:
        if (--beast.timer <= 0) { beast.state = M_PAUSE; beast.timer = 40; }
        break;
    case M_WATCH:
        beast.dir = dx < 0 ? -1 : 1;                                   // it follows you with its head
        beast.headLift += (0.8f - beast.headLift) * 0.1f;
        if (!near) { if (++beast.timer > 40) { beast.state = M_PAUSE; beast.timer = 30 + (int)(Rnd() * 60); } }
        else beast.timer = 0;
        break;
    }
    if (beast.state != M_WATCH) beast.headLift += (0.0f - beast.headLift) * 0.1f;
    // The tail follows, late, each segment later than the one before.
    float sway = sinf(frameNo * 0.05f) * 0.6f + (beast.state == M_WALK ? sinf(beast.legT * 3.0f) * 0.8f : 0.0f);
    float target = (beast.state == M_SIT ? 2.0f : 0.0f) + sway;
    for (int i = 0; i < 5; i++) {
        beast.tail[i] += (target - beast.tail[i]) * (0.35f - i * 0.05f);
        target = beast.tail[i];
    }
    if (--beast.blink < 0) beast.blink = 80 + (int)(Rnd() * 200);
}

// Where its head is: at the front of the body, lifted when it watches, lower when it sits.
static void BeastHead(int *hx, int *hy) {
    int x = (int)floorf(beast.x), y = ROOM_Y + (int)floorf(beast.y);
    int sit = beast.state == M_SIT;
    *hx = beast.dir > 0 ? x + 9 : x - 4;
    *hy = y - 7 + (sit ? 2 : 0) - (int)floorf(beast.headLift + 0.5f);
}
static void BeastDraw(void) {
    if (!beast.alive) return;
    int x = (int)floorf(beast.x), y = ROOM_Y + (int)floorf(beast.y);
    int flip = beast.dir < 0;
    const Sprite *s = &SPR_BEAST_STAND;
    if (beast.state == M_SIT) s = &SPR_BEAST_SIT;
    else if (beast.state == M_WALK) s = (((int)(beast.legT * 2)) & 1) ? &SPR_BEAST_WALK1 : &SPR_BEAST_WALK2;
    // tail first, behind the body: a lag chain, ringed dark and light
    int sit = beast.state == M_SIT;
    int tx = beast.dir > 0 ? x : x + BEAST_W - 1;
    for (int i = 0; i < 6; i++) {
        int ty = y - (sit ? 3 : 4) - (int)floorf(beast.tail[i < 5 ? i : 4] * 0.6f + i * 0.55f);
        DrawRectangle(tx - (beast.dir > 0 ? i : -i), ty, 1, 1, PAL[(i & 1) ? PL_WARMD : PL_WARM]);
    }
    DrawSpriteEx(s, x - 1, y - 7, flip);
    int hx, hy; BeastHead(&hx, &hy);
    DrawSpriteEx(&SPR_BEAST_HEAD, hx, hy, flip);
}

static void BeastDrawEyes(void) {
    if (!beast.alive || beast.blink < 4) return;
    int hx, hy; BeastHead(&hx, &hy);
    DrawRectangle(hx + 2, hy + 1, 1, 1, PAL[PL_CITYH]);    // one green eye: it is of this place
}

// ---------------------------------------------------------------- the plant
// A stalk with three fruit. When you come near, the fruit turn to you and speak -- a
// phrase of a few syllables, each from one pod, whose mouth opens for it -- and then
// they are quiet for a while. It says nothing you can use. It is saying it anyway.
#define PLANT_MAX 3
enum { P_IDLE, P_SPEAK, P_COOL };
typedef struct {
    i32 x, y;               // base: bottom-centre, room px
    int state, timer, syl, pod, mouth, perk, alive;
    f32 sway, lean;
    f32 basePitch;
} Plant;
static Plant plants[PLANT_MAX];
static const int PODX[3] = { -3, 3, -2 }, PODY[3] = { -6, -10, -14 };
static const float PODPITCH[3] = { 0.84f, 1.0f, 1.26f };

static void PlantsInit(void) {
    memset(plants, 0, sizeof plants);
    int xs[4], ys[4], n = RoomMarkPlants(xs, ys, 4);
    for (int i = 0; i < n && i < PLANT_MAX; i++) {
        plants[i].alive = 1; plants[i].x = xs[i] * TS + 4; plants[i].y = (ys[i] + 1) * TS;
        plants[i].basePitch = 0.9f + 0.2f * i;
        plants[i].state = P_IDLE;
    }
}

static void PlantSyllable(Plant *p) {
    int pod = (int)(Rnd() * 3);
    if (pod == p->pod) pod = (pod + 1) % 3;
    p->pod = pod; p->mouth = 7 + (int)(Rnd() * 4);
    p->timer = p->mouth + 2 + (int)(Rnd() * 3);
    int which = SFX_PLANT0 + (int)(Rnd() * 3);
    SfxAt(which, 0.8f, p->basePitch * PODPITCH[pod] * (0.97f + Rnd() * 0.06f), (float)p->x, (float)p->y);
}

static void PlantsStep(void) {
    float px = PlayerCX(), py = PlayerCY();
    for (int i = 0; i < PLANT_MAX; i++) {
        Plant *p = &plants[i];
        if (!p->alive) continue;
        float dx = px - p->x, dy = py - (p->y - 10);
        int near = fabsf(dx) < 40 && fabsf(dy) < 30;
        // lean toward you a little when you are near; sway in no wind otherwise
        float wantLean = near ? (dx < 0 ? -1.0f : 1.0f) * 0.8f : 0.0f;
        p->lean += (wantLean - p->lean) * 0.06f;
        p->sway = sinf(frameNo * 0.021f + i * 1.7f) * 0.7f + p->lean;
        if (p->perk > 0) p->perk--;
        if (near && player.jumpBuf == BUFFER_FRAMES) p->perk = 12;    // it noticed the jump
        switch (p->state) {
        case P_IDLE:
            if (near && Rnd() < 0.02f) { p->state = P_SPEAK; p->syl = 4 + (int)(Rnd() * 5); PlantSyllable(p); lifePlantPhrases++; }
            break;
        case P_SPEAK:
            if (p->mouth > 0) p->mouth--;
            if (--p->timer <= 0) {
                if (--p->syl > 0) PlantSyllable(p);
                else { p->state = P_COOL; p->timer = 600 + (int)(Rnd() * 600); p->mouth = 0; }
            }
            break;
        case P_COOL:
            if (--p->timer <= 0) p->state = P_IDLE;
            break;
        }
    }
}

static void PodPos(Plant *p, int k, int *ox, int *oy) {
    float h = -PODY[k] / 14.0f;                       // higher pods sway more
    *ox = p->x + PODX[k] + (int)floorf(p->sway * h + 0.5f);
    *oy = p->y + PODY[k] - (p->perk > 6 ? 1 : 0);
}

static void PlantsDraw(void) {
    for (int i = 0; i < PLANT_MAX; i++) {
        Plant *p = &plants[i];
        if (!p->alive) continue;
        int bx = p->x, by = ROOM_Y + p->y;
        // stalk, in three lengths that lean progressively
        for (int s = 0; s < 14; s++) {
            int sx = bx + (int)floorf(p->sway * (s / 14.0f) + 0.5f);
            DrawRectangle(sx, by - 1 - s, 1, 1, PAL[PL_COOLM]);
        }
        // leaves at the base and the middle
        DrawRectangle(bx - 3, by - 3, 3, 1, PAL[PL_COOLM]); DrawRectangle(bx + 1, by - 2, 3, 1, PAL[PL_COOLM]);
        DrawRectangle(bx - 3, by - 2, 1, 1, PAL[PL_COOLD]); DrawRectangle(bx + 3, by - 1, 1, 1, PAL[PL_COOLD]);
        DrawRectangle(bx - 2 + (int)floorf(p->sway * 0.5f), by - 8, 2, 1, PAL[PL_COOLM]);
        for (int k = 0; k < 3; k++) {
            int ox, oy; PodPos(p, k, &ox, &oy); oy += ROOM_Y;
            DrawRectangle(ox, oy - 2, 1, 1, PAL[PL_COOLM]);                             // stem
            // the fruit; its mouth opens for the syllable
            int open = p->state == P_SPEAK && p->pod == k && p->mouth > 0;
            DrawSprite(open ? &SPR_POD_OPEN : &SPR_POD, ox - 1, oy - 1);
        }
    }
}

static void PlantsLights(void) {
    for (int i = 0; i < PLANT_MAX; i++) {
        Plant *p = &plants[i];
        if (!p->alive) continue;
        for (int k = 0; k < 3; k++) {
            int ox, oy; PodPos(p, k, &ox, &oy);
            int talking = p->state == P_SPEAK && p->pod == k && p->mouth > 0;
            LightAddPoint(ox + 0.5f, oy + 1.5f, talking ? 3.0f : 1.8f, talking ? 0.30f : 0.10f);
        }
    }
}

// ---------------------------------------------------------------- api
void LifeInit(void) {
    rng = 0x51F15EEDu ^ (u32)(roomIdx * 977);
    memset(bushShake, 0, sizeof bushShake);
    rustleCool = 0;
    FindPerches();
    BirdsInit();
    BeastInit();
    PlantsInit();
}
void LifeStep(void)     { BushStep(); BirdsStep(); BeastStep(); PlantsStep(); }
void LifeDraw(void)     { PlantsDraw(); BeastDraw(); BirdsDraw(); }
void LifeDrawEyes(void) { BirdsDrawEyes(); BeastDrawEyes(); }
void LifeLights(void)   { PlantsLights(); }
void LifePrintStats(void) {
    printf("LIFE birds startled %d, plant phrases %d, beast turns %d, rustles %d, perches %d\n",
           lifeBirdsStartled, lifePlantPhrases, lifeBeastTurns, lifeRustles, perchCount);
}
