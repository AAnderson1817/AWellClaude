#include "aw.h"
#include "rooms_gen.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

// ---------------------------------------------------------------- memory
// One static block for the whole process. Nothing else allocates, ever.
#define GLOBAL_BYTES (8u * 1024u * 1024u)
static u8 globalBlock[GLOBAL_BYTES];
Arena globalArena, sessionArena, roomArena;

Input in;
long frameNo = 0;

int dbgShotFrames[16];
int dbgShotCount = 0;
const char *dbgOutDir = "shots";
int dbgFixedStep = 0;
static int dbgTrace = 0;
static int startTx = -1, startTy = -1, startLoad = 0;
static int startRx = START_ROOM_X, startRy = START_ROOM_Y;
static const char *contactPath = 0;
static long maxFrames = 0;
static int visitReport = 0;
static int noDraw = 0;
static u8 visited[ROOM_COUNT];
static int dbgShotsDone = 0;
static float acc = 0.0f;

// ---------------------------------------------------------------- input
// A scripted plan lets movement be regression-tested headlessly, with no browser.
// "R:60,RJ:8,R:40,-:30" = 60 frames right, 8 right+jump, 40 right, 30 idle.
#define PLAN_MAX 64
static struct { int mask, frames; } plan[PLAN_MAX];
static int planLen = 0, planIdx = 0, planLeft = 0, planTotal = 0;
int PlanExhausted(void) { return planLen && frameNo > planTotal + 2; }
enum { M_L = 1, M_R = 2, M_U = 4, M_D = 8, M_J = 16, M_A = 32, M_B = 64 };

static void ParsePlan(char *s) {
    char *tok = strtok(s, ",");
    while (tok && planLen < PLAN_MAX) {
        int mask = 0; char *colon = strchr(tok, ':');
        int n = colon ? atoi(colon + 1) : 1;
        for (char *c = tok; *c && c != colon; c++) {
            switch (*c) {
                case 'L': mask |= M_L; break;  case 'R': mask |= M_R; break;
                case 'U': mask |= M_U; break;  case 'D': mask |= M_D; break;
                case 'J': mask |= M_J; break;  case 'A': mask |= M_A; break;
                case 'B': mask |= M_B; break;  default: break;
            }
        }
        plan[planLen].mask = mask; plan[planLen].frames = n; planLen++;
        tok = strtok(NULL, ",");
    }
    if (planLen) planLeft = plan[0].frames;
    for (int i = 0; i < planLen; i++) planTotal += plan[i].frames;
}

// A wandering bot. Not clever: it holds a direction for a while, jumps sometimes, grabs
// sometimes, fills sometimes -- roughly the shape of a person with no plan. Deterministic
// from its seed so a run can be repeated exactly.
int wanderSeed = 0;
static u32 wrng;
static int wDir, wHold, wJump, wGaff, wKett, wUp, wDown;
static float WRnd(void) {
    wrng ^= wrng << 13; wrng ^= wrng >> 17; wrng ^= wrng << 5;
    return (float)(wrng & 0xFFFF) / 65535.0f;
}

