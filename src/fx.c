#include "aw.h"
#include <math.h>

static Particle fx[FX_MAX];
static int fxNext = 0;
static u32 rngState = 0x2545F491u;

static float Rnd(void) {   // deterministic; screenshots and scripted plays must repeat
    rngState ^= rngState << 13; rngState ^= rngState >> 17; rngState ^= rngState << 5;
    return (float)(rngState & 0xFFFF) / 65535.0f;
}

void FxReset(void) {
    for (int i = 0; i < FX_MAX; i++) fx[i].life = 0;
    fxNext = 0; rngState = 0x2545F491u;
}

void FxSpawn(int kind, float x, float y, float vx, float vy, int life) {
    Particle *p = &fx[fxNext];
    fxNext = (fxNext + 1) % FX_MAX;      // ring; oldest is overwritten, never allocated
    p->x = x; p->y = y; p->vx = vx; p->vy = vy;
    p->life = p->maxLife = (u8)(life > 255 ? 255 : life);
    p->kind = (u8)kind;
}

void FxBurst(int kind, float x, float y, int n, float spread, float up) {
    for (int i = 0; i < n; i++) {
        float a = (Rnd() * 2.0f - 1.0f) * spread;
        FxSpawn(kind, x, y, a, -up * (0.4f + Rnd() * 0.8f), 18 + (int)(Rnd() * 26));
    }
}

void FxStep(void) {
    for (int i = 0; i < FX_MAX; i++) {
        Particle *p = &fx[i];
        if (!p->life) continue;
        p->life--;
        p->x += p->vx; p->y += p->vy;
        switch (p->kind) {
            case FX_DUST:     p->vy += 0.010f; p->vx *= 0.94f; break;
            case FX_SALT:     p->vy += 0.055f; p->vx *= 0.97f; break;
            case FX_DROP:     p->vy += 0.090f; break;
            case FX_SPLINTER: p->vy += 0.075f; p->vx *= 0.99f; break;
        }
        if (p->y > (float)(RH * TS)) p->life = 0;
    }
}

void FxDraw(void) {
    for (int i = 0; i < FX_MAX; i++) {
        Particle *p = &fx[i];
        if (!p->life) continue;
        float t = (float)p->life / (float)(p->maxLife ? p->maxLife : 1);
        Color c;
        switch (p->kind) {
            case FX_SALT:     c = palSalt;   break;
            case FX_DROP:     c = palBrineL; break;
            case FX_SPLINTER: c = palTimber; break;
            default:          c = palDust;   break;
        }
        // No alpha fade to a soft blur: quantise to three steps so it stays pixel art.
        if (t < 0.34f)      c = (Color){ c.r / 3, c.g / 3, c.b / 3, 255 };
        else if (t < 0.67f) c = (Color){ (u8)(c.r * 2 / 3), (u8)(c.g * 2 / 3), (u8)(c.b * 2 / 3), 255 };
        DrawRectangle((int)p->x, ROOM_Y + (int)p->y, 1, 1, c);
    }
}

// ---------------------------------------------------------------- room step
void RoomStepBegin(void) {
    for (int y = 0; y < RH; y++)
        for (int x = 0; x < RW; x++) scratch.loadMap[y][x] = 0;
}

void BrineDisturb(float px, float strength) {
    int c = (int)(px / TS);
    if (c < 0 || c >= RW) return;
    scratch.surfV[c] += strength;
    if (c > 0)      scratch.surfV[c - 1] += strength * 0.5f;
    if (c < RW - 1) scratch.surfV[c + 1] += strength * 0.5f;
}

void RoomStepEnd(void) {
    Room *r = CurRoom();

    // Crust fatigue. Stress accumulates only while something load>=2 rests on it, and
    // knits back when nothing does -- so a room you left recrystallises.
    for (int y = 0; y < RH; y++) {
        for (int x = 0; x < RW; x++) {
            if (r->tiles[y][x] != T_CRUST) continue;
            if (scratch.loadMap[y][x] >= CRUST_BREAK_LOAD) {
                if (scratch.stress[y][x] < 255) scratch.stress[y][x]++;
                if (scratch.stress[y][x] >= CRUST_STRESS_MAX) {
                    r->tiles[y][x] = T_EMPTY;
                    scratch.stress[y][x] = 0;
                    scratch.cornersDirty = 1;
                    FxBurst(FX_SALT, x * TS + TS * 0.5f, y * TS + TS * 0.5f, 7, 0.55f, 0.55f);
                }
            } else if (scratch.stress[y][x] > 0 && (frameNo & 7) == 0) {
                scratch.stress[y][x]--;
            }
        }
    }

    // 1D brine surface. Presentational only; no rule reads surfH.
    for (int x = 0; x < RW; x++) {
        float l = (x > 0)      ? scratch.surfH[x - 1] : scratch.surfH[x];
        float rr = (x < RW - 1) ? scratch.surfH[x + 1] : scratch.surfH[x];
        scratch.surfV[x] += (l + rr - 2.0f * scratch.surfH[x]) * 0.22f;
        scratch.surfV[x] -= scratch.surfH[x] * 0.010f;
        scratch.surfV[x] *= 0.955f;
    }
    for (int x = 0; x < RW; x++) scratch.surfH[x] += scratch.surfV[x];
}
