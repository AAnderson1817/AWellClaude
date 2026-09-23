// air.c -- the air in the room, moving.
//
// Animal Well runs a fluid solver over the whole screen on a layer of its own, and sprites
// push into it. This is the same idea, small: stable fluids (Stam) on a grid of 4px cells
// over the room. Stone is a wall to it and the water's surface is its floor. What stirs it:
// you moving, landing, jumping, splashing; the fire; the door's dust; birds leaving; a draft
// up through the grate; mist off the water. What shows it: dust, smoke and mist drawn into
// the lit layer -- so the air is seen only where light falls -- and the motes, which ride it.
//
// Presentational. Nothing in the game reads it, and headless runs switch it off.
#include "aw.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define NX (RW * TS / 4)            // 240 cells across
#define NY (RH * TS / 4)            // 88 down
#define IX(i, j) ((i) + (NX + 2) * (j))
#define SZ ((NX + 2) * (NY + 2))
#define ITERS 10

static f32 u[SZ], v[SZ], u0[SZ], v0[SZ], d[SZ], d0[SZ], tint[SZ];
static u8  wall[SZ];                 // 1: stone or water, where air is not
int airOff = 0;

static int CellWall(int i, int j) {  // i, j: 1..NX, 1..NY
    int px = (i - 1) * 4 + 2, py = (j - 1) * 4 + 2;
    u8 t = TileAtPx((float)px, (float)py);
    return TileSolid(t) || TileWater(t);
}

void AirInit(void) {
    memset(u, 0, sizeof u); memset(v, 0, sizeof v); memset(d, 0, sizeof d);
    memset(u0, 0, sizeof u0); memset(v0, 0, sizeof v0); memset(d0, 0, sizeof d0); memset(tint, 0, sizeof tint);
    for (int j = 0; j < NY + 2; j++)
        for (int i = 0; i < NX + 2; i++)
            wall[IX(i, j)] = (i == 0 || j == 0 || i == NX + 1 || j == NY + 1) ? 1 : CellWall(i, j);
}

// Walls: nothing moves into or out of them, and nothing is kept in them.
static void Bound(f32 *x) { for (int k = 0; k < SZ; k++) if (wall[k]) x[k] = 0.0f; }

static void LinSolve(f32 *x, const f32 *x0, f32 a, f32 c) {
    for (int it = 0; it < ITERS; it++) {
        for (int j = 1; j <= NY; j++)
            for (int i = 1; i <= NX; i++) {
                int k = IX(i, j);
                if (wall[k]) continue;
                x[k] = (x0[k] + a * (x[k - 1] + x[k + 1] + x[k - (NX + 2)] + x[k + (NX + 2)])) / c;
            }
        Bound(x);
    }
}

static void Advect(f32 *dd, const f32 *s, const f32 *uu, const f32 *vv) {
    for (int j = 1; j <= NY; j++)
        for (int i = 1; i <= NX; i++) {
            int k = IX(i, j);
            if (wall[k]) { dd[k] = 0; continue; }
            f32 x = i - uu[k], y = j - vv[k];
            if (x < 0.5f) x = 0.5f;
            if (x > NX + 0.5f) x = NX + 0.5f;
            if (y < 0.5f) y = 0.5f;
            if (y > NY + 0.5f) y = NY + 0.5f;
            int i0 = (int)x, j0 = (int)y;
            f32 s1 = x - i0, t1 = y - j0, s0 = 1 - s1, t0 = 1 - t1;
            dd[k] = s0 * (t0 * s[IX(i0, j0)] + t1 * s[IX(i0, j0 + 1)]) + s1 * (t0 * s[IX(i0 + 1, j0)] + t1 * s[IX(i0 + 1, j0 + 1)]);
        }
    Bound(dd);
}