void InputPoll(void) {
    static int prevJump = 0;
    if (wanderSeed) {
        static float lastX, lastY; static int stuckFor, climbFor;
        if (!wrng) wrng = (u32)wanderSeed * 2654435761u + 1u;
        if (--wHold <= 0) {
            wHold = 14 + (int)(WRnd() * 70);
            float p = WRnd();
            wDir  = p < 0.45f ? 1 : (p < 0.90f ? -1 : 0);
            wJump = WRnd() < 0.40f;
            wGaff = WRnd() < 0.25f;
            wKett = WRnd() < 0.30f;
            wUp   = WRnd() < 0.30f;
            wDown = WRnd() < 0.15f;
        }
        // The one thing a bot needs to stop being useless: if it is pushing against
        // something and going nowhere, try going over it. Without this it never climbs,
        // and a test that never climbs cannot tell a hard route from a sealed one.
        if (fabsf(player.x - lastX) < 0.3f && fabsf(player.y - lastY) < 0.3f) stuckFor++;
        else stuckFor = 0;
        lastX = player.x; lastY = player.y;
        if (stuckFor > 10 && climbFor <= 0) { climbFor = 26 + (int)(WRnd() * 34); stuckFor = 0; }
        if (climbFor > 0) climbFor--;

        // While climbing, sweep back and forth: nearly every ladder in this world is
        // landings on alternating sides, and a bot that only ever pushes one way stalls
        // against the first wall it meets and never finds the next step.
        int cdir = wDir;
        if (climbFor > 0) cdir = ((frameNo / 22) & 1) ? 1 : -1;
        in.left = cdir < 0; in.right = cdir > 0;
        in.up = wUp || (climbFor > 0 && (frameNo & 3) == 0);
        in.down = wDown && climbFor <= 0;
        // hold the jump through a rise (height is variable) but let it re-arm
        int want = (climbFor > 0) ? ((frameNo % 26) < 17) : (wJump && ((frameNo / 9) & 1));
        in.jump = want;
        in.jumpPressed = in.jump && !prevJump;
        in.a = wKett; in.b = wGaff;
        prevJump = in.jump;
        return;
    }
    if (planLen) {
        while (planIdx < planLen && planLeft <= 0) { planIdx++; if (planIdx < planLen) planLeft = plan[planIdx].frames; }
        int m = (planIdx < planLen) ? plan[planIdx].mask : 0;
        planLeft--;
        in.left = !!(m & M_L); in.right = !!(m & M_R);
        in.up   = !!(m & M_U); in.down  = !!(m & M_D);
        in.jump = !!(m & M_J);
        in.jumpPressed = in.jump && !prevJump;
        in.a = !!(m & M_A); in.b = !!(m & M_B);
        prevJump = in.jump;
        return;
    }
    int left  = IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A);
    int right = IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D);
    int up    = IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W);
    int down  = IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S);
    int jump  = IsKeyDown(KEY_Z) || IsKeyDown(KEY_SPACE) || IsKeyDown(KEY_K);
    in.left = left; in.right = right; in.up = up; in.down = down;
    in.jump = jump;
    in.jumpPressed = jump && !prevJump;
    in.a = IsKeyDown(KEY_X) || IsKeyDown(KEY_L);
    in.b = IsKeyDown(KEY_C) || IsKeyDown(KEY_J);
    prevJump = jump;
}

// ---------------------------------------------------------------- frame
static void Sim(void) {
    InputPoll();
    RoomStepBegin();
    PlayerStep();
    RoomStepEnd();
    FxStep();
    RoomTransition();
}

static void Frame(void) {
    frameNo++;
    visited[world.cy * WORLD_W + world.cx] = 1;

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
        RenderBeginRoom();
            DrawRoom(CurRoom());
                PlayerDraw();
            FxDraw();
        RenderEndRoomAndPresent();
    }

    if (dbgTrace)
        printf("f=%4ld r=%d,%d x=%6.2f y=%6.2f vx=%6.3f vy=%6.3f g=%d load=%d sub=%d\n",
               frameNo, world.cx, world.cy, player.x, player.y, player.vx, player.vy,
               player.onGround, player.load, player.submerged);

    for (int i = 0; i < dbgShotCount; i++) {
        if (dbgShotFrames[i] == (int)frameNo) {
            char path[256];
            snprintf(path, sizeof path, "%s/f%04d.png", dbgOutDir, (int)frameNo);
            // NOT TakeScreenshot(): raylib discards the directory component.
            Image img = LoadImageFromScreen();
            ExportImage(img, path);
            UnloadImage(img);
            printf("shot %s\n", path); fflush(stdout);
            if (i == dbgShotCount - 1) dbgShotsDone = 1;
        }
    }
}

