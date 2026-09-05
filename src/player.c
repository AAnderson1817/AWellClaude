// player.c -- running and jumping. This is the whole game right now, on purpose.
#include "aw.h"
#include <math.h>

Player player;

// Feel, in pixels per 1/60s frame. These are the only numbers that decide whether
// the thing is worth building on, so they get their own file and no company.
#define RUN_MAX     1.45f
#define ACCEL_G     0.24f      // ground
#define ACCEL_A     0.16f      // air
#define ACCEL_TURN  1.90f      // pushing against your own momentum bites harder
#define FRIC_G      0.34f
#define FRIC_A      0.045f
#define GRAV        0.185f
#define TERM        2.90f
#define JUMP_V     -3.45f
#define JUMP_CUT   -1.15f      // let go early and the rise is cut to this
#define GRAV_APEX   0.62f      // gravity is lighter near the top of an arc...
#define APEX_BAND   0.70f      // ...inside this band of vertical speed
#define COYOTE      6          // frames you may still jump after walking off
#define BUFFER      7          // frames a jump press is remembered before landing

// Off a bulb: about 5.2 tiles above the dome's crown. A full jump is 4, so it is a
// place you could not otherwise get to. Meet it with the button and it is 6.2.
#define BOUNCE_V   -4.03f
#define BOUNCE_TIMED_V -4.39f
#define BOUNCE_LIFT (BOUNCE_TIMED_V - BOUNCE_V)   // added to a bounce already in the air

// In water. Gravity and buoyancy are blended BY HOW MUCH OF YOU IS UNDER, never by a
// yes/no test: a body floats where the two cancel, which for this one is a quarter
// under, and it settles there instead of oscillating across the line. This is the
// one piece of the earlier build that was liked, and it is carried over unchanged.
#define WATER_SINK  -0.55f     // signed; negative rises. You are light.
#define STONE_SINK   0.30f     // with a stone in hand you are not: you go to the floor
#define STONE_JUMP   0.78f     // of a jump; the rise scales with its square: 4.0 -> 2.4 tiles
#define STONE_TERM   3.6f
#define STONE_BOUNCE 0.85f     // a bulb throws a heavy body less
#define WATER_DRAG   0.90f
#define WATER_TERM   1.35f
#define WATER_DAMP   0.16f
#define SWIM_THRUST  0.17f     // jump or up, held: you swim upward
#define SURF_JUMP    0.80f     // a jump off the surface, as a fraction of a real one

// ---------------------------------------------------------------- collision
static int RectHitsSolid(float x, float y, int w, int h) {
    int tx0 = (int)floorf(x / TS), tx1 = (int)floorf((x + w - 1) / TS);
    int ty0 = (int)floorf(y / TS), ty1 = (int)floorf((y + h - 1) / TS);
    for (int ty = ty0; ty <= ty1; ty++)
        for (int tx = tx0; tx <= tx1; tx++)
            if (TileSolid(TileGet(tx, ty))) return 1;
    return 0;
}

// Whether the feet CROSS the top edge of a one-way shelf on this step.
//
// This is a crossing test, not an occupancy test. Writing it the other way -- with
// the usual (bottom - 1) inclusive-AABB convention -- looks correct and is not: for
// a one-pixel fall it inspects the row the feet are leaving and never the row they
// are entering, so every shelf in the room is silently passable from above. That
// bug survived four phases of the previous build and was found by a person playing
// it, not by any tool here. It gets its own comment for that reason.
static int LedgeBlocks(float oldBottom, float newY, int w, int h) {
    float newBottom = newY + h;
    int ty0 = (int)floorf(oldBottom / TS), ty1 = (int)floorf(newBottom / TS);
    int tx0 = (int)floorf(player.x / TS), tx1 = (int)floorf((player.x + w - 1) / TS);
    for (int ty = ty0; ty <= ty1; ty++)
        for (int tx = tx0; tx <= tx1; tx++) {
            if (!TileOneWay(TileGet(tx, ty))) continue;
            float top = ty * (float)TS;
            if (oldBottom <= top + 0.001f && newBottom > top) return 1;
        }
    return 0;
}

