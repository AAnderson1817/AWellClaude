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

// ---------------------------------------------------------------- test room
// Phase 1 has exactly one room. It is a movement gym, not a design: flat ground,
// gaps at 2/3/4 tiles to calibrate jump distance, a ledge stack to test coyote
// time and one-way platforms, a water column, and a black shelf.
static void BuildGymRoom(void) {
    ArenaReset(&roomArena);
    Room *r = RoomAt(2, 2);
    memset(r->tiles, 0, sizeof r->tiles);
    r->exists = 1;
    world.cx = 2; world.cy = 2;

    for (int x = 0; x < RW; x++) { r->tiles[RH - 1][x] = T_ROCK; r->tiles[RH - 2][x] = T_ROCK; }
    for (int y = 0; y < RH; y++) { r->tiles[y][0] = T_ROCK; r->tiles[y][RW - 1] = T_ROCK; }
    for (int x = 0; x < RW; x++) r->tiles[0][x] = T_ROCK;

    // gaps of 2, 3 and 4 tiles in the floor
    for (int x = 6;  x < 8;  x++) { r->tiles[RH - 1][x] = T_EMPTY; r->tiles[RH - 2][x] = T_EMPTY; }
    for (int x = 13; x < 16; x++) { r->tiles[RH - 1][x] = T_EMPTY; r->tiles[RH - 2][x] = T_EMPTY; }
    for (int x = 22; x < 26; x++) { r->tiles[RH - 1][x] = T_EMPTY; r->tiles[RH - 2][x] = T_EMPTY; }

    // step ladder: 1, 2 and 3 tile rises
    r->tiles[RH - 3][30] = T_ROCK;
    r->tiles[RH - 3][31] = T_ROCK; r->tiles[RH - 4][31] = T_ROCK;
    for (int y = RH - 5; y < RH - 2; y++) r->tiles[y][32] = T_ROCK;

    // one-way ledges to fall through and coyote-jump from
    for (int x = 4;  x < 10; x++) r->tiles[RH - 7][x] = T_LEDGE;
    for (int x = 12; x < 18; x++) r->tiles[RH - 11][x] = T_LEDGE;

    // a black shelf: any black tile may be hiding something
    for (int x = 20; x < 27; x++) r->tiles[RH - 9][x] = T_DARK;

    // water column on the right
    for (int y = RH - 8; y < RH - 2; y++) for (int x = 34; x < 38; x++) r->tiles[y][x] = T_WATER;
    for (int y = RH - 8; y < RH - 2; y++) r->tiles[y][33] = T_ROCK;

    for (int x = 1; x < RW - 1; x++)
        if (r->tiles[RH - 2][x] == T_ROCK && r->tiles[RH - 3][x] == T_EMPTY && (x * 7 % 5) == 0)
            r->tiles[RH - 3][x] = T_GRASS;

    PlayerInit(3 * TS, (RH - 4) * TS);
}

// ---------------------------------------------------------------- frame
static void Sim(void) { InputPoll(); PlayerStep(); }

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
    RenderEndRoomAndPresent();

    if (dbgTrace && (frameNo % 6 == 0))
        printf("f=%4ld x=%7.2f y=%7.2f vx=%6.3f vy=%6.3f g=%d coy=%d buf=%d w=%d\n",
               frameNo, player.x, player.y, player.vx, player.vy,
               player.onGround, player.coyote, player.jumpBuf, player.inWater);

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
    BuildGymRoom();

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(Frame, 0, 1);
#else
    while (!WindowShouldClose() && !dbgShotsDone) Frame();
#endif
    CloseWindow();
    return 0;
}
