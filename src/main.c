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
static int  mute = 0;
static const char *wavPath = 0;
static long maxFrames = 0;
static float acc = 0.0f;

// ---------------------------------------------------------------- scripted input
// "R:60,RJ:8,-:30" = 60 frames right, 8 right+jump, 30 idle. Lets movement be
// regression-tested headlessly, with no browser and no hands.
#define PLAN_MAX 64
static struct { int mask, frames; } plan[PLAN_MAX];
static int planLen, planIdx, planLeft, planTotal;
enum { M_L = 1, M_R = 2, M_U = 4, M_D = 8, M_J = 16, M_X = 32, M_H = 64 };   // H: hold R (start over)

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
                case 'J': mask |= M_J; break; case 'X': mask |= M_X; break;
                case 'H': mask |= M_H; break; default: break;
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
static long homeFrame = 0;                 // first frame a bot dropped elsewhere stood on the start tile again
static float homeX, homeY;
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
    static int prevJump = 0, prevAct = 0;
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
        in.act = !!(m & M_X);
        in.actPressed = in.act && !prevAct;
        prevAct = in.act;
        in.reset = !!(m & M_H);
        return;
    }
    in.reset = IsKeyDown(KEY_R);
    in.left  = IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A);
    in.right = IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D);
    in.up    = IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W);
    in.down  = IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S);
    int jump = IsKeyDown(KEY_Z) || IsKeyDown(KEY_SPACE) || IsKeyDown(KEY_K);
    in.jump = jump;
    // X: hold / let go. Latched like the jump, for the same reason.
    int act = IsKeyDown(KEY_X);
    in.act = act;
    in.actPressed |= act && !prevAct;
    prevAct = act;
    // A render frame may have no physics tick. Keep its press (even if released
    // before the next tick) until PlayerStep has consumed it.
    in.jumpPressed |= jump && !prevJump;
    prevJump = jump;
}

// ---------------------------------------------------------------- starting over
// Where you began, as it was when you began. The rooms are rebuilt (RoomEnter does
// that), every held or dropped thing goes home, and the body is set on the start tile.
// There is no progress to keep yet; when there is, it is kept and this returns only
// what moves.
#define RESET_HOLD 90        // frames R is held to go through: a second and a half
#define RESET_WAKE 54        // frames for the room to come back after
f32 resetFade = 0.0f;
static int resetHeld, resetSpent, resetWaking;
long dbgResets = 0;

static void BeginAgain(void) {
    ItemsHome();
    PropsReset();
    RoomEnter(0);
    PlayerInit(RoomStartTx() * TS + 1.0f, (RoomStartTy() + 1) * TS - 11.0f);
    CameraInit();
    dbgResets++;
}

void ResetStep(void) {
    if (in.reset && !resetSpent) {
        resetHeld++;
        resetWaking = 0;
        f32 f = (f32)resetHeld / RESET_HOLD;
        if (f > resetFade) resetFade = f;            // never a jump back down when re-pressed mid-wake
        if (resetHeld >= RESET_HOLD) {
            BeginAgain();
            Sfx(SFX_WAKE, 0.9f, 1.0f + AudioRnd() * 0.04f, 0.5f);
            resetFade = 1.0f; resetHeld = 0;
            resetSpent = 1;                          // let go before it can happen again
            resetWaking = 1;
        }
    } else {
        if (!in.reset) resetSpent = 0;
        resetHeld = 0;
        // Let go early: it comes back at three times the speed it went. Waking: slower.
        resetFade -= resetWaking ? 1.0f / RESET_WAKE : 3.0f / RESET_HOLD;
        if (resetFade <= 0.0f) { resetFade = 0.0f; resetWaking = 0; }
    }
}

