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

void InputPoll(void) {
    static int prevJump = 0;
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
    // Both verbs delete tiles, and corners are derived from tiles. Rebuilding here is
    // the third shared line of the coupling: crust that fails under weight manufactures
    // the hooks the gaff needs, and a sheared nub exposes its neighbours.
    if (scratch.cornersDirty) { BuildCorners(); scratch.cornersDirty = 0; }
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

    RenderBeginRoom();
        DrawRoom(CurRoom());
        GaffDraw();
        PlayerDraw();
        FxDraw();
    RenderEndRoomAndPresent();

    if (dbgTrace)
        printf("f=%4ld r=%d,%d x=%6.2f y=%6.2f vx=%6.3f vy=%6.3f g=%d load=%d rim=%d sub=%d hook=%d corners=%d\n",
               frameNo, world.cx, world.cy, player.x, player.y, player.vx, player.vy,
               player.onGround, player.load, player.atRim, player.submerged,
               player.hooked, (int)scratch.cornerCount);

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
        } else if (!strcmp(argv[i], "--drop")) {
            extern int dbgReleaseAtBottom; dbgReleaseAtBottom = 1;
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
    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(GW * winScale, GH * winScale, "well");
    SetTargetFPS(60);
    RenderInit();
    LoadWorld();
    world.cx = startRx; world.cy = startRy;
    ArenaReset(&roomArena);
    memset(&scratch, 0, sizeof scratch);
    FxReset();
    BuildCorners();
    // P marks the tile the player stands IN: feet at that tile's bottom edge.
    int ptx = (startTx >= 0 ? startTx : START_TILE_X);
    int pty = (startTy >= 0 ? startTy : START_TILE_Y);
    PlayerInit(ptx * TS + 1, (pty + 1) * TS - 12);
    player.load = (u8)startLoad;

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(Frame, 0, 1);
#else
    while (!WindowShouldClose() && !dbgShotsDone && !PlanExhausted()) Frame();
#endif
    CloseWindow();
    return 0;
}
