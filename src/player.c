#include "aw.h"
#include <math.h>

Player player;

// ---- feel constants, in pixels per 1/60s frame -----------------------------
// Jump apex = JUMP_V^2 / (2*GRAV) = 3.20^2/0.45 ~= 22.8px ~= 2.85 tiles, plus
// apex hangtime -> clears a 3-tile gap but not a 4-tile one.
#define RUN_MAX     1.45f
#define ACCEL_G     0.24f
#define ACCEL_A     0.16f
#define ACCEL_TURN  1.9f      // multiplier when input opposes velocity
#define FRIC_G      0.34f
#define FRIC_A      0.045f
#define GRAV        0.225f
#define GRAV_APEX   0.62f     // gravity scale near the top of a held jump
#define APEX_BAND   0.70f
#define TERM        4.30f
#define JUMP_V     -3.20f
#define JUMP_CUT   -1.05f     // released early: clamp rise to this
#define COYOTE      6         // frames of ground-memory (bubble chains need these)
#define BUFFER      7         // frames a jump press stays live

#define WATER_GRAV  0.055f
#define WATER_TERM  1.05f
#define WATER_SWIM -1.35f
#define WATER_DRAG  0.88f

// ---------------------------------------------------------------- collision
static int RectHitsSolid(float x, float y, int w, int h) {
    int tx0 = (int)floorf(x / TS), tx1 = (int)floorf((x + w - 1) / TS);
    int ty0 = (int)floorf(y / TS), ty1 = (int)floorf((y + h - 1) / TS);
    for (int ty = ty0; ty <= ty1; ty++) {
        for (int tx = tx0; tx <= tx1; tx++) {
            if (tx < 0 || tx >= RW || ty < 0 || ty >= RH) return 1;
            if (TileSolid(CurRoom()->tiles[ty][tx])) return 1;
        }
    }
    return 0;
}

// One-way ledges only exist for a downward move that starts entirely above them.
static int LedgeBlocks(float oldBottom, float newY, int w, int h) {
    float newBottom = newY + h;
    int ty0 = (int)floorf(oldBottom / TS), ty1 = (int)floorf((newBottom - 1) / TS);
    int tx0 = (int)floorf(player.x / TS), tx1 = (int)floorf((player.x + w - 1) / TS);
    for (int ty = ty0; ty <= ty1; ty++) {
        if (ty < 0 || ty >= RH) continue;
        for (int tx = tx0; tx <= tx1; tx++) {
            if (tx < 0 || tx >= RW) continue;
            if (!TileOneWay(CurRoom()->tiles[ty][tx])) continue;
            float top = ty * (float)TS;
            if (oldBottom <= top + 0.001f && newBottom > top) return 1;
        }
    }
    return 0;
}

static void MoveX(float dx) {
    float rem = dx;
    while (fabsf(rem) > 0.0001f) {
        float step = rem;
        if (step >  1.0f) step =  1.0f;
        if (step < -1.0f) step = -1.0f;
        if (RectHitsSolid(player.x + step, player.y, player.w, player.h)) {
            player.vx = 0;
            return;
        }
        player.x += step;
        rem -= step;
    }
}

static void MoveY(float dy) {
    float rem = dy;
    while (fabsf(rem) > 0.0001f) {
        float step = rem;
        if (step >  1.0f) step =  1.0f;
        if (step < -1.0f) step = -1.0f;
        float ny = player.y + step;
        int blocked = RectHitsSolid(player.x, ny, player.w, player.h);
        if (!blocked && step > 0 && !in.down)
            blocked = LedgeBlocks(player.y + player.h, ny, player.w, player.h);
        if (blocked) {
            if (step > 0) { player.onGround = 1; player.coyote = COYOTE; }
            player.vy = 0;
            return;
        }
        player.y = ny;
        rem -= step;
    }
}

// ---------------------------------------------------------------- step
void PlayerInit(float x, float y) {
    player = (Player){0};
    player.x = x; player.y = y;
    player.w = 6; player.h = 12;
    player.facing = 1;
}

