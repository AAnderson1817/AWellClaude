#include "aw.h"
#include <math.h>

// The gaff adds no energy and alters no property of the body. It is a positional
// constraint that redirects momentum you already had (PREMISE.md, verb B).
// The two-body mass ratio is deliberately absent: D2 cut it because it degenerates
// to a plain pendulum against static corners, which is every corner that exists yet.

// A corner worth biting is the top edge of a solid tile with air above it and air
// to one side. Bottom corners are excluded: you hang below the thing you catch.
// Rebuilding renumbers every corner, so anything that refers to one by index has to
// be re-identified afterwards. Wear is carried across by (tile, side) as well: a hook
// you have nearly worn through must not heal because a tile broke somewhere else.
void BuildCorners(void) {
    Room *r = CurRoom();
    static Corner prev[CORNER_MAX];
    int prevCount = scratch.cornerCount;
    for (int i = 0; i < prevCount; i++) prev[i] = scratch.corners[i];
    scratch.cornerCount = 0;
    for (int ty = 1; ty < RH; ty++) {
        for (int tx = 0; tx < RW; tx++) {
            if (!TileSolid(r->tiles[ty][tx])) continue;
            if (TileSolid(r->tiles[ty - 1][tx])) continue;          // roofed: no corner
            int openL = (tx == 0)      ? 0 : !TileSolid(r->tiles[ty][tx - 1]);
            int openR = (tx == RW - 1) ? 0 : !TileSolid(r->tiles[ty][tx + 1]);
            for (int s = 0; s < 2; s++) {
                if (s == 0 && !openL) continue;
                if (s == 1 && !openR) continue;
                if (scratch.cornerCount >= CORNER_MAX) return;
                Corner *c = &scratch.corners[scratch.cornerCount++];
                c->tx = tx; c->ty = ty; c->side = (u8)s; c->worn = 0;
                c->px = tx * TS + (s ? TS : 0);
                c->py = ty * TS;
                for (int k = 0; k < prevCount; k++)
                    if (prev[k].tx == tx && prev[k].ty == ty && prev[k].side == (u8)s) {
                        c->worn = prev[k].worn; break;
                    }
            }
        }
    }

    // Re-point an active hook at the same physical corner, or let go if it is gone.
    if (player.hooked >= 0) {
        int found = -1;
        for (int i = 0; i < scratch.cornerCount; i++)
            if (scratch.corners[i].tx == player.hookTx &&
                scratch.corners[i].ty == player.hookTy &&
                scratch.corners[i].side == player.hookSide) { found = i; break; }
        player.hooked = found;
        if (found < 0) { player.omega = 0.0f; player.pivotFrames = 0; }
    }
}

static int LineClear(float x0, float y0, float x1, float y1) {
    float dx = x1 - x0, dy = y1 - y0;
    int steps = (int)(fabsf(dx) > fabsf(dy) ? fabsf(dx) : fabsf(dy));
    if (steps < 1) return 1;
    for (int i = 1; i < steps; i++) {
        float t = (float)i / (float)steps;
        if (TileSolid(TileAtPx(x0 + dx * t, y0 + dy * t))) return 0;
    }
    return 1;
}

static void PlaceFromAngle(Corner *c, float th, float rr, float *ox, float *oy) {
    *ox = (float)c->px + rr * sinf(th) - player.w * 0.5f;
    *oy = (float)c->py + rr * cosf(th) - player.h * 0.5f;
}

static int BodyFits(float x, float y) {
    int tx0 = (int)floorf(x / TS), tx1 = (int)floorf((x + player.w - 1) / TS);
    int ty0 = (int)floorf(y / TS), ty1 = (int)floorf((y + player.h - 1) / TS);
    for (int ty = ty0; ty <= ty1; ty++)
        for (int tx = tx0; tx <= tx1; tx++) {
            if (tx < 0 || tx >= RW || ty < 0 || ty >= RH) return 0;
            if (TileSolid(CurRoom()->tiles[ty][tx])) return 0;
        }
    return 1;
}

static int FindCorner(float hx, float hy) {
    int best = -1; float bestD = GAFF_REACH * GAFF_REACH;
    for (int i = 0; i < scratch.cornerCount; i++) {
        Corner *c = &scratch.corners[i];
        float dx = (float)c->px - hx, dy = (float)c->py - hy;
        float d2 = dx * dx + dy * dy;
        if (d2 >= bestD) continue;
        if (!LineClear(hx, hy, (float)c->px, (float)c->py)) continue;
        bestD = d2; best = i;
    }
    return best;
}

static void Release(void) {
    Corner *c = &scratch.corners[player.hooked];
    float th = player.theta, rr = player.r;
    // Tangent of P = A + r(sin th, cos th) is r(cos th, -sin th).
    player.vx = player.omega * rr * cosf(th);
    player.vy = -player.omega * rr * sinf(th);
    player.hooked = -1;
    player.pivotFrames = 0;
    (void)c;
}

int dbgReleaseAtBottom = 0;   // measurement instrument, not a game feature

