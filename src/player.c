#include "aw.h"
#include <math.h>

Player player;

// ---- feel constants, in pixels per 1/60s frame -----------------------------
// D4: weight touches the gravity axis ONLY. Run speed, acceleration, friction and
// turnaround are identical at every load. The moment weight changes horizontal
// handling it starts doing the gaff's job, and L4 goes with it.
#define RUN_MAX     1.45f
#define ACCEL_G     0.24f
#define ACCEL_A     0.16f
#define ACCEL_TURN  1.9f
#define FRIC_G      0.34f
#define FRIC_A      0.045f
#define GRAV_APEX   0.62f
#define APEX_BAND   0.70f
#define COYOTE      6
#define BUFFER      7

// The dial. Jump apex runs 4.8 tiles empty down to 1.5 tiles full; terminal velocity
// runs the other way, so the light body drifts and the full one plummets.
const f32 JUMP_V[LOAD_MAX + 1] = { -3.55f, -3.38f, -3.20f, -2.95f, -2.70f };
const f32 GRAV_L[LOAD_MAX + 1] = {  0.165f, 0.195f, 0.225f, 0.265f, 0.310f };
const f32 TERM_L[LOAD_MAX + 1] = {  2.60f,  3.40f,  4.30f,  5.20f,  6.10f  };
const f32 JCUT_L[LOAD_MAX + 1] = { -1.17f, -1.12f, -1.05f, -0.97f, -0.89f };
// Signed, and the sign flip is the whole second reading: empty you cannot descend,
// full you walk the pan floor.
const f32 SINK_L[LOAD_MAX + 1] = { -0.55f, -0.24f,  0.22f,  0.34f,  0.46f  };

#define BRINE_DRAG   0.90f
#define BRINE_TERM   1.35f
// Swim thrust is a constant, so which loads can beat it is decided entirely by SINK_L:
// 0 and 1 rise, 2 is marginal and barely holds station, 3 and 4 cannot rise at all.
#define SWIM_THRUST  0.17f
#define FILL_FRAMES  20      // one kettle per 20 frames

// ---------------------------------------------------------------- collision
int RectHitsSolidPublic(float x, float y, int w, int h);

static int RectHitsSolid(float x, float y, int w, int h) {
    int tx0 = (int)floorf(x / TS), tx1 = (int)floorf((x + w - 1) / TS);
    int ty0 = (int)floorf(y / TS), ty1 = (int)floorf((y + h - 1) / TS);
    for (int ty = ty0; ty <= ty1; ty++)
        for (int tx = tx0; tx <= tx1; tx++) {
            if (tx < 0 || tx >= RW || ty < 0 || ty >= RH) return 1;
            if (TileSolid(TileGet(tx, ty))) return 1;
        }
    return 0;
}

int RectHitsSolidPublic(float x, float y, int w, int h) { return RectHitsSolid(x, y, w, h); }

static int LedgeBlocks(float oldBottom, float newY, int w, int h) {
    float newBottom = newY + h;
    int ty0 = (int)floorf(oldBottom / TS), ty1 = (int)floorf((newBottom - 1) / TS);
    int tx0 = (int)floorf(player.x / TS), tx1 = (int)floorf((player.x + w - 1) / TS);
    for (int ty = ty0; ty <= ty1; ty++) {
        if (ty < 0 || ty >= RH) continue;
        for (int tx = tx0; tx <= tx1; tx++) {
            if (tx < 0 || tx >= RW) continue;
            if (!TileOneWay(TileGet(tx, ty))) continue;
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
        if (RectHitsSolid(player.x + step, player.y, player.w, player.h)) { player.vx = 0; return; }
        player.x += step; rem -= step;
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
            if (step > 0) {
                // Landing. Dust scales with speed AND weight, and the floor decides
                // what comes off it. Solves nothing; it is how the world answers you.
                if (player.vy > 1.2f) {
                    float cx = player.x + player.w * 0.5f, cy = ny + player.h;
                    u8 under = TileAtPx(cx, cy + 1.0f);
                    int n = (int)(player.vy * (1.0f + player.load * 0.6f));
                    if (n > 14) n = 14;
                    int kind = (under == T_CRUST) ? FX_SALT
                             : (under == T_TIMBER) ? FX_SPLINTER : FX_DUST;
                    FxBurst(kind, cx, cy, n, 0.7f, 0.32f);
                    player.landImpact = 6 + player.load * 2;

                    // A landing dimples what it lands on. This is the first reading
                    // taken literally, and it is what stops hopping being free: air
                    // frames cost the crust nothing, but arriving does.
                    int add = (int)((player.vy - 1.2f) * (1.0f + player.load) * 0.8f);
                    if (add > 0) {
                        int ty = (int)floorf((ny + player.h) / TS);
                        int tx0 = (int)floorf(player.x / TS);
                        int tx1 = (int)floorf((player.x + player.w - 1) / TS);
                        for (int tx = tx0; tx <= tx1; tx++) {
                            if (tx < 0 || tx >= RW || ty < 0 || ty >= RH) continue;
                            if (TileGet(tx, ty) != T_CRUST) continue;
                            int v = scratch.stress[ty][tx] + add;
                            scratch.stress[ty][tx] = (u8)(v > 255 ? 255 : v);
                        }
                    }
                }
                player.onGround = 1; player.coyote = COYOTE;
            }
            player.vy = 0;
            return;
        }
        player.y = ny; rem -= step;
    }
}