int main(int argc, char **argv) {
    int winScale = 4;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--shots") && i + 1 < argc) {
            char *tok = strtok(argv[++i], ",");
            while (tok && dbgShotCount < 16) { dbgShotFrames[dbgShotCount++] = atoi(tok); tok = strtok(NULL, ","); }
            dbgFixedStep = 1;
        } else if (!strcmp(argv[i], "--out") && i + 1 < argc) {
            dbgOutDir = argv[++i];
        } else if (!strcmp(argv[i], "--scale") && i + 1 < argc) {
            winScale = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--play") && i + 1 < argc) {
            ParsePlan(argv[++i]);
            dbgFixedStep = 1;
        } else if (!strcmp(argv[i], "--trace")) {
            dbgTrace = 1;
        } else if (!strcmp(argv[i], "--at") && i + 1 < argc) {
            startTx = atoi(strtok(argv[++i], ","));
            char *t = strtok(NULL, ","); if (t) startTy = atoi(t);
        } else if (!strcmp(argv[i], "--load") && i + 1 < argc) {
            startLoad = atoi(argv[++i]);
        } else if (0) {
        } else if (!strcmp(argv[i], "--wander") && i + 1 < argc) {
            extern int wanderSeed; wanderSeed = atoi(argv[++i]);
            dbgFixedStep = 1; visitReport = 1; noDraw = 1;
        } else if (!strcmp(argv[i], "--frames") && i + 1 < argc) {
            maxFrames = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--contact") && i + 1 < argc) {
            contactPath = argv[++i];
        } else if (!strcmp(argv[i], "--room") && i + 1 < argc) {
            startRx = atoi(strtok(argv[++i], ","));
            char *t = strtok(NULL, ","); if (t) startRy = atoi(t);
        }
    }

    ArenaInit(&globalArena, globalBlock, GLOBAL_BYTES);
    u8 *sess = (u8 *)ArenaPush(&globalArena, 4u * 1024u * 1024u, 16);
    ArenaInit(&sessionArena, sess, 4u * 1024u * 1024u);
    u8 *rm = (u8 *)ArenaPush(&sessionArena, 1u * 1024u * 1024u, 16);
    ArenaInit(&roomArena, rm, 1u * 1024u * 1024u);

    SetTraceLogLevel(LOG_WARNING);
    // Headless runs are deterministic and fixed-step, so there is no reason to sit at
    // 60 fps waiting for a wall clock nobody is watching.
    if (!dbgFixedStep) SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(GW * winScale, GH * winScale, "well");
    SetTargetFPS(dbgFixedStep ? 0 : 60);
    RenderInit();
    LoadWorld();
    world.cx = startRx; world.cy = startRy;
    RoomArenaFresh();
    FxReset();
    // P marks the tile the player stands IN: feet at that tile's bottom edge.
    int ptx = (startTx >= 0 ? startTx : START_TILE_X);
    int pty = (startTy >= 0 ? startTy : START_TILE_Y);
    PlayerInit(ptx * TS + 1, (pty + 1) * TS - 12);
    player.load = (u8)startLoad;

    if (contactPath) {
        // One image of the whole world, so 25 rooms can actually be looked at.
        Image sheet = GenImageColor(GW * WORLD_W, GH * WORLD_H, BLACK);
        for (int ry = 0; ry < WORLD_H; ry++) {
            for (int rx = 0; rx < WORLD_W; rx++) {
                world.cx = rx; world.cy = ry;
                RoomArenaFresh();
                BeginTextureMode(screenRT);
                    ClearBackground(palBgDeep);
                    DrawRoom(CurRoom());
                EndTextureMode();
                Image im = LoadImageFromTexture(screenRT.texture);
                ImageFlipVertical(&im);
                ImageDraw(&sheet, im,
                          (Rectangle){0, 0, (float)GW, (float)GH},
                          (Rectangle){(float)(rx * GW), (float)(ry * GH), (float)GW, (float)GH},
                          WHITE);
                UnloadImage(im);
            }
        }
        ExportImage(sheet, contactPath);
        UnloadImage(sheet);
        printf("contact sheet -> %s\n", contactPath);
        CloseWindow();
        return 0;
    }

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(Frame, 0, 1);
#else
    while (!WindowShouldClose() && !dbgShotsDone && !PlanExhausted()
           && !(maxFrames && frameNo >= maxFrames)) Frame();
#endif
    if (visitReport) {
        int n = 0;
        for (int i = 0; i < ROOM_COUNT; i++) if (visited[i]) n++;
        printf("VISITED %d/25\n", n);
        for (int y = 0; y < WORLD_H; y++) {
            printf("  ");
            for (int x = 0; x < WORLD_W; x++) printf("%s", visited[y * WORLD_W + x] ? " ## " : " .. ");
            printf("\n");
        }
    }
    CloseWindow();
    return 0;
}
