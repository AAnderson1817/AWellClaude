#include "aw.h"
#include <string.h>
#include <math.h>

World world;
RoomScratch *roomScratch;

const u8 tileFlags[T_TILE_KINDS] = {
    [T_EMPTY]  = 0,
    [T_ROCK]   = TF_SOLID | TF_BLOCKS_L | TF_CONTIG,
    [T_DARK]   = TF_SOLID | TF_BLOCKS_L | TF_CONTIG | TF_DARK,
    [T_LEDGE]  = TF_ONEWAY,
    [T_BRINE]  = TF_WATER,
    [T_GRASS]  = 0,
    [T_CRUST]  = TF_SOLID | TF_CONTIG,
    [T_TIMBER] = TF_ONEWAY,
};

u8 TileAtPx(float px, float py) {
    return TileGet((int)floorf(px / TS), (int)floorf(py / TS));
}

#include "rooms_gen.h"

void LoadWorld(void) {
    for (int i = 0; i < ROOM_COUNT; i++) {
        Room *r = &world.rooms[i];
        memcpy(r->tiles, roomData[i], sizeof r->tiles);
        r->exists = 1;
    }
    world.cx = START_ROOM_X;
    world.cy = START_ROOM_Y;
}

// A transition wipes the room arena. Crust knits back, sheared nubs return, particles
// and the brine surface reset, and no bug in one room can survive into the next.
void RoomArenaFresh(void) {
    ArenaReset(&roomArena);
    roomScratch = PushStruct(&roomArena, RoomScratch);
    memset(roomScratch, 0, sizeof *roomScratch);
}

void EnterRoom(int nx, int ny) {
    if (nx < 0 || nx >= WORLD_W || ny < 0 || ny >= WORLD_H) return;
    world.cx = nx; world.cy = ny;
    RoomArenaFresh();
    FxReset();
}

int RoomTransition(void) {
    const float wpx = RW * TS, hpx = RH * TS;
    if (player.x + player.w > wpx && world.cx < WORLD_W - 1) {
        EnterRoom(world.cx + 1, world.cy); player.x = 0.5f; return 1;
    }
    if (player.x < 0.0f && world.cx > 0) {
        EnterRoom(world.cx - 1, world.cy); player.x = wpx - player.w - 0.5f; return 1;
    }
    if (player.y + player.h > hpx && world.cy < WORLD_H - 1) {
        EnterRoom(world.cx, world.cy + 1); player.y = 0.5f; return 1;
    }
    if (player.y < 0.0f && world.cy > 0) {
        EnterRoom(world.cx, world.cy - 1); player.y = hpx - player.h - 0.5f; return 1;
    }
    // The world's outer edge is sealed; walking into it is walking into rock.
    if (player.x < 0.0f) { player.x = 0.0f; player.vx = 0.0f; }
    if (player.x + player.w > wpx) { player.x = wpx - player.w; player.vx = 0.0f; }
    if (player.y < 0.0f) { player.y = 0.0f; player.vy = 0.0f; }
    if (player.y + player.h > hpx) { player.y = hpx - player.h; player.vy = 0.0f; }
    return 0;
}