static void MoveX(float dx) {
    float rem = dx;
    while (fabsf(rem) > 0.0001f) {
        float step = rem > 1.0f ? 1.0f : (rem < -1.0f ? -1.0f : rem);
        if (RectHitsSolid(player.x + step, player.y, player.w, player.h)) { player.vx = 0; return; }
        player.x += step; rem -= step;
    }
}

static void MoveY(float dy) {
    float rem = dy;
    while (fabsf(rem) > 0.0001f) {
        float step = rem > 1.0f ? 1.0f : (rem < -1.0f ? -1.0f : rem);
        float ny = player.y + step;
        if (step > 0) {
            int b = BulbCrossed(player.y + player.h, ny + player.h, player.x, player.w);
            if (b >= 0) {
                // A press in the last few frames is the buffered jump; here it is the
                // timed bounce instead, and it is spent so it cannot also be a jump.
                int timed = player.jumpBuf > 0;
                Sfx(timed ? SFX_BULB_TIMED : SFX_BULB, 1.0f, 1.0f + AudioRnd() * 0.03f, 0.5f);
                player.y = (float)(bulbs[b].y - BULB_H) - player.h;
                player.vy = (timed ? BOUNCE_TIMED_V : BOUNCE_V) * (player.heavy ? STONE_BOUNCE : 1.0f);
                player.launched = 1;
                player.jumpHeld = 0;
                player.jumpBuf = 0;
                player.bulbGrace = timed ? 0 : BULB_LATE;
                player.lastBulb = b;
                bulbs[b].squash = 9;
                bulbs[b].flash  = timed ? 26 : 22;
                bulbs[b].timed  = timed;
                FxBurst(FX_DUST, (float)bulbs[b].x, (float)(bulbs[b].y - BULB_H),
                        timed ? 11 : 6, timed ? 1.5f : 1.1f, 0.22f);
                return;
            }
        }
        int blocked = RectHitsSolid(player.x, ny, player.w, player.h);
        if (!blocked && step > 0 && !in.down)
            blocked = LedgeBlocks(player.y + player.h, ny, player.w, player.h);
        if (blocked) {
            if (step > 0) {
                if (player.vy > 1.1f) {
                    // The floor answers you. It solves nothing and it is not a reward.
                    float cx = player.x + player.w * 0.5f, cy = ny + player.h;
                    int n = (int)(player.vy * 3.0f);
                    if (n > 10) n = 10;
                    FxBurst(FX_DUST, cx, cy, n, 0.8f, 0.30f);
                    player.landImpact = 7;
                    float v = (player.vy - 0.8f) / 2.2f;
                    if (player.heavy) v = v * 1.3f + 0.15f;
                    if (v > 1.0f) v = 1.0f;
                    if (v < 0.25f) v = 0.25f;
                    Sfx(SFX_LAND, v, (player.heavy ? 0.86f : 0.94f) + AudioRnd() * 0.06f, 0.5f);
                } else if (player.vy > 0.3f && !player.onGround) {
                    // a small step down: the footstep sound, not the thud
                    int shelf = TileOneWay(TileAtPx(player.x + player.w * 0.5f, ny + player.h + 0.5f));
                    Sfx(shelf ? SFX_STEP_SHELF : SFX_STEP_STONE, 0.7f, 0.95f + AudioRnd() * 0.08f, 0.5f);
                }
                player.onGround = 1; player.coyote = COYOTE;
            }
            player.vy = 0;
            return;
        }
        player.y = ny; rem -= step;
    }
}

