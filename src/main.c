#include "aw.h"
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
static Arena globalArena, sessionArena, roomArena;

Input in;
long frameNo = 0;

int dbgShotFrames[16];
int dbgShotCount = 0;
const char *dbgOutDir = "shots";
int dbgFixedStep = 0;
static int dbgTrace = 0;
static int startTx = 2, startTy = 17, startLoad = 0;
static int dbgShotsDone = 0;
static float acc = 0.0f;

// ---------------------------------------------------------------- input
// A scripted plan lets movement be regression-tested headlessly, with no browser.
// "R:60,RJ:8,R:40,-:30" = 60 frames right, 8 right+jump, 40 right, 30 idle.
#define PLAN_MAX 64
static struct { int mask, frames; } plan[PLAN_MAX];
static int planLen = 0, planIdx = 0, planLeft = 0;
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

// ---------------------------------------------------------------- sandbox room
// Phase 2 has one room and it is a place to play with weight, not a design.
// Left: a crust shelf over a drop, so light walks where heavy falls through.
// Middle: the rim of a shallow pan you can fill and empty at.
// Right: a deep pan whose floor you can only reach heavy -- and whose rim sits over
// the deep end, which is where filling starts fighting itself.
static void BuildSandbox(void) {
    ArenaReset(&roomArena);
    Room *r = RoomAt(2, 2);
    memset(r->tiles, 0, sizeof r->tiles);
    memset(&scratch, 0, sizeof scratch);
    FxReset();
    r->exists = 1;
    world.cx = 2; world.cy = 2;

    for (int x = 0; x < RW; x++) { r->tiles[0][x] = T_ROCK; r->tiles[RH - 1][x] = T_ROCK; }
    for (int y = 0; y < RH; y++) { r->tiles[y][0] = T_ROCK; r->tiles[y][RW - 1] = T_ROCK; }
    for (int x = 1; x < RW - 1; x++) { r->tiles[19][x] = T_ROCK; r->tiles[20][x] = T_ROCK; }

    // --- LEFT: a crust bridge flush with the floor over a two-tile pit.
    // Reachable at any load, which is the point: light it holds, heavy it does not,
    // and the only way to find that out is to stand on it and wait.
    for (int x = 5; x < 12; x++) { r->tiles[19][x] = T_CRUST; r->tiles[20][x] = T_EMPTY; }

    // --- a crust shelf four tiles up. A load-0 jump rises 38 px and clears it; a
    // load-1 jump rises 29 px and does not. A plane only the empty player has stood on.
    for (int x = 3; x < 13; x++) r->tiles[15][x] = T_CRUST;

    // --- scaffold, four tiles above the shelf: same rule again, one storey up
    for (int x = 8; x < 17; x++) r->tiles[11][x] = T_TIMBER;

    // --- MIDDLE: a shallow pan flush with the floor, a rim on each lip. Two tiles
    // deep, so you can stand in it at any load. The safe place to learn the dial.
    for (int x = 16; x < 25; x++) { r->tiles[19][x] = T_BRINE; r->tiles[20][x] = T_BRINE; }
    r->tiles[19][15] = T_RIM;
    r->tiles[19][25] = T_RIM;

    // --- RIGHT: a terrace, reached by single-tile steps so every load can climb it.
    r->tiles[18][26] = T_ROCK;
    r->tiles[17][27] = T_ROCK;  r->tiles[18][27] = T_ROCK;
    r->tiles[16][28] = T_ROCK;  r->tiles[17][28] = T_ROCK;  r->tiles[18][28] = T_ROCK;
    for (int y = 15; y < 21; y++) r->tiles[y][29] = T_ROCK;
    for (int y = 15; y < 21; y++) r->tiles[y][38] = T_ROCK;

    // --- the deep pan: five tiles of brine. Empty you float on it and cannot descend;
    // full you sink and walk its floor. There is a rim down there, so the sandbox
    // lets you back out -- the real game's bell room will not.
    for (int y = 16; y < 21; y++)
        for (int x = 30; x < 38; x++) r->tiles[y][x] = T_BRINE;
    r->tiles[15][29] = T_RIM;
    r->tiles[20][33] = T_RIM;  r->tiles[20][34] = T_RIM;

    // --- a black shelf, because any black tile may be hiding something
    for (int x = 19; x < 25; x++) r->tiles[4][x] = T_DARK;

    // --- salt fringe on dry stone
    for (int x = 1; x < RW - 1; x++)
        if (r->tiles[19][x] == T_ROCK && r->tiles[18][x] == T_EMPTY && (x * 7 % 5) == 0)
            r->tiles[18][x] = T_GRASS;

    PlayerInit(startTx * TS, startTy * TS);
    player.load = (u8)startLoad;
}

// ---------------------------------------------------------------- frame
static void Sim(void) {
    InputPoll();
    RoomStepBegin();
    PlayerStep();
    RoomStepEnd();
    FxStep();
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
        PlayerDraw();
        FxDraw();
    RenderEndRoomAndPresent();

    if (dbgTrace)
        printf("f=%4ld x=%6.2f y=%6.2f vx=%6.3f vy=%6.3f g=%d load=%d rim=%d sub=%d\n",
               frameNo, player.x, player.y, player.vx, player.vy,
               player.onGround, player.load, player.atRim, player.submerged);

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
    BuildSandbox();

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(Frame, 0, 1);
#else
    while (!WindowShouldClose() && !dbgShotsDone) Frame();
#endif
    CloseWindow();
    return 0;
}