// ---------------------------------------------------------------- helpers
static int TileFlagsUnderBody(int flag) {
    int tx0 = (int)floorf(player.x / TS), tx1 = (int)floorf((player.x + player.w - 1) / TS);
    int ty0 = (int)floorf(player.y / TS), ty1 = (int)floorf((player.y + player.h) / TS);
    for (int ty = ty0; ty <= ty1; ty++)
        for (int tx = tx0; tx <= tx1; tx++) {
            if (tx < 0 || tx >= RW || ty < 0 || ty >= RH) continue;
            if (tileFlags[TileGet(tx, ty)] & flag) return 1;
        }
    return 0;
}

// How much of the body is under the brine line, 0..1. Buoyancy has to be continuous
// or the body oscillates across the surface instead of floating on it.
static float SubmergedFraction(void) {
    float cx = player.x + player.w * 0.5f;
    int n = 0;
    player.waterY = -1;
    for (int i = 0; i < player.h; i++) {
        if (tileFlags[TileAtPx(cx, player.y + i + 0.5f)] & TF_WATER) {
            if (player.waterY < 0) player.waterY = (int)(player.y) + i;
            n++;
        }
    }
    return (float)n / (float)player.h;
}

// Bodies stamp (1 + load) into every tile they rest on. No pivot exemption, no
// airborne exemption beyond not touching anything -- surprise 2 depends on that.
static void StampLoad(void) {
    if (!player.onGround) return;
    int ty = (int)floorf((player.y + player.h) / TS);
    if (ty < 0 || ty >= RH) return;
    int tx0 = (int)floorf(player.x / TS), tx1 = (int)floorf((player.x + player.w - 1) / TS);
    for (int tx = tx0; tx <= tx1; tx++) {
        if (tx < 0 || tx >= RW) continue;
        u8 v = (u8)(1 + player.load);
        if (scratch.loadMap[ty][tx] < v) scratch.loadMap[ty][tx] = v;
    }
}

// ---------------------------------------------------------------- kettles
static void KettleStep(void) {
    player.atRim = TileFlagsUnderBody(TF_RIM);
    // Standing at a rim stirs the brine beside it. Solves nothing; it is only the
    // world admitting you are there.
    if (player.atRim && player.onGround && (frameNo % 14) == 0)
        BrineDisturb(player.x + player.w * 0.5f + player.facing * 10.0f,
                     0.05f + player.load * 0.02f);

    int want = in.a && player.atRim;
    if (!want) { player.fillTimer = 0; }
    else {
        player.fillTimer++;
        if (player.fillTimer >= FILL_FRAMES) {
            player.fillTimer = 0;
            if (in.down) {                       // tip the kettles out over the rim
                if (player.load > 0) {
                    player.load--;
                    FxBurst(FX_DROP, player.x + player.w * 0.5f, player.y + player.h - 2, 9, 0.5f, 0.25f);
                    BrineDisturb(player.x + player.w * 0.5f, 0.45f);
                }
            } else {
                if (player.load < LOAD_MAX) {
                    player.load++;
                    FxBurst(FX_DROP, player.x + player.w * 0.5f, player.y + player.h - 4, 5, 0.35f, 0.18f);
                }
            }
        }
    }
}