// How much of the body is under the surface, 0..1, sampled down its centre line.
static float SubmergedFraction(void) {
    float cx = player.x + player.w * 0.5f;
    int n = 0;
    player.waterY = -1;
    for (int i = 0; i < player.h; i++) {
        if (TileWater(TileAtPx(cx, player.y + i + 0.5f))) {
            if (player.waterY < 0) player.waterY = (int)player.y + i;
            n++;
        }
    }
    return (float)n / (float)player.h;
}

// ---------------------------------------------------------------- step
void PlayerInit(float x, float y) {
    player = (Player){ 0 };
    player.x = x; player.y = y;
    player.w = 6; player.h = 11;
    player.facing = 1;
    player.blink = 90;
    player.waterY = -1;
}

void PlayerStep(void) {
    int wasSubmerged = player.submerged;
    player.onGround = 0;
    if (RectHitsSolid(player.x, player.y + 1, player.w, player.h) ||
        (!in.down && LedgeBlocks(player.y + player.h, player.y + 1, player.w, player.h)))
        player.onGround = 1;

    float frac = SubmergedFraction();
    player.submerged = frac > 0.05f;
    if (player.submerged != wasSubmerged) {
        // A splash the size of your arrival. Solves nothing.
        float cx = player.x + player.w * 0.5f, sp = fabsf(player.vy);
        FxBurst(FX_SPLASH, cx, player.y + player.h * 0.6f, 4 + (int)(sp * 3.0f), 0.9f, 0.5f + sp * 0.2f);
        WaterDisturb(cx, (wasSubmerged ? -0.45f : 0.9f) * (0.35f + sp * 0.25f));
        // One sound per arrival: the body bobs across the line a few times as it
        // settles, and each crossing is not a new splash.
        if (player.splashCool == 0) {
            if (!wasSubmerged && sp > 0.4f) {
                float v = 0.35f + sp * 0.25f; if (v > 1.0f) v = 1.0f;
                Sfx(SFX_SPLASH_IN, v, 0.92f + AudioRnd() * 0.1f, 0.5f);
                player.splashCool = 75;      // the settling bob crosses the line for about a second
            } else if (wasSubmerged && player.vy < -0.8f) {
                Sfx(SFX_SPLASH_OUT, 0.6f, 1.0f + AudioRnd() * 0.1f, 0.5f);
                player.splashCool = 20;
            }
        }
    }
    if (player.splashCool > 0) player.splashCool--;

    if (player.onGround) { player.coyote = COYOTE; player.airFrames = 0; }
    else { if (player.coyote > 0) player.coyote--; player.airFrames++; }

    if (in.jumpPressed) player.jumpBuf = BUFFER;
    else if (player.jumpBuf > 0) player.jumpBuf--;

    // A press just after leaving a bulb lifts the bounce you are already on. Adding
    // the difference now gives the same arc as having left with it -- a body a few
    // pixels behind an identical curve, which nobody can see.
    if (player.bulbGrace > 0) {
        player.bulbGrace--;
        if (in.jumpPressed) {
            player.vy += BOUNCE_LIFT;
            player.jumpBuf = 0;
            player.bulbGrace = 0;
            Bulb *b = &bulbs[player.lastBulb];
            b->flash = 26; b->timed = 1;
            FxBurst(FX_DUST, (float)b->x, (float)(b->y - BULB_H), 6, 1.5f, 0.30f);
        }
    }

    // ---- horizontal
    int dir = in.right - in.left;
    if (dir) player.facing = dir;
    if (dir) {
        float a = player.onGround ? ACCEL_G : ACCEL_A;
        if (dir * player.vx < 0) a *= ACCEL_TURN;
        player.vx += dir * a;
        if (player.vx >  RUN_MAX) player.vx =  RUN_MAX;
        if (player.vx < -RUN_MAX) player.vx = -RUN_MAX;
    } else {
        float f = player.onGround ? FRIC_G : FRIC_A;
        if (player.vx >  f) player.vx -= f;
        else if (player.vx < -f) player.vx += f;
        else player.vx = 0;
    }

    // ---- vertical
    if (frac > 0.05f) {
        float sink = player.heavy ? STONE_SINK : WATER_SINK;
        float g = GRAV * (1.0f - frac) + sink * frac;
        if (in.jump || in.up) g -= SWIM_THRUST * (frac > 0.2f ? 1.0f : frac * 5.0f);
        player.vy += g;
        player.vy *= (1.0f - WATER_DAMP * frac);          // or it never settles
        player.vx *= (1.0f - (1.0f - WATER_DRAG) * frac);
        float term = WATER_TERM + (1.0f - frac) * 2.0f;
        if (player.vy >  term) player.vy =  term;
        if (player.vy < -term) player.vy = -term;
        // A jump works off a flooded floor, and a smaller one off the surface itself:
        // you are floating, there is nothing to push against but the water.
        if (player.jumpBuf > 0 && (player.coyote > 0 || frac <= 0.6f)) {
            player.vy = (player.coyote > 0 ? JUMP_V * (1.0f - frac * 0.45f) : JUMP_V * SURF_JUMP)
                      * (player.heavy ? STONE_JUMP : 1.0f);
            player.jumpBuf = 0; player.coyote = 0; player.onGround = 0; player.jumpHeld = 1;
            Sfx(SFX_SPLASH_OUT, 0.7f, 1.0f + AudioRnd() * 0.08f, 0.5f);
        }
        if (!in.jump) player.jumpHeld = 0;
        player.launched = 0;
        // swimming: water moved aside, every so often, while you move through it
        if (fabsf(player.vx) > 0.4f && (frameNo % 19) == 0)
            Sfx(SFX_SWIM, 0.5f + 0.5f * fabsf(player.vx) / RUN_MAX, 0.9f + AudioRnd() * 0.15f, 0.5f);
        if ((frameNo & 7) == 0) WaterDisturb(player.x + player.w * 0.5f, fabsf(player.vy) * 0.06f);
    } else {
        if (player.jumpBuf > 0 && player.coyote > 0) {
            player.vy = JUMP_V * (player.heavy ? STONE_JUMP : 1.0f);
            player.jumpBuf = 0; player.coyote = 0;
            player.onGround = 0; player.jumpHeld = 1;
            FxBurst(FX_DUST, player.x + player.w * 0.5f, player.y + player.h, 4, 0.5f, 0.10f);
            Sfx(SFX_JUMP, 1.0f, 0.95f + AudioRnd() * 0.1f, 0.5f);
        }
        if (!in.jump) player.jumpHeld = 0;
        if (player.vy >= 0) player.launched = 0;
        // The cut belongs to a jump you chose the length of. A bounce is the bulb's.
        if (!player.launched && !player.jumpHeld && player.vy < JUMP_CUT) player.vy = JUMP_CUT;

        float g = GRAV;
        if (player.jumpHeld && fabsf(player.vy) < APEX_BAND) g *= GRAV_APEX;
        player.vy += g;
        float term = player.heavy ? STONE_TERM : TERM;
        if (player.vy > term) player.vy = term;
    }

    MoveX(player.vx);
    MoveY(player.vy);

    // The side walls are sealed. Top and bottom are sealed only where no room lies.
    if (player.x < 0) { player.x = 0; player.vx = 0; }
    if (player.x + player.w > RW * TS) { player.x = RW * TS - player.w; player.vx = 0; }
    if (roomIdx == 0 && player.y < 0) { player.y = 0; player.vy = 0; }
    if (roomIdx == ROOM_COUNT - 1 && player.y + player.h > RH * TS) {
        player.y = RH * TS - player.h; player.vy = 0;
    }
    RoomTransition();

    // ---- procedural pass. Lag only: nothing here squashes and nothing stretches.
    {
        int before = (int)player.animT & 1;
        player.animT += (player.onGround && fabsf(player.vx) > 0.05f)
                      ? fabsf(player.vx) * 0.22f : 0.03f;
        // A footstep on each bob of the walk cycle, and it knows what it is on.
        if (player.onGround && fabsf(player.vx) > 0.3f && (((int)player.animT & 1) != before)) {
            int shelf = TileOneWay(TileAtPx(player.x + player.w * 0.5f, player.y + player.h + 0.5f));
            Sfx(shelf ? SFX_STEP_SHELF : SFX_STEP_STONE,
                (0.55f + 0.45f * fabsf(player.vx) / RUN_MAX) * (player.heavy ? 1.25f : 1.0f),
                (player.heavy ? 0.84f : 0.92f) + AudioRnd() * 0.12f, 0.5f);
        }
    }
    player.leanX += (player.vx * 0.9f  - player.leanX) * 0.22f;
    player.leanY += (player.vy * 0.35f - player.leanY) * 0.18f;
    if (player.landImpact > 0) player.landImpact--;
    if (--player.blink < 0) player.blink = 70 + (int)(Hash2((int)frameNo, 3) % 150);
}

