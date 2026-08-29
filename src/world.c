#include "aw.h"

World world;
RoomScratch scratch;

const u8 tileFlags[T_TILE_KINDS] = {
    [T_EMPTY]  = 0,
    [T_ROCK]   = TF_SOLID | TF_BLOCKS_L | TF_CONTIG,
    [T_DARK]   = TF_SOLID | TF_BLOCKS_L | TF_CONTIG | TF_DARK,
    [T_LEDGE]  = TF_ONEWAY,
    [T_BRINE]  = TF_WATER,
    [T_GRASS]  = 0,
    [T_CRUST]  = TF_SOLID | TF_CONTIG,
    [T_RIM]    = TF_SOLID | TF_BLOCKS_L | TF_RIM,
    [T_TIMBER] = TF_ONEWAY,
};

u8 TileAtPx(float px, float py) {
    int tx = (int)(px / TS), ty = (int)(py / TS);
    if (tx < 0 || tx >= RW || ty < 0 || ty >= RH) return T_ROCK;  // outside is solid
    return CurRoom()->tiles[ty][tx];
}