// ---------------------------------------------------------------- step
void PlayerInit(float x, float y) {
    player = (Player){0};
    player.x = x; player.y = y;
    player.w = 6; player.h = 12;
    player.facing = 1;
    player.load = 0;
    player.hooked = -1;
}

void PlayerStep(void) {
    const int L = player.load;
    int wasSubmerged = player.submerged;
    player.onGround = 0;

    if (RectHitsSolid(player.x, player.y + 1, player.w, player.h) ||
        LedgeBlocks(player.y + player.h, player.y + 1, player.w, player.h))
        player.onGround = 1;

    float frac = SubmergedFraction();
    player.submerged = frac > 0.05f;

    if (player.submerged != wasSubmerged) {           // a splash sized by what you weigh
        float cx = player.x + player.w * 0.5f;
        FxBurst(FX_DROP, cx, player.y + player.h * 0.5f, 4 + L * 3, 0.8f, 0.45f + L * 0.12f);
        BrineDisturb(cx, (wasSubmerged ? -0.5f : 0.9f) * (0.4f + L * 0.25f));
    }

    if (player.onGround) player.coyote = COYOTE;
    else if (player.coyote > 0) player.coyote--;

    if (in.jumpPressed) player.jumpBuf = BUFFER;
    else if (player.jumpBuf > 0) player.jumpBuf--;

    KettleStep();
    GaffStep();
    if (player.hooked >= 0) {
        // Hanging replaces free movement. The kettles still work -- you can fill or
        // tip while on the hook if a rim is in reach, which is how D1's ratchet and
        // the deep-pan fill are meant to interact.
        player.hookHeldPrev = in.b;
        StampLoad();
        player.animT += 0.05f;
        player.leanX += (player.vx * 0.9f  - player.leanX) * 0.22f;
        player.leanY += (player.vy * 0.35f - player.leanY) * 0.18f;
        float sl = 0.26f - L * 0.045f;
        player.sloshX += (player.vx * (0.5f + L * 0.55f) - player.sloshX) * sl;
        player.sloshY += (player.vy * (0.2f + L * 0.30f) - player.sloshY) * (sl * 0.8f);
        player.yokeAng += (-player.vx * 0.11f * (1.0f + L * 0.4f) - player.yokeAng) * 0.14f;
        if (player.landImpact > 0) player.landImpact--;
        return;
    }
    player.hookHeldPrev = in.b;

    // ---- horizontal: identical at every load (D4)
    int dir = in.right - in.left;
    if (dir) player.facing = dir;
    float accel = player.onGround ? ACCEL_G : ACCEL_A;
    if (dir) {
        if (dir * player.vx < 0) accel *= ACCEL_TURN;
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
    if (frac > 0.05f) {
        // The sign flip IS the second reading: empty, SINK is negative and you rise to
        // the surface and cannot descend; full, it is positive and you walk the floor.
        // Blending by submerged fraction is what makes the surface a place you can rest
        // -- a body floats where buoyancy cancels gravity, which for load 0 is about a
        // quarter under and for load 1 is about half.
        float g = GRAV_L[L] * (1.0f - frac) + SINK_L[L] * frac;
        if (in.jump || in.up) g -= SWIM_THRUST * (frac > 0.2f ? 1.0f : frac * 5.0f);
        player.vy += g;
        player.vy *= (1.0f - 0.16f * frac);           // damping, or it never settles
        player.vx *= (1.0f - (1.0f - BRINE_DRAG) * frac);
        float term = BRINE_TERM + (1.0f - frac) * 2.0f;
        if (player.vy >  term) player.vy =  term;
        if (player.vy < -term) player.vy = -term;
        // A jump still works off the pan floor; it just has water on top of it.
        if (player.jumpBuf > 0 && player.coyote > 0 && frac < 0.9f) {
            player.vy = JUMP_V[L] * (1.0f - frac * 0.45f);
            player.jumpBuf = 0; player.coyote = 0; player.onGround = 0; player.jumpHeld = 1;
        }
        if (!in.jump) player.jumpHeld = 0;
        if ((frameNo & 7) == 0) BrineDisturb(player.x + player.w * 0.5f, fabsf(player.vy) * 0.06f);
    } else {
        if (player.jumpBuf > 0 && player.coyote > 0) {
            player.vy = JUMP_V[L];
            player.jumpBuf = 0; player.coyote = 0;
            player.onGround = 0; player.jumpHeld = 1;
        }
        if (!in.jump) player.jumpHeld = 0;
        if (!player.jumpHeld && player.vy < JCUT_L[L]) player.vy = JCUT_L[L];

        float g = GRAV_L[L];
        if (player.jumpHeld && fabsf(player.vy) < APEX_BAND) g *= GRAV_APEX;
        player.vy += g;
        if (player.vy > TERM_L[L]) player.vy = TERM_L[L];
    }

    if (player.hooked < 0) {
        MoveX(player.vx);
        MoveY(player.vy);
    }
    StampLoad();

    // ---- procedural animation. Lag only: no squash, no stretch (L10).
    player.animT += (fabsf(player.vx) > 0.05f && player.onGround) ? fabsf(player.vx) * 0.20f : 0.035f;
    player.leanX += (player.vx * 0.9f  - player.leanX) * 0.22f;
    player.leanY += (player.vy * 0.35f - player.leanY) * 0.18f;
    // The liquid keeps moving after you stop, and the more of it there is the longer
    // it takes to settle. Solves nothing.
    float slosh = 0.26f - L * 0.045f;
    player.sloshX += (player.vx * (0.5f + L * 0.55f) - player.sloshX) * slosh;
    player.sloshY += (player.vy * (0.2f + L * 0.30f) - player.sloshY) * (slosh * 0.8f);
    player.yokeAng += (-player.vx * 0.11f * (1.0f + L * 0.4f) - player.yokeAng) * 0.14f;
    if (player.landImpact > 0) player.landImpact--;
}

// ---------------------------------------------------------------- draw
void PlayerDraw(void) {
    int px = (int)floorf(player.x), py = (int)floorf(player.y) + ROOM_Y;
    int bob = (player.onGround && fabsf(player.vx) > 0.05f) ? ((int)player.animT & 1) : 0;
    int L = player.load;

    // Yoke, drawn behind: a bar across the shoulders that lags the turn.
    int yy = py + 3 + bob;
    int yl = (int)floorf(-player.yokeAng * 2.0f);
    DrawRectangle(px - 3, yy + (yl > 0 ? 1 : 0), player.w + 6, 1, palIron);

    DrawRectangle(px - 1, py + 2 + bob, player.w + 2, player.h - 2, palSkin);
    DrawRectangle(px, py + bob, player.w, 4, palSkin);

    // Two kettles hanging off the yoke, each showing its own liquid line. No number,
    // no icon, no bar: the load is legible because you can see it.
    for (int s = 0; s < 2; s++) {
        int side = s ? 1 : -1;
        int kx = px + (s ? player.w + 1 : -4);
        int ky = yy + 1 + (int)floorf(player.sloshY * 0.6f) + (yl * side > 0 ? 1 : 0);
        DrawRectangle(kx, ky, 3, 4, palIron);
        int have = (L + (s ? 0 : 1)) / 2;          // kettles fill one at a time
        if (have > 0) {
            int h = have >= 2 ? 3 : 2;
            int tilt = (int)floorf(player.sloshX * 0.8f);
            if (tilt >  1) tilt =  1;
            if (tilt < -1) tilt = -1;
            DrawRectangle(kx, ky + 4 - h, 3, h, palBrine);
            DrawRectangle(kx + (tilt > 0 ? 2 : 0), ky + 4 - h, 1, 1, palBrineL);
        }
    }

    // The brine line cuts the body. Without this a floating player reads as standing
    // on top of the water, because at load 0 only a fifth of them is under it.
    if (player.waterY >= 0) {
        int wy = ROOM_Y + player.waterY;
        int bot = py + player.h;
        if (wy < bot) {
            int top = wy > py ? wy : py;
            DrawRectangle(px - 1, top, player.w + 2, bot - top, palSkinWet);
        }
    }

    int ex = px + (player.facing > 0 ? 3 : 1);
    int ey = py + 1 + bob + (int)(player.leanY * 0.5f);
    DrawRectangle(ex, ey, 1, 2, palBgDeep);
    DrawRectangle(ex + (player.facing > 0 ? 2 : -2), ey, 1, 2, palBgDeep);
}
