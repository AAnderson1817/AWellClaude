// lamp.c -- the first thing you can hold. A small light you carry or leave behind.
//
// It is an object, not a power: it falls, it lands on shelves, it floats, it stays
// where you set it down when you go to another room. Its light adds to the little
// glow you already carry, so with it in hand you are bright and without it you are
// a body with two eyes and a few pixels of aura.
#include "aw.h"
#include <math.h>

Lamp lamp;

#define LAMP_W 4
#define LAMP_H 5
#define LAMP_R 7.4f          // tiles of reach, against the aura's 4.6
#define LAMP_PEAK 0.90f      // against the aura's 0.42

static u32 rng = 0x1A5B7C9Du;
static float Rnd(void) { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return (float)(rng & 0xFFFF) / 65535.0f - 0.5f; }

void LampInit(int room, int tx, int ty) {
    lamp = (Lamp){ 0 };
    lamp.room = room;
    lamp.x = tx * TS + 2.0f;
    lamp.y = (ty + 1) * TS - LAMP_H;
    lamp.held = 0;
}

// Where the lamp hangs when carried: at your side, a little behind your lean.
static void HeldPos(float *x, float *y) {
    int bob = (player.onGround && fabsf(player.vx) > 0.05f) ? ((int)player.animT & 1) : 0;
    float lag = -player.leanX * 0.8f;
    if (lag >  2) lag =  2;
    if (lag < -2) lag = -2;
    *x = player.x + (player.facing > 0 ? player.w + 1.0f : -LAMP_W - 1.0f) + lag;
    *y = player.y + 4.0f + bob;
}

static int Blocked(float x, float y) {
    int tx0 = (int)floorf(x / TS), tx1 = (int)floorf((x + LAMP_W - 1) / TS);
    int ty0 = (int)floorf(y / TS), ty1 = (int)floorf((y + LAMP_H - 1) / TS);
    for (int ty = ty0; ty <= ty1; ty++)
        for (int tx = tx0; tx <= tx1; tx++)
            if (TileSolid(TileGet(tx, ty))) return 1;
    return 0;
}

// Falling, the lamp is caught by whatever would catch you: stone, and a shelf's top
// edge crossed from above. In water it rises and rests at the surface.
static void Fall(void) {
    float cx = lamp.x + LAMP_W * 0.5f;
    int wet = TileWater(TileAtPx(cx, lamp.y + LAMP_H - 1.0f));
    if (wet) {
        int surfaceAbove = TileWater(TileAtPx(cx, lamp.y + 3.5f));   // rest with the glass above the line
        lamp.vy += surfaceAbove ? -0.10f : 0.05f;        // rise until the top clears the line
        lamp.vy *= 0.86f;
    } else {
        lamp.vy += 0.185f;
        if (lamp.vy > 2.9f) lamp.vy = 2.9f;
    }
    float rem = lamp.vy;
    lamp.onGround = 0;
    while (fabsf(rem) > 0.0001f) {
        float step = rem > 1 ? 1 : (rem < -1 ? -1 : rem);
        float ny = lamp.y + step;
        int blocked = Blocked(lamp.x, ny);
        if (!blocked && step > 0) {
            float ob = lamp.y + LAMP_H, nb = ny + LAMP_H;
            int ty0 = (int)floorf(ob / TS), ty1 = (int)floorf(nb / TS);
            int tx0 = (int)floorf(lamp.x / TS), tx1 = (int)floorf((lamp.x + LAMP_W - 1) / TS);
            for (int ty = ty0; ty <= ty1 && !blocked; ty++)
                for (int tx = tx0; tx <= tx1 && !blocked; tx++)
                    if (TileOneWay(TileGet(tx, ty)) && ob <= ty * TS + 0.001f && nb > ty * TS) blocked = 1;
        }
        if (blocked) {
            if (step > 0) {
                if (lamp.vy > 1.0f) Sfx(SFX_SETDOWN, 0.5f + 0.2f * lamp.vy, 0.9f + Rnd() * 0.1f, cx / GW);
                lamp.onGround = 1;
            }
            lamp.vy = 0; return;
        }
        lamp.y = ny; rem -= step;
    }
    if (lamp.y > RH * TS + 8 && lamp.room < ROOM_COUNT - 1) {      // through the floor: the room below
        lamp.room++; lamp.y -= RH * TS;
    }
}

void LampStep(void) {
    lamp.flick += Rnd() * 0.16f;
    lamp.flick *= 0.90f;
    if (lamp.cool > 0) lamp.cool--;

    if (lamp.held) {
        lamp.room = roomIdx;                          // it goes where you go
        HeldPos(&lamp.x, &lamp.y);
        lamp.vy = 0;
        if (in.actPressed && lamp.cool == 0) {
            // Set down at your feet, on the side you face; if that is inside stone, at
            // your feet exactly. Airborne, it simply falls from your hand.
            float x = player.x + (player.facing > 0 ? player.w + 1.0f : -LAMP_W - 1.0f);
            float y = player.y + player.h - LAMP_H;
            if (Blocked(x, y)) x = player.x + (player.w - LAMP_W) * 0.5f;
            lamp.x = x; lamp.y = y; lamp.held = 0; lamp.cool = 8;
            Sfx(SFX_SETDOWN, 0.6f, 1.0f + Rnd() * 0.1f, x / GW);
        }
        return;
    }
    if (lamp.room != roomIdx) return;
    Fall();
    if (in.actPressed && lamp.cool == 0) {
        float lcx = lamp.x + LAMP_W * 0.5f, lcy = lamp.y + LAMP_H * 0.5f;
        float pcx = player.x + player.w * 0.5f;
        if (fabsf(lcx - pcx) < 11.0f && lcy > player.y - 6.0f && lcy < player.y + player.h + 6.0f) {
            lamp.held = 1; lamp.cool = 8;
            Sfx(SFX_PICKUP, 0.7f, 0.95f + Rnd() * 0.1f, lcx / GW);
        }
    }
}

void LampLight(void) {
    if (!lamp.held && lamp.room != roomIdx) return;
    LightAddPoint(lamp.x + LAMP_W * 0.5f, lamp.y + 2.0f, LAMP_R, LAMP_PEAK * (1.0f + lamp.flick * 0.35f));
}

// The body of the thing, before the light pass: dark iron around a pale glass.
void LampDraw(void) {
    if (!lamp.held && lamp.room != roomIdx) return;
    int x = (int)floorf(lamp.x), y = ROOM_Y + (int)floorf(lamp.y);
    DrawRectangle(x + 1, y - 1, 2, 1, palLampIron);            // the loop you hold it by
    DrawRectangle(x, y, LAMP_W, LAMP_H, palLampIron);
    DrawRectangle(x + 1, y + 1, 2, 3, palLampGlass);
}

// The glass again, after the light pass: it is the one thing that is always bright.
void LampDrawCore(void) {
    if (!lamp.held && lamp.room != roomIdx) return;
    int x = (int)floorf(lamp.x), y = ROOM_Y + (int)floorf(lamp.y);
    int hot = lamp.flick > 0.05f;
    DrawRectangle(x + 1, y + 1, 2, 3, palLampGlass);
    DrawRectangle(x + 1 + (hot ? 1 : 0), y + 2, 1, 1, palLampHot);
}
