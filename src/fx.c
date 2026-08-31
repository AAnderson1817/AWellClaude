// fx.c -- specks. Motes in the air, water finding its way down, dust off a landing.
// A fixed pool, no allocation, and nothing in here is ever read by a rule.
#include "aw.h"
#include <math.h>
#include <string.h>

static Particle pool[FX_MAX];
static int next;

// Deterministic, so two runs of the same scripted playtest produce the same frame.
static u32 rng = 0x9E3779B9u;
static float Rnd(void) {
    rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
    return (float)(rng & 0xFFFFu) / 65535.0f;
}

// Motes live for the whole session and are not part of the pool: the air is always
// moving, and it should not compete for slots with anything the player caused.
#define MOTE_N 34
static struct { f32 x, y, ph, sp; } motes[MOTE_N];

// Where water can come from: stone with open air beneath it.
#define DRIP_N 64
static struct { i32 x, y; } drips[DRIP_N];
static int dripCount;

void FxInit(void) {
    memset(pool, 0, sizeof pool);
    next = 0;
    rng = 0x9E3779B9u;
    for (int i = 0; i < MOTE_N; i++) {
        motes[i].x = Rnd() * (RW * TS);
        motes[i].y = Rnd() * (RH * TS);
        motes[i].ph = Rnd() * 6.283f;
        motes[i].sp = 0.16f + Rnd() * 0.30f;
    }
    dripCount = 0;
    for (int y = 0; y < RH - 1 && dripCount < DRIP_N; y++)
        for (int x = 1; x < RW - 1 && dripCount < DRIP_N; x++)
            if (TileSolid(TileGet(x, y)) && TileGet(x, y + 1) == T_EMPTY
                && ((x * 7 + y * 13) % 5) == 0) {
                drips[dripCount].x = x; drips[dripCount].y = y; dripCount++;
            }
}

void FxSpawn(int kind, float x, float y, float vx, float vy, int life) {
    Particle *p = &pool[next];
    next = (next + 1) % FX_MAX;
    p->x = x; p->y = y; p->vx = vx; p->vy = vy;
    p->life = p->maxLife = (u16)life;
    p->kind = (u8)kind;
    p->seed = (u8)(rng & 0xFF);
}

void FxBurst(int kind, float x, float y, int n, float spread, float up) {
    for (int i = 0; i < n; i++)
        FxSpawn(kind, x + (Rnd() - 0.5f) * 4.0f, y - Rnd() * 2.0f,
                (Rnd() - 0.5f) * 2.0f * spread, -Rnd() * up - 0.05f,
                16 + (int)(Rnd() * 22.0f));
}

void FxStep(void) {
    for (int i = 0; i < MOTE_N; i++) {
        motes[i].ph += 0.013f + motes[i].sp * 0.004f;
        motes[i].x += sinf(motes[i].ph) * 0.10f;
        motes[i].y -= motes[i].sp * 0.14f;          // the air in here rises, slowly
        if (motes[i].y < 0) { motes[i].y = RH * TS; motes[i].x = Rnd() * (RW * TS); }
    }

    // A drip every so often, from somewhere that has stone over it.
    if (dripCount && (frameNo % 23) == 0 && Rnd() < 0.55f) {
        int k = (int)(Rnd() * dripCount);
        FxSpawn(FX_DRIP, drips[k].x * TS + 2.0f + Rnd() * 4.0f,
                drips[k].y * TS + TS + 0.5f, 0.0f, 0.0f, 400);
    }

    for (int i = 0; i < FX_MAX; i++) {
        Particle *p = &pool[i];
        if (!p->life) continue;
        p->life--;
        switch (p->kind) {
            case FX_DRIP: {
                p->vy += 0.10f;
                if (p->vy > 3.2f) p->vy = 3.2f;
                p->y += p->vy;
                u8 t = TileAtPx(p->x, p->y + 1.0f);
                if (TileSolid(t) || TileOneWay(t) || p->y > RH * TS) {
                    // It lands, and what it does when it lands is all it ever does.
                    for (int k = 0; k < 3; k++)
                        FxSpawn(FX_SPLASH, p->x, p->y, (Rnd() - 0.5f) * 1.4f,
                                -Rnd() * 0.55f - 0.1f, 8 + (int)(Rnd() * 7.0f));
                    p->life = 0;
                }
            } break;
            case FX_SPLASH:
            case FX_DUST: {
                p->vy += (p->kind == FX_SPLASH) ? 0.13f : 0.020f;
                p->x += p->vx; p->y += p->vy;
                p->vx *= 0.90f;
                if (TileSolid(TileAtPx(p->x, p->y))) p->life = 0;
            } break;
            default: break;
        }
    }
}

void FxDraw(void) {
    for (int i = 0; i < MOTE_N; i++) {
        // Motes are barely there. In the dark the light pass removes them entirely,
        // and they only appear where something is lighting the air.
        int x = (int)motes[i].x, y = ROOM_Y + (int)motes[i].y;
        if (TileSolid(TileAtPx(motes[i].x, motes[i].y))) continue;
        DrawRectangle(x, y, 1, 1, (Color){ 128, 124, 150, 255 });
    }
    for (int i = 0; i < FX_MAX; i++) {
        Particle *p = &pool[i];
        if (!p->life) continue;
        int x = (int)p->x, y = ROOM_Y + (int)p->y;
        float t = (float)p->life / (float)p->maxLife;
        switch (p->kind) {
            case FX_DRIP:
                DrawRectangle(x, y, 1, 2, palDrop);
                break;
            case FX_SPLASH:
                DrawRectangle(x, y, 1, 1, palDrop);
                break;
            case FX_DUST: {
                Color c = { 92, 88, 104, 255 };
                if (t > 0.6f) c = (Color){ 118, 112, 130, 255 };
                DrawRectangle(x, y, 1, 1, c);
            } break;
            default: break;
        }
    }
}
