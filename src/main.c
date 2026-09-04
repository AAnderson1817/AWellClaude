// main.c -- window, fixed timestep, and the handful of switches the headless
// verification runs need. Nothing allocates; there is nothing to allocate.
#include "aw.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

Input in;
long frameNo = 0;
int  dbgFixedStep = 0;
int  dbgLabels = 0;
const char *dbgOutDir = "shots";

static int  shotFrames[16], shotCount, shotsDone;
static int  dbgTrace = 0;
static int  noDraw = 0;   // the wanderer runs a million frames; it does not need pictures
static long maxFrames = 0;
static float acc = 0.0f;

// ---------------------------------------------------------------- scripted input
// "R:60,RJ:8,-:30" = 60 frames right, 8 right+jump, 30 idle. Lets movement be
// regression-tested headlessly, with no browser and no hands.
#define PLAN_MAX 64
static struct { int mask, frames; } plan[PLAN_MAX];
static int planLen, planIdx, planLeft, planTotal;
enum { M_L = 1, M_R = 2, M_U = 4, M_D = 8, M_J = 16 };

static void ParsePlan(char *s) {
    char *tok = strtok(s, ",");
    while (tok && planLen < PLAN_MAX) {
        int mask = 0;
        char *colon = strchr(tok, ':');
        int n = colon ? atoi(colon + 1) : 1;
        for (char *c = tok; *c && c != colon; c++) {
            switch (*c) {
                case 'L': mask |= M_L; break; case 'R': mask |= M_R; break;
                case 'U': mask |= M_U; break; case 'D': mask |= M_D; break;
                case 'J': mask |= M_J; break; default: break;
            }
        }
        plan[planLen].mask = mask; plan[planLen].frames = n; planLen++;
        tok = strtok(NULL, ",");
    }
    if (planLen) planLeft = plan[0].frames;
    for (int i = 0; i < planLen; i++) planTotal += plan[i].frames;
}
static int PlanExhausted(void) { return planLen && frameNo > planTotal + 2; }

// ---------------------------------------------------------------- the wanderer
// Not clever: it holds a direction for a while, jumps sometimes, and tries going
// over whatever it is stuck against. It exists because I designed the room, so I
// am the worst possible judge of whether the room can be climbed. Deterministic
// from its seed, so a failure can be replayed exactly.
static int wanderSeed = 0;
static u8 stood[ROOM_COUNT][RH][RW];
static u32 wrng;
static float WRnd(void) {
    wrng ^= wrng << 13; wrng ^= wrng >> 17; wrng ^= wrng << 5;
    return (float)(wrng & 0xFFFF) / 65535.0f;
}
static void WanderPoll(void) {
    static int prevJump, dir, hold, jumpy, downy, stuck, climb;
    static float lx, ly;
    if (!wrng) wrng = (u32)wanderSeed * 2654435761u + 1u;
    if (--hold <= 0) {
        hold = 16 + (int)(WRnd() * 60);
        float p = WRnd();
        dir   = p < 0.44f ? 1 : (p < 0.88f ? -1 : 0);
        jumpy = WRnd() < 0.55f;
        downy = WRnd() < 0.18f;
    }
    if (fabsf(player.x - lx) < 0.3f && fabsf(player.y - ly) < 0.3f) stuck++; else stuck = 0;
    lx = player.x; ly = player.y;
    if (stuck > 10 && climb <= 0) { climb = 30 + (int)(WRnd() * 40); stuck = 0; }
    if (climb > 0) climb--;
    // While climbing, sweep both ways: the shelves here alternate sides, and a bot
    // that only ever pushes one way stalls on the first wall and never finds the next.
    int d = (climb > 0) ? (((frameNo / 24) & 1) ? 1 : -1) : dir;
    in.left = d < 0; in.right = d > 0;
    in.up = 0;
    in.down = downy && climb <= 0;
    in.jump = (climb > 0) ? ((frameNo % 28) < 18) : (jumpy && ((frameNo / 10) & 1));
    in.jumpPressed = in.jump && !prevJump;
    prevJump = in.jump;
}

void InputPoll(void) {
    static int prevJump = 0;
    if (wanderSeed) { WanderPoll(); return; }
    if (planLen) {
        while (planIdx < planLen && planLeft <= 0) {
            planIdx++;
            if (planIdx < planLen) planLeft = plan[planIdx].frames;
        }
        int m = (planIdx < planLen) ? plan[planIdx].mask : 0;
        planLeft--;
        in.left = !!(m & M_L); in.right = !!(m & M_R);
        in.up   = !!(m & M_U); in.down  = !!(m & M_D);
        in.jump = !!(m & M_J);
        in.jumpPressed = in.jump && !prevJump;
        prevJump = in.jump;
        return;
    }
    if (IsKeyPressed(KEY_L)) dbgLabels = !dbgLabels;
    in.left  = IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A);
    in.right = IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D);
    in.up    = IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W);
    in.down  = IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S);
    int jump = IsKeyDown(KEY_Z) || IsKeyDown(KEY_SPACE) || IsKeyDown(KEY_K)
            || IsKeyDown(KEY_X) || IsKeyDown(KEY_C);
    in.jump = jump;
    // A render frame may have no physics tick. Keep its press (even if released
    // before the next tick) until PlayerStep has consumed it.
    in.jumpPressed |= jump && !prevJump;
    prevJump = jump;
}