// Make the flow free of divergence: what goes in somewhere comes out somewhere else, so a
// push becomes a swirl rather than a pile.
static void Project(f32 *uu, f32 *vv, f32 *p, f32 *div) {
    for (int j = 1; j <= NY; j++)
        for (int i = 1; i <= NX; i++) {
            int k = IX(i, j);
            div[k] = wall[k] ? 0 : -0.5f * (uu[k + 1] - uu[k - 1] + vv[k + (NX + 2)] - vv[k - (NX + 2)]);
            p[k] = 0;
        }
    LinSolve(p, div, 1.0f, 4.0f);
    for (int j = 1; j <= NY; j++)
        for (int i = 1; i <= NX; i++) {
            int k = IX(i, j);
            if (wall[k]) continue;
            uu[k] -= 0.5f * (p[k + 1] - p[k - 1]);
            vv[k] -= 0.5f * (p[k + (NX + 2)] - p[k - (NX + 2)]);
        }
    Bound(uu); Bound(vv);
}

static int Cell(f32 px, f32 py, int *i, int *j) {
    *i = (int)(px / 4.0f) + 1; *j = (int)(py / 4.0f) + 1;
    return *i >= 1 && *i <= NX && *j >= 1 && *j <= NY && !wall[IX(*i, *j)];
}

// Push the air at a point (room px), in px per frame, over a radius in px.
void AirPush(f32 px, f32 py, f32 vx, f32 vy, f32 radius) {
    if (airOff) return;
    int r = (int)(radius / 4.0f) + 1, ci, cj;
    Cell(px, py, &ci, &cj);
    for (int j = cj - r; j <= cj + r; j++)
        for (int i = ci - r; i <= ci + r; i++) {
            if (i < 1 || i > NX || j < 1 || j > NY || wall[IX(i, j)]) continue;
            f32 dx = (i - 0.5f) * 4 - px, dy = (j - 0.5f) * 4 - py, w = 1.0f - sqrtf(dx * dx + dy * dy) / (radius + 2.0f);
            if (w <= 0) continue;
            u[IX(i, j)] += vx / 4.0f * w;
            v[IX(i, j)] += vy / 4.0f * w;
        }
}
// Put something into the air there: dust, smoke, mist. warm: 0 dust and mist, 1 smoke.
void AirPuff(f32 px, f32 py, f32 amount, f32 radius, f32 warm) {
    if (airOff) return;
    int r = (int)(radius / 4.0f) + 1, ci, cj;
    Cell(px, py, &ci, &cj);
    for (int j = cj - r; j <= cj + r; j++)
        for (int i = ci - r; i <= ci + r; i++) {
            if (i < 1 || i > NX || j < 1 || j > NY || wall[IX(i, j)]) continue;
            f32 dx = (i - 0.5f) * 4 - px, dy = (j - 0.5f) * 4 - py, w = 1.0f - sqrtf(dx * dx + dy * dy) / (radius + 2.0f);
            if (w <= 0) continue;
            int k = IX(i, j);
            d[k] += amount * w;
            tint[k] += (warm - tint[k]) * 0.5f * w;
        }
}
// The room's drafts, pushed in every step (the table is antechamber.c's).
static void RoomDrafts(void) {
    for (const Draft *r = ROOM_DRAFTS; r->r > 0; r++) AirPush(r->x * TS, r->y * TS, r->vx, r->vy, r->r);
}

// What the drafts add up to at a point, with the same reach as a push. Not the air: the air
// is for looking at and headless runs switch it off, and what floats must drift the same
// way in every run, so it reads this.
void DraftAt(f32 px, f32 py, f32 *vx, f32 *vy) {
    *vx = *vy = 0;
    for (const Draft *r = ROOM_DRAFTS; r->r > 0; r++) {
        f32 dx = px - r->x * TS, dy = py - r->y * TS, w = 1.0f - sqrtf(dx * dx + dy * dy) / (r->r + 2.0f);
        if (w <= 0) continue;
        *vx += r->vx * w; *vy += r->vy * w;
    }
}

// The air's velocity at a point, in px per frame.
void AirAt(f32 px, f32 py, f32 *vx, f32 *vy) {
    int i, j;
    if (airOff || !Cell(px, py, &i, &j)) { *vx = *vy = 0; return; }
    *vx = u[IX(i, j)] * 4.0f; *vy = v[IX(i, j)] * 4.0f;
}

