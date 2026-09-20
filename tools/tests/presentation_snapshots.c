// The renderer receives copies, never writable pointers into simulation ownership.
#define main game_main
#include "../../src/main.c"
#undef main
#include <assert.h>

static void CheckViews(void) {
    BirdView birds[4], birdsAgain[4];
    BeastView beast, beastAgain;
    PlantView plants[3], plantsAgain[3];
    PropView props[96], propsAgain[96];
    Particle particles[FX_MAX], particlesAgain[FX_MAX];
    memset(birds, 0, sizeof birds); memset(birdsAgain, 0, sizeof birdsAgain);
    memset(&beast, 0, sizeof beast); memset(&beastAgain, 0, sizeof beastAgain);
    memset(plants, 0, sizeof plants); memset(plantsAgain, 0, sizeof plantsAgain);
    memset(props, 0, sizeof props); memset(propsAgain, 0, sizeof propsAgain);
    memset(particles, 0, sizeof particles); memset(particlesAgain, 0, sizeof particlesAgain);
    Player before = player;
    Item itemsBefore[ITEM_MAX]; memcpy(itemsBefore, items, sizeof items);
    u8 tilesBefore[RH][RW]; memcpy(tilesBefore, tiles, sizeof tiles);
    int fireBefore = PropFireLit(roomIdx), ageBefore = PropsAge();
    int nb = LifeBirdViews(birds, 4), nm = LifeBeastView(&beast);
    int np = LifePlantViews(plants, 3), nr = PropsViews(props, 96);
    int nf = FxViews(particles, FX_MAX);
    assert(nb >= 0 && nb <= 4 && np >= 0 && np <= 3 && nr >= 0 && nr <= 96);
    assert(LifeBirdViews(NULL, 4) == 0 && LifeBirdViews(birdsAgain, 0) == 0);
    assert(LifeBeastView(NULL) == 0);
    assert(LifePlantViews(NULL, 3) == 0 && LifePlantViews(plantsAgain, -1) == 0);
    assert(PropsViews(NULL, 96) == 0 && PropsViews(propsAgain, 0) == 0);
    assert(FxViews(NULL, FX_MAX) == 0 && FxViews(particlesAgain, -1) == 0);
    // Read repeatedly, including every animation/door phase, without advancing time.
    for (int repeat = 0; repeat < 4; repeat++) {
        assert(LifeBirdViews(birdsAgain, 4) == nb);
        assert(LifeBeastView(&beastAgain) == nm);
        assert(LifePlantViews(plantsAgain, 3) == np);
        assert(PropsViews(propsAgain, 96) == nr);
        assert(FxViews(particlesAgain, FX_MAX) == nf);
        assert(memcmp(birds, birdsAgain, sizeof birds) == 0);
        assert(memcmp(&beast, &beastAgain, sizeof beast) == 0);
        assert(memcmp(plants, plantsAgain, sizeof plants) == 0);
        assert(memcmp(props, propsAgain, sizeof props) == 0);
        assert(memcmp(particles, particlesAgain, sizeof particles) == 0);
    }
    assert(memcmp(&before, &player, sizeof player) == 0);
    assert(memcmp(itemsBefore, items, sizeof items) == 0);
    assert(memcmp(tilesBefore, tiles, sizeof tiles) == 0);
    assert(fireBefore == PropFireLit(roomIdx) && ageBefore == PropsAge());
    // Overwriting a copied view cannot alter a future view or a live entity.
    memset(birdsAgain, 0xAB, sizeof birdsAgain);
    memset(propsAgain, 0xAB, sizeof propsAgain);
    LifeBirdViews(birdsAgain, nb); PropsViews(propsAgain, nr);
    assert(memcmp(birds, birdsAgain, (size_t)nb * sizeof *birds) == 0);
    assert(memcmp(props, propsAgain, (size_t)nr * sizeof *props) == 0);
    if (nf > 1) {
        Particle bounded[2]; memset(bounded, 0xCC, sizeof bounded);
        Particle canary = bounded[1];
        assert(FxViews(bounded, 1) == 1);
        assert(memcmp(&canary, &bounded[1], sizeof canary) == 0);
        bounded[0].x = -1000;
        assert(FxViews(bounded, 1) == 1);
        assert(bounded[0].x != -1000);
    }
    if (nb > 1) {
        BirdView bounded[2]; memset(bounded, 0xCC, sizeof bounded);
        BirdView canary = bounded[1];
        assert(LifeBirdViews(bounded, 1) == 1);
        assert(memcmp(&canary, &bounded[1], sizeof canary) == 0);
    }
    if (nr > 1) {
        PropView bounded[2]; memset(bounded, 0xCC, sizeof bounded);
        PropView canary = bounded[1];
        assert(PropsViews(bounded, 1) == 1);
        assert(memcmp(&canary, &bounded[1], sizeof canary) == 0);
    }
}

int main(void) {
    ItemsReset(); ItemsAdd(IT_LAMP, 0, 1, 14); RoomLoad();
    AudioInit(1); noDraw = 1; dbgFixedStep = 1;
    for (int room = 0; room < ROOM_COUNT; room++) {
        RoomEnter(room);
        PlayerInit(room ? 257 : 280, room ? 37 : 101);
        for (int frame = 0; frame < 1800; frame++) {
            if (frame % 40 == 0) FxBurst(FX_SPLASH, player.x, player.y, 5, 1, 1);
            CheckViews(); Frame();
        }
    }
    puts("PASS: 3600 frames of bounded, detached, read-only presentation snapshots");
    return 0;
}