void GaffStep(void) {
    if (player.clack > 0) player.clack--;

    if (player.hooked < 0) {
        if (!in.b) return;
        if (player.hookHeldPrev) return;                    // one attempt per press
        float hx = player.x + player.w * 0.5f + player.facing * 3.0f;
        float hy = player.y + player.h * 0.4f;
        int idx = FindCorner(hx, hy);
        if (idx < 0) {
            // It clacks off. The world answers a wrong idea (L6).
            player.clack = 8;
            FxBurst(FX_DUST, hx + player.facing * 6.0f, hy, 3, 0.35f, 0.12f);
            return;
        }
        Corner *c = &scratch.corners[idx];
        float px = player.x + player.w * 0.5f, py = player.y + player.h * 0.5f;
        float dx = px - (float)c->px, dy = py - (float)c->py;
        float d = sqrtf(dx * dx + dy * dy);
        if (d < 0.5f) return;
        player.r = d < GAFF_RMIN ? GAFF_RMIN : (d > GAFF_RMAX ? GAFF_RMAX : d);
        player.theta = atan2f(dx, dy);
        // If there is nowhere to hang -- a corner at your own feet, a corner with the
        // floor right under it -- the hook clacks off instead of grabbing and dropping
        // you a moment later. The world answers no, once, clearly.
        {
            float tx, ty;
            PlaceFromAngle(c, player.theta, player.r, &tx, &ty);
            if (!BodyFits(tx, ty)) {
                player.clack = 8;
                FxBurst(FX_DUST, (float)c->px, (float)c->py, 3, 0.35f, 0.12f);
                return;
            }
        }
        // Carry in exactly the momentum you already had, projected onto the tangent.
        float ct = cosf(player.theta), st = sinf(player.theta);
        player.omega = (player.vx * ct - player.vy * st) / player.r;
        player.hooked = idx;
        player.hookTx = c->tx; player.hookTy = c->ty; player.hookSide = c->side;
        player.pivotFrames = 0;
        player.onGround = 0;
        return;
    }

    // --- hanging
    Corner *c = &scratch.corners[player.hooked];
    if (!TileSolid(CurRoom()->tiles[c->ty][c->tx])) { Release(); return; }
    if (!in.b) { Release(); return; }

    // Grip slide. Angular momentum r^2*omega is conserved, so pulling in whips you.
    float rOld = player.r;
    if (in.up)   player.r -= 0.35f;
    if (in.down) player.r += 0.35f;
    if (player.r < GAFF_RMIN) player.r = GAFF_RMIN;
    if (player.r > GAFF_RMAX) player.r = GAFF_RMAX;
    if (player.r != rOld) player.omega *= (rOld * rOld) / (player.r * player.r);

    // The one line the whole coupling runs through: the pendulum reads the same
    // gravity table the jump reads.
    player.omega -= GRAV_L[player.load] * sinf(player.theta) / player.r;
    player.omega *= 0.9985f;                      // the haft is not frictionless

    float nth = player.theta + player.omega;
    // You hang below what you bite. The haft cannot pass through the ledge it caught,
    // so the arc stops level with the corner and the swing rebounds off its own limit.
    if (nth >  GAFF_ARC) { nth =  GAFF_ARC; player.omega = -player.omega * 0.35f; }
    if (nth < -GAFF_ARC) { nth = -GAFF_ARC; player.omega = -player.omega * 0.35f; }

    float nx, ny;
    PlaceFromAngle(c, nth, player.r, &nx, &ny);
    if (BodyFits(nx, ny)) {
        player.theta = nth;
        player.x = nx; player.y = ny;
        player.vx = player.omega * player.r * cosf(player.theta);
        player.vy = -player.omega * player.r * sinf(player.theta);
        player.stuck = 0;
    } else {
        player.omega = -player.omega * 0.30f;     // a thud against the thing you caught
        if (++player.stuck > 30) { Release(); return; }   // never wedge: let go instead
    }

    // Instrumented release: let go exactly as the swing crosses straight-down, so the
    // launch is repeatable across loads and the range claim can actually be measured.
    if (dbgReleaseAtBottom && player.omega > 0.0f && player.theta >= 0.0f) { Release(); return; }

    // Feet still touch the world at the bottom of the arc, with no pivot exemption.
    if (RectHitsSolidPublic(player.x, player.y + 1, player.w, player.h)) player.onGround = 1;

    // Corner wear reads the same (1 + load) the crust stamp reads.
    player.pivotFrames++;
    if ((player.pivotFrames & 7) == 0) {
        int w = c->worn + 1 + player.load;
        c->worn = (u8)(w > 255 ? 255 : w);
        if (c->worn >= CORNER_WEAR_MAX) {
            // The nub shears off mid-arc, at whatever angle the counter expired on.
            Room *rm = CurRoom();
            rm->tiles[c->ty][c->tx] = T_EMPTY;
            FxBurst(FX_SALT, (float)c->px, (float)c->py, 9, 0.7f, 0.5f);
            Release();
            scratch.cornersDirty = 1;
            return;
        }
    }
    if ((player.pivotFrames % 5) == 0 && c->worn > 8)
        FxSpawn(FX_DUST, (float)c->px, (float)c->py + 1.0f, 0.0f, 0.05f, 20);

    player.hookHeldPrev = 1;
}

void GaffDraw(void) {
    if (player.hooked < 0) {
        if (player.clack > 0) {
            int hx = (int)(player.x + player.w * 0.5f + player.facing * 6.0f);
            int hy = (int)(player.y + player.h * 0.4f) + ROOM_Y;
            DrawRectangle(hx, hy, 2, 1, palIron);
        }
        return;
    }
    Corner *c = &scratch.corners[player.hooked];
    float px = player.x + player.w * 0.5f, py = player.y + player.h * 0.5f;
    // The haft: a straight iron line to the corner it is biting.
    float dx = (float)c->px - px, dy = (float)c->py - py;
    int steps = (int)(fabsf(dx) > fabsf(dy) ? fabsf(dx) : fabsf(dy));
    for (int i = 0; i <= steps; i++) {
        float t = steps ? (float)i / (float)steps : 0.0f;
        DrawRectangle((int)(px + dx * t), ROOM_Y + (int)(py + dy * t), 1, 1,
                      (i > steps - 3) ? palSaltLit : palIron);
    }
}