void AirStep(void) {
    if (airOff) return;
    RoomDrafts();
    // velocity: carried by itself, made to swirl, and let go of slowly
    memcpy(u0, u, sizeof u); memcpy(v0, v, sizeof v);
    Advect(u, u0, u0, v0); Advect(v, v0, u0, v0);
    Project(u, v, u0, v0);
    for (int k = 0; k < SZ; k++) { u[k] *= 0.985f; v[k] *= 0.985f; }
    // what is in it: carried, spread a very little, and fading
    memcpy(d0, d, sizeof d);
    Advect(d, d0, u, v);
    memcpy(d0, tint, sizeof tint);
    Advect(tint, d0, u, v);
    for (int k = 0; k < SZ; k++) { d[k] *= 0.988f; if (d[k] < 0.002f) d[k] = 0; }
    if (getenv("AWELL_AIRDBG") && (frameNo % 10) == 0) {
        f32 md = 0, sd = 0, mu = 0; int nd = 0;
        for (int k = 0; k < SZ; k++) { if (d[k] > md) md = d[k]; sd += d[k]; if (d[k] > 0.03f) nd++; f32 m = fabsf(u[k]) + fabsf(v[k]); if (m > mu) mu = m; }
        printf("AIR f=%ld max d %.3f sum %.2f cells>0.03 %d max |vel| %.3f\n", frameNo, md, sd, nd, mu);
    }
}

// Dust, smoke and mist, drawn into the lit layer as dithered pixels: in the dark they vanish,
// and a lamp or the fire finds them hanging in the air. Never over stone.
void AirDraw(void) {
    if (airOff) return;
    int i0v = (int)(camX / 4.0f), j0v = (int)(camY / 4.0f) - 1;     // only the cells in view
    for (int j = j0v < 1 ? 1 : j0v; j <= NY && j <= j0v + GH / 4 + 2; j++)
        for (int i = i0v < 1 ? 1 : i0v; i <= NX && i <= i0v + GW / 4 + 2; i++) {
            int k = IX(i, j);
            if (d[k] < 0.03f) continue;
            for (int y = 0; y < 4; y++)
                for (int x = 0; x < 4; x++) {
                    int px = (i - 1) * 4 + x, py = (j - 1) * 4 + y;
                    // density between cell centres, so the edges of a cloud are soft
                    f32 fx = px / 4.0f + 0.5f, fy = py / 4.0f + 0.5f;
                    int i0 = (int)fx, j0 = (int)fy;
                    f32 s1 = fx - i0, t1 = fy - j0;
                    f32 dv = (1 - s1) * ((1 - t1) * d[IX(i0, j0)] + t1 * d[IX(i0, j0 + 1)])
                           + s1 * ((1 - t1) * d[IX(i0 + 1, j0)] + t1 * d[IX(i0 + 1, j0 + 1)]);
                    // a fixed 4x4 ordered dither: the cloud moves over it, it does not sparkle
                    int ax = px & 3, ay = py & 3, a1x = ax & 1, a1y = ay & 1, a2x = ax >> 1, a2y = ay >> 1;
                    f32 th = (4 * ((2 * a1x + 3 * a1y) & 3) + ((2 * a2x + 3 * a2y) & 3) + 0.5f) / 16.0f;
                    if (dv * 1.8f < th + 0.08f) continue;
                    u8 t = TileAtPx((f32)px, (f32)py);
                    if (TileSolid(t) || TileOneWay(t)) continue;
                    // paler than any wall, or the light snaps it to the wall behind it and it is gone
                    int pl = tint[k] > 0.5f ? (dv > 0.7f ? PL_STONEH : PL_STONEL) : (dv > 0.6f ? PL_BONE : PL_STONEH);
                    DrawRectangle(px, ROOM_Y + py, 1, 1, PAL[pl]);
                }
        }
}