void PlayerStep(void) {
    int wasGround = player.onGround;
    player.onGround = 0;

    // ground probe: one pixel down, ignoring one-ways we are already inside
    if (RectHitsSolid(player.x, player.y + 1, player.w, player.h) ||
        LedgeBlocks(player.y + player.h, player.y + 1, player.w, player.h))
        player.onGround = 1;

    player.inWater = ((tileFlags[TileAtPx(player.x + player.w * 0.5f,
                                          player.y + player.h * 0.5f)]) & TF_WATER) != 0;

    if (player.onGround) player.coyote = COYOTE;
    else if (player.coyote > 0) player.coyote--;

    if (in.jumpPressed) player.jumpBuf = BUFFER;
    else if (player.jumpBuf > 0) player.jumpBuf--;

    // ---- horizontal
    int dir = in.right - in.left;
    if (dir) player.facing = dir;
    float accel = player.onGround ? ACCEL_G : ACCEL_A;
    if (dir) {
        if (dir * player.vx < 0) accel *= ACCEL_TURN;   // turnarounds bite
        player.vx += dir * accel;
        if (player.vx >  RUN_MAX) player.vx =  RUN_MAX;
        if (player.vx < -RUN_MAX) player.vx = -RUN_MAX;
    } else {
        float fric = player.onGround ? FRIC_G : FRIC_A;
        if (player.vx >  fric) player.vx -= fric;
        else if (player.vx < -fric) player.vx += fric;
        else player.vx = 0;
    }

    // ---- vertical
    if (player.inWater) {
        player.vy += WATER_GRAV;
        player.vx *= WATER_DRAG;
        if (in.jump || in.up) player.vy = WATER_SWIM;
        if (player.vy >  WATER_TERM) player.vy =  WATER_TERM;
        if (player.vy < -WATER_TERM * 1.4f) player.vy = -WATER_TERM * 1.4f;
        player.jumpBuf = 0;
    } else {
        if (player.jumpBuf > 0 && player.coyote > 0) {
            player.vy = JUMP_V;
            player.jumpBuf = 0;
            player.coyote = 0;      // consumed, but never retro-cleared elsewhere
            player.onGround = 0;
            player.jumpHeld = 1;
        }
        if (!in.jump) player.jumpHeld = 0;
        if (!player.jumpHeld && player.vy < JUMP_CUT) player.vy = JUMP_CUT;

        float g = GRAV;
        if (player.jumpHeld && fabsf(player.vy) < APEX_BAND) g *= GRAV_APEX;
        player.vy += g;
        if (player.vy > TERM) player.vy = TERM;
    }

    MoveX(player.vx);
    MoveY(player.vy);

    // ---- procedural animation state (second-order lag; never squash-and-stretch)
    player.animT += (fabsf(player.vx) > 0.05f && player.onGround) ? fabsf(player.vx) * 0.20f : 0.035f;
    float targetLeanX = player.vx * 0.9f;
    float targetLeanY = player.vy * 0.35f;
    player.leanX += (targetLeanX - player.leanX) * 0.22f;
    player.leanY += (targetLeanY - player.leanY) * 0.18f;
    (void)wasGround;
}

// ---------------------------------------------------------------- draw
// Placeholder figure. Deliberately no squash-and-stretch: the body is rigid and
// only the lag offsets move, which is what makes code-driven animation misbehave
// in the way we want.
void PlayerDraw(void) {
    int px = (int)floorf(player.x), py = (int)floorf(player.y) + ROOM_Y;
    int bob = (player.onGround && fabsf(player.vx) > 0.05f) ? ((int)player.animT & 1) : 0;

    DrawRectangle(px - 1, py + 2 + bob, player.w + 2, player.h - 2, palSkin);
    DrawRectangle(px, py + bob, player.w, 4, palSkin);

    // eyes: at 320x180 one pixel of eye offset is the whole personality
    int ex = px + (player.facing > 0 ? 3 : 1);
    int ey = py + 1 + bob + (int)(player.leanY * 0.5f);
    DrawRectangle(ex, ey, 1, 2, palBgDeep);
    DrawRectangle(ex + (player.facing > 0 ? 2 : -2), ey, 1, 2, palBgDeep);
}
