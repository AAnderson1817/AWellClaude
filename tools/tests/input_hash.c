// Exercise the real frame loop and simulation; raylib_stubs.c supplies only I/O.
#define main game_main
#include "../../src/main.c"
#undef main
#include <assert.h>
#include <limits.h>

extern float testFrameTime;
extern int testJumpDown;

static void Reset(void) {
    memset(tiles, 0, sizeof tiles);
    for (int x = 0; x < RW; x++) tiles[20][x] = T_ROCK;
    bulbCount = 0;
    PlayerInit(65, 149);
    memset(&in, 0, sizeof in);
    testJumpDown = 0;
    InputPoll();
    acc = 0;
    frameNo = 0;
    noDraw = 1;
    dbgFixedStep = 0;
}

int main(void) {
    Reset();
    testJumpDown = 1; testFrameTime = DT; Frame();
    assert(player.vy < 0 && player.jumpHeld && !player.onGround && !in.jumpPressed);

    // A held press on a render-only frame must reach the next physics tick.
    Reset();
    testJumpDown = 1; testFrameTime = DT * 0.25f; Frame();
    assert(in.jumpPressed && player.y == 149);
    testFrameTime = DT * 0.75f; Frame();
    assert(player.vy < 0 && player.jumpHeld && !player.onGround && !in.jumpPressed);

    // A tap released before simulation still causes a short jump.
    Reset();
    testJumpDown = 1; testFrameTime = DT * 0.25f; Frame();
    testJumpDown = 0; Frame();
    assert(in.jumpPressed);
    testFrameTime = DT; Frame();
    assert(player.vy < 0 && !player.jumpHeld && !in.jumpPressed);   // released: the cut applied

    // Catch-up ticks must not treat one held press as repeated presses.
    Reset();
    testJumpDown = 1; testFrameTime = DT * 2.5f; Frame();
    assert(player.vy < 0 && !player.onGround && player.jumpBuf == 0 && !in.jumpPressed);

    // Pressing again after release must still produce another jump.
    testJumpDown = 0; testFrameTime = DT; Frame();
    PlayerInit(65, 149);
    testJumpDown = 1; Frame();
    assert(player.vy < 0 && !player.onGround);

    // This loop drives Hash2 through seeds whose products overflow int. The signed
    // form wraps to the same value the uint64_t reference computes, so the assert is
    // a sanity check only: what catches the regression is the UBSan trap check.sh
    // builds with (-fsanitize=undefined -fno-sanitize-recover=all).
    int seeds[] = {0, 1, 6, 18, 92, 162, 258, -1, INT_MIN, INT_MAX};
    for (unsigned i = 0; i < sizeof seeds / sizeof seeds[0]; i++) {
        for (unsigned j = 0; j < sizeof seeds / sizeof seeds[0]; j++) {
            u32 h = (u32)((uint64_t)(u32)seeds[i] * 374761393u
                       + (uint64_t)(u32)seeds[j] * 668265263u);
            h = (u32)((uint64_t)(h ^ (h >> 13)) * 1274126177u);
            h ^= h >> 16;
            assert(Hash2(seeds[i], seeds[j]) == h);
        }
    }
    puts("PASS: frame input and hash regressions");
    return 0;
}