// ---------------------------------------------------------------- frame
static void Sim(void) {
    InputPoll();
    PlayerStep();
    in.jumpPressed = 0;
    BulbsStep();
    WaterStep();
    FxStep();
    if (wanderSeed && player.onGround) {
        // Half a pixel BELOW the feet, not at them. Landing on stone leaves the feet
        // a fraction past the tile top; landing on a shelf stops them a fraction
        // short of it. Reading the tile at the feet therefore names the empty tile
        // above every shelf -- which is why the first run of this said no bot had
        // ever stood on a shelf, when in fact they had.
        int ty = (int)floorf((player.y + player.h + 0.5f) / TS);
        int x0 = (int)floorf(player.x / TS), x1 = (int)floorf((player.x + player.w - 1) / TS);
        for (int tx = x0; tx <= x1; tx++)
            if (tx >= 0 && tx < RW && ty >= 0 && ty < RH) stood[roomIdx][ty][tx] = 1;
    }
}

static void Frame(void) {
    frameNo++;
    if (dbgFixedStep) {
        Sim();
    } else {
        acc += GetFrameTime();
        if (acc > 0.25f) acc = 0.25f;
        int steps = 0;
        while (acc >= DT && steps < 5) { Sim(); acc -= DT; steps++; }
        if (steps == 0) InputPoll();
    }
    if (!noDraw) {
        LightStep();
        RenderBegin();
            RoomDraw();
            BulbsDraw();
            PlayerDraw();
            FxDraw();
            LightDraw();
            PlayerDrawEyes();
            DebugLabelsDraw();
        RenderPresent();
    }

    if (dbgTrace)
        printf("f=%4ld x=%7.2f y=%7.2f vx=%6.3f vy=%6.3f ground=%d air=%d coy=%d buf=%d room=%d wet=%d\n",
               frameNo, player.x, player.y, player.vx, player.vy,
               player.onGround, player.airFrames, player.coyote, player.jumpBuf,
               roomIdx, player.submerged);

    for (int i = 0; i < shotCount; i++)
        if (shotFrames[i] == (int)frameNo) {
            char path[256];
            snprintf(path, sizeof path, "%s/f%04d.png", dbgOutDir, (int)frameNo);
            // NOT TakeScreenshot(): raylib throws away the directory component.
            Image img = LoadImageFromScreen();
            ExportImage(img, path);
            UnloadImage(img);
            printf("shot %s\n", path);
            fflush(stdout);
            if (i == shotCount - 1) shotsDone = 1;
        }
}

int main(int argc, char **argv) {
    int winScale = 4, atx = -1, aty = -1, startRoom = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--shots") && i + 1 < argc) {
            char *tok = strtok(argv[++i], ",");
            while (tok && shotCount < 16) { shotFrames[shotCount++] = atoi(tok); tok = strtok(NULL, ","); }
            dbgFixedStep = 1;
        } else if (!strcmp(argv[i], "--out") && i + 1 < argc) {
            dbgOutDir = argv[++i];
        } else if (!strcmp(argv[i], "--play") && i + 1 < argc) {
            ParsePlan(argv[++i]); dbgFixedStep = 1;
        } else if (!strcmp(argv[i], "--trace")) {
            dbgTrace = 1;
        } else if (!strcmp(argv[i], "--nodraw")) {
            noDraw = 1;
        } else if (!strcmp(argv[i], "--labels")) {
            dbgLabels = 1;
        } else if (!strcmp(argv[i], "--frames") && i + 1 < argc) {
            maxFrames = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--scale") && i + 1 < argc) {
            winScale = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--wander") && i + 1 < argc) {
            wanderSeed = atoi(argv[++i]); dbgFixedStep = 1; noDraw = 1;
        } else if (!strcmp(argv[i], "--room") && i + 1 < argc) {
            startRoom = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--at") && i + 1 < argc) {
            atx = atoi(strtok(argv[++i], ","));
            char *t = strtok(NULL, ","); if (t) aty = atoi(t);
        }
    }

    SetTraceLogLevel(LOG_WARNING);
    if (!dbgFixedStep) SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(GW * winScale, GH * winScale, "well");
    SetTargetFPS(dbgFixedStep ? 0 : 60);
    RenderInit();
    RoomLoad();
    if (startRoom > 0 && startRoom < ROOM_COUNT) RoomEnter(startRoom);
    if (dbgLabels) DebugLabelsPrint();
    // P marks the tile you stand IN: feet on that tile's bottom edge.
    int tx = (atx >= 0) ? atx : RoomStartTx();
    int ty = (aty >= 0) ? aty : RoomStartTy();
    PlayerInit(tx * TS + 1.0f, (ty + 1) * TS - 11.0f);

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(Frame, 0, 1);
#else
    while (!WindowShouldClose() && !shotsDone && !PlanExhausted()
           && !(maxFrames && frameNo >= maxFrames)) Frame();
#endif
    if (wanderSeed) {
        // Every surface a body could rest on, per room, and whether this bot ever did.
        for (int r = 0; r < ROOM_COUNT; r++) {
            RoomEnter(r);
            int total = 0, hit = 0;
            for (int y = 1; y < RH; y++)
                for (int x = 1; x < RW - 1; x++) {
                    u8 t = TileGet(x, y), up = TileGet(x, y - 1);
                    if (!(TileSolid(t) || TileOneWay(t))) continue;
                    if (up != T_EMPTY && up != T_MOSS && up != T_WATER) continue;
                    total++;
                    if (stood[r][y][x]) hit++;
                    else printf("unvisited surface room %d %d,%d\n", r, x, y);
                }
            printf("STOOD room %d: %d/%d surfaces  (seed %d, %ld frames)\n", r, hit, total, wanderSeed, frameNo);
        }
    }
    CloseWindow();
    return 0;
}