// Your lids, over everything the light pass produced -- the lamp's glass and the
// creatures' eyes included. Your own eyes are drawn after this, and close on their own.
void ResetDrawLids(void) {
    if (resetFade <= 0.0f) return;
    int a = (int)(resetFade * 255.0f);
    if (a > 255) a = 255;
    DrawRectangle(0, 0, GW, GH, (Color){ 0, 0, 0, (u8)a });
}

// ---------------------------------------------------------------- frame
static void Sim(void) {
    InputPoll();
    ResetStep();
    PlayerStep();
    CameraStep();
    ItemsStep();
    in.jumpPressed = 0;
    in.actPressed = 0;
    BulbsStep();
    WaterStep();
    FxStep();
    LifeStep();
    PropsStep();
    AirStep();
    if (wanderSeed && !homeFrame && frameNo > 60 && roomIdx == 0 && player.onGround
        && fabsf(player.x - homeX) < 12.0f && fabsf(player.y - homeY) < 4.0f) homeFrame = frameNo;
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
    dbgLastSfx = "-";
    // Toggles, once per rendered frame. Read inside the physics step they fired once per
    // tick, and a frame can run several ticks: one press of V flipped the look twice and
    // did nothing.
    if (!planLen && !wanderSeed) {
        if (IsKeyPressed(KEY_L)) dbgLabels = !dbgLabels;
        if (IsKeyPressed(KEY_V)) lookNew = !lookNew;    // the old look, for comparing
    }
    if (dbgFixedStep) {
        Sim();
    } else {
        acc += GetFrameTime();
        if (acc > 0.25f) acc = 0.25f;
        int steps = 0;
        while (acc >= DT && steps < 5) { Sim(); acc -= DT; steps++; }
        if (steps == 0) InputPoll();
    }
    AudioStep();
    if (!noDraw) {
        LightStep();
        RenderBegin();
            RoomDraw();
            PropsDrawFront();
            BulbsDraw();
            AirDraw();             // dust, smoke, mist: seen only where the light falls
            LifeDraw();
            ItemsDrawBehind();
            PlayerDraw();
            ItemsDrawHeld();
            FxDraw();
        if (LOOK_NEW) {
            RenderLayer(RL_EMIS);      // what gives its own light
                LifeDrawEyes();
                ItemsDrawCore();
                BackdropDrawEmis();
                PropsDrawEmis();
            RenderLayer(RL_BACK);      // the far city
                CityDraw();
            RenderComposite();
        } else {
            LightDraw();
            LifeDrawEyes();
            ItemsDrawCore();
            WorldEnd();
        }
            ResetDrawLids();
            WorldBegin();
                PlayerDrawEyes();  // over the lids: your own eyes close on their own, last
                DebugLabelsDraw();
            WorldEnd();
        RenderPresent();
    }

    if (dbgTrace)
        printf("f=%4ld x=%7.2f y=%7.2f vx=%6.3f vy=%6.3f ground=%d air=%d coy=%d buf=%d room=%d wet=%d sfx=%s hold=%d lamp=%d/%.0f,%.0f stone=%d/%.0f,%.0f fade=%.2f cam=%.0f,%.0f\n",
               frameNo, player.x, player.y, player.vx, player.vy,
               player.onGround, player.airFrames, player.coyote, player.jumpBuf,
               roomIdx, player.submerged, dbgLastSfx, PlayerHolds(),
               items[0].room, items[0].x, items[0].y,
               itemCount > 1 ? items[1].room : -1, itemCount > 1 ? items[1].x : 0.0f, itemCount > 1 ? items[1].y : 0.0f,
               resetFade, camX, camY);

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
    int lampRoom = 0, lampTx = 9, lampTy = 13;     // at your feet, at the foot of the door
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
        } else if (!strcmp(argv[i], "--albedo")) {
            dbgAlbedo = 1;
        } else if (!strcmp(argv[i], "--oldlook")) {
            lookNew = 0;
        } else if (!strcmp(argv[i], "--labels")) {
            dbgLabels = 1;
        } else if (!strcmp(argv[i], "--mute")) {
            mute = 1;
        } else if (!strcmp(argv[i], "--wav") && i + 1 < argc) {
            wavPath = argv[++i]; mute = 1;
        } else if (!strcmp(argv[i], "--frames") && i + 1 < argc) {
            maxFrames = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--scale") && i + 1 < argc) {
            winScale = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--wander") && i + 1 < argc) {
            wanderSeed = atoi(argv[++i]); dbgFixedStep = 1; noDraw = 1;
        } else if (!strcmp(argv[i], "--room") && i + 1 < argc) {
            startRoom = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--lamp") && i + 1 < argc) {
            lampRoom = atoi(strtok(argv[++i], ","));
            char *t = strtok(NULL, ","); if (t) lampTx = atoi(t);
            t = strtok(NULL, ","); if (t) lampTy = atoi(t);
        } else if (!strcmp(argv[i], "--at") && i + 1 < argc) {
            atx = atoi(strtok(argv[++i], ","));
            char *t = strtok(NULL, ","); if (t) aty = atoi(t);
        }
    }

    airOff = noDraw;          // the air is only ever looked at
    SetTraceLogLevel(LOG_WARNING);
    if (!dbgFixedStep) SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(GW * winScale, GH * winScale, "well");
    SetTargetFPS(dbgFixedStep ? 0 : 60);
    RenderInit();
    ItemsReset();
    ItemsAdd(IT_LAMP, lampRoom, lampTx, lampTy);   // items[0]: by default beside you where you begin
    RoomLoad();
    // Headless runs do not open a device: the container has none, and probing for one
    // is slow. Every sound still gets synthesized and counted, so the trace can say
    // what would have played.
    AudioInit(mute || noDraw);
    if (wavPath) { int ok = AudioExportMontage(wavPath); printf("%s -> %s\n", ok ? "wrote" : "FAILED", wavPath); CloseWindow(); return ok ? 0 : 1; }
    if (startRoom > 0 && startRoom < ROOM_COUNT) RoomEnter(startRoom);
    if (dbgLabels) DebugLabelsPrint();
    // P marks the tile you stand IN: feet on that tile's bottom edge.
    int tx = (atx >= 0) ? atx : RoomStartTx();
    int ty = (aty >= 0) ? aty : RoomStartTy();
    PlayerInit(tx * TS + 1.0f, (ty + 1) * TS - 11.0f);
    CameraInit();
    homeX = RoomStartTx() * TS + 1.0f; homeY = (RoomStartTy() + 1) * TS - 11.0f;
    

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(Frame, 0, 1);
#else
    while (!WindowShouldClose() && !shotsDone && !PlanExhausted()
           && !(maxFrames && frameNo >= maxFrames)) Frame();
#endif
    if (dbgTrace) LifePrintStats();
    if (wanderSeed) {
        // Every surface a body could rest on, per room, and whether this bot ever did.
        for (int r = 0; r < ROOM_COUNT; r++) {
            RoomEnter(r);
            int total = 0, hit = 0;
            for (int y = 1; y < RH; y++)
                for (int x = 1; x < RW - 1; x++) {
                    u8 t = TileGet(x, y), up = TileGet(x, y - 1);
                    if (!(TileSolid(t) || TileOneWay(t))) continue;
                    if (up != T_EMPTY && up != T_MOSS && up != T_WATER && up != T_BUSH) continue;
                    total++;
                    if (stood[r][y][x]) hit++;
                    else printf("unvisited surface room %d %d,%d\n", r, x, y);
                }
            printf("STOOD room %d: %d/%d surfaces  (seed %d, %ld frames)\n", r, hit, total, wanderSeed, frameNo);
        }
        // Whether, dropped wherever --at put it, this bot ever stood on the start tile again.
        // tools/escape.py asks that of every surface in the map.
        printf("HOME %s (%ld)\n", homeFrame ? "reached" : "never", homeFrame);
    }
    CloseWindow();
    return 0;
}