// ---------------------------------------------------------------- draw
// Drawn before the light pass, so the body is as dark as wherever it is standing.
void PlayerDraw(void) {
    int px = (int)floorf(player.x), py = (int)floorf(player.y) + ROOM_Y;
    int bob = (player.onGround && fabsf(player.vx) > 0.05f) ? ((int)player.animT & 1) : 0;
    py += bob;

    int lean = (int)floorf(player.leanX * 0.9f);
    if (lean >  1) lean =  1;
    if (lean < -1) lean = -1;

    // A rounded body: two overlapping rectangles take the corners off at 6x11.
    DrawRectangle(px,     py + 1, player.w,     player.h - 2, palSkin);
    DrawRectangle(px + 1, py,     player.w - 2, player.h,     palSkin);
    DrawRectangle(px + 1, py + player.h - 1, player.w - 2, 1, palSkinDeep);
    DrawRectangle(px,     py + player.h - 3, 1, 2, palSkinDeep);
    DrawRectangle(px + player.w - 1, py + player.h - 3, 1, 2, palSkinDeep);

    // The surface cuts the body. Without this a floating body reads as standing on
    // the water, because only a quarter of it is under.
    if (player.waterY >= 0) {
        int wy = ROOM_Y + player.waterY, bot = py + player.h;
        if (wy < bot) {
            int top = wy > py ? wy : py;
            DrawRectangle(px, top, player.w, bot - top, palSkinWet);
        }
    }

    // Two nubs on the head that lag the turn. They are the only thing on the body
    // that reads which way it is going, and they are late about it.
    DrawRectangle(px + 1 + lean, py - 1, 1, 1, palSkin);
    DrawRectangle(px + player.w - 2 + lean, py - 1, 1, 1, palSkin);
}

// Drawn AFTER the light pass. Everything else in the room goes dark where nothing
// reaches it; these two do not, so you can always find yourself.
void PlayerDrawEyes(void) {
    if (player.blink < 5) return;
    int px = (int)floorf(player.x), py = (int)floorf(player.y) + ROOM_Y;
    int bob = (player.onGround && fabsf(player.vx) > 0.05f) ? ((int)player.animT & 1) : 0;
    py += bob;
    int ey = py + 3 + (int)(player.leanY * 0.4f);
    int ex = px + (player.facing > 0 ? 2 : 1);
    // Dark eyes with a lit point in each. In a corner nothing reaches, the two points
    // are the only thing left of you, which is the reason they are drawn out here.
    DrawRectangle(ex,     ey, 1, 2, palPupil);
    DrawRectangle(ex + 3, ey, 1, 2, palPupil);
    DrawRectangle(ex,     ey, 1, 1, palEye);
    DrawRectangle(ex + 3, ey, 1, 1, palEye);
}
