// aw.h -- the entire shared surface of the navigation slice.
//
// This is a restart. The slice does one thing: a body that runs and jumps around
// one room. There is no world grid, no allocator, no second verb, nothing that
// counts or scores, and no text anywhere on screen. Everything else waits until
// the movement has been played and signed off.
//
// C, not C++. Flat structs, fixed-size arrays, no allocation after startup, no
// virtual dispatch, no entity base class.
#ifndef AW_H
#define AW_H

#include "raylib.h"
#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef int16_t  i16;
typedef uint32_t u32;
typedef int32_t  i32;
typedef float    f32;

// ---------------------------------------------------------------- resolution
#define GW 320              // internal render target
#define GH 180
#define TS 8                // tile size
#define RW 40               // 40 * 8 = 320
#define RH 22               // 22 * 8 = 176
#define ROOM_Y 2            // the room sits in the 180px frame with a 2px band
#define DT (1.0f / 60.0f)

// ---------------------------------------------------------------- tiles
// The whole collision and lighting model is these four bits. A tile is one byte.
enum {
    TF_SOLID  = 1 << 0,     // blocks on all four sides
    TF_ONEWAY = 1 << 1,     // blocks downward motion only: stand on it, jump through it
    TF_OPAQUE = 1 << 2,     // receives light, does not pass it on
    TF_EMIT   = 1 << 3,     // a light source
    TF_WATER  = 1 << 4,     // buoyant; light dies faster in it
};

enum {
    T_EMPTY = 0,
    T_ROCK,                 // ordinary stone
    T_LEDGE,                // one-way shelf
    T_VEIN,                 // stone with a lit mineral seam running through it
    T_MOSS,                 // hanging growth; not solid, not anything
    T_BULB,                 // authoring only: where a bulb sits. Becomes T_EMPTY + a Bulb
    T_WATER,                // the flooded part. You float in it, a quarter under
    T_BUSH,                 // a clump you can walk through; it minds
    T_KINDS
};

extern const u8 tileFlags[T_KINDS];
static inline int TileSolid(u8 t)  { return (tileFlags[t] & TF_SOLID)  != 0; }
static inline int TileOneWay(u8 t) { return (tileFlags[t] & TF_ONEWAY) != 0; }
static inline int TileWater(u8 t)  { return (tileFlags[t] & TF_WATER)  != 0; }

// ---------------------------------------------------------------- the rooms
// Two rooms, stacked: the chamber and the flooded one under it. Level data is authored
// as text in room.c and read once at startup; nothing streams from disk, and nothing
// writes to it at runtime. `tiles` holds the room you are in.
#define ROOM_COUNT 2
extern u8  tiles[RH][RW];                  // the room you are in
extern u8  roomTiles[ROOM_COUNT][RH][RW];  // every room, parsed once
extern int roomIdx;

// Past the side walls is stone. Past the top or bottom is THE NEXT ROOM'S TILES, not
// open air: a body straddling the seam collides with what is really there. Without
// this, a shelf just inside the next room did not exist until the room switched, and
// by then you were below it -- which is what "I fell straight through A1" was.
static inline u8 TileGet(int tx, int ty) {
    if (tx < 0 || tx >= RW) return T_ROCK;
    if (ty < 0)   return (roomIdx > 0 && ty >= -RH) ? roomTiles[roomIdx - 1][ty + RH][tx] : T_ROCK;
    if (ty >= RH) return (roomIdx < ROOM_COUNT - 1 && ty < 2 * RH) ? roomTiles[roomIdx + 1][ty - RH][tx] : T_ROCK;
    return tiles[ty][tx];
}
u8 TileAtPx(float px, float py);

void RoomLoad(void);
void RoomEnter(int idx);
int  RoomTransition(void);  // stepped off the top or bottom: change room, keep motion
void RoomDraw(void);
void WaterStep(void);       // the surface, a 1D wave
void WaterDisturb(float px, float strength);
void LightStep(void);       // recompute the moving part of the light
void LightDraw(void);       // multiply the room by it
int  RoomStartTx(void);
int  RoomStartTy(void);
// The standable runs, and the authored places for the living things, for life.c.
int  SurfCount(void);
void SurfGet(int i, int *x0, int *x1, int *y, int *shelf);
int  RoomMarkBeast(int *tx, int *ty);
int  RoomMarkPlants(int *txs, int *tys, int max);
int  RoomMarkStones(int *txs, int *tys, int max);
void LightAddPoint(f32 px, f32 py, f32 R, f32 peak);   // an occluded point light, this frame

// ---------------------------------------------------------------- bulbs
// A dome you land on and leave faster than you arrived. Not solid: you walk through
// it, you cannot stand on it, it only answers a fall. Every landing throws you the
// same height -- higher than you can jump. Press jump as you meet it and it throws
// you a little higher still: the press counts from a few frames before contact to a
// few frames after, the way a spring in any platformer worth the name does.
#define BULB_MAX 8
#define BULB_W   12
#define BULB_H   6
#define BULB_LATE 4                  // frames after contact a press still counts
typedef struct { i32 x, y; i32 squash; i32 flash; i32 timed; } Bulb;   // x,y: base centre, room px
extern Bulb bulbs[BULB_MAX];
extern int  bulbCount;
void BulbsStep(void);
void BulbsDraw(void);
int  BulbCrossed(float oldBottom, float newBottom, float x, int w);   // index or -1

// ---------------------------------------------------------------- the body
typedef struct {
    f32 x, y;               // top-left of the hitbox, in room pixels
    f32 vx, vy;
    i32 w, h;
    int onGround;
    int facing;             // -1 / +1
    int coyote;             // frames of ground-memory left
    int jumpBuf;            // frames of buffered jump left
    int jumpHeld;
    int airFrames;
    int landImpact;
    int launched;           // rising off a bulb: the jump cut does not apply
    int bulbGrace;          // frames left in which a late press still lifts the bounce
    int lastBulb;
    int submerged;          // any of the body under the surface
    int splashCool;         // frames before another splash may sound
    int heavy;              // holding a stone: the gravity axis changes, nothing else
    i32 waterY;             // room-pixel y of the surface where it cuts the body, or -1
    f32 animT;
    f32 leanX, leanY;       // second-order lag. Lag only -- no squash, no stretch.
    i32 blink;
} Player;

extern Player player;
void PlayerInit(float x, float y);
void PlayerStep(void);
void PlayerDraw(void);
void PlayerDrawEyes(void);   // drawn after the light pass: you can always find yourself

// ---------------------------------------------------------------- life
// Bushes, birds, the animal, the plant. Nothing here is read by a rule.
#define BUFFER_FRAMES 7
void LifeInit(void);        // per room, after the tiles and surfaces are known
void LifeStep(void);
void LifeDraw(void);        // before the light pass
void LifeDrawEyes(void);    // after it
void LifeLights(void);      // called by LightStep
void LifePrintStats(void);
extern u8 bushShake[RH][RW];

// ---------------------------------------------------------------- things you can hold
// One hand. The lamp gives light and floats; a stone sinks, and so do you while you
// hold it. Persistent: each has a room of its own when set down.
enum { IT_NONE = 0, IT_LAMP, IT_STONE };
#define ITEM_MAX 8
typedef struct {
    int kind, room;         // room: where it is when not held
    f32 x, y, vy;
    int onGround, cool;
    f32 flick;              // the lamp's
} Item;
extern Item items[ITEM_MAX];
extern int  itemCount, heldItem;
void ItemsReset(void);
int  ItemsAdd(int kind, int room, int tx, int ty);
void ItemsStep(void);
void ItemsLight(void);      // called by LightStep
void ItemsDrawBehind(void); // before the body, before the light pass
void ItemsDrawHeld(void);   // after the body, before the light pass
void ItemsDrawCore(void);   // after the light pass
int  PlayerHolds(void);     // IT_NONE, IT_LAMP or IT_STONE

// ---------------------------------------------------------------- input
// One indirection, so a scripted playtest and a keyboard take the same path.
typedef struct { int left, right, up, down, jump, jumpPressed, act, actPressed; } Input;
extern Input in;
void InputPoll(void);

// ---------------------------------------------------------------- fx
// A fixed pool of specks. Nothing here is ever read by a rule; it exists so the
// room looks like somewhere air moves and water finds its way down.
enum { FX_MOTE, FX_DRIP, FX_SPLASH, FX_DUST };
typedef struct { f32 x, y, vx, vy; u16 life, maxLife; u8 kind; u8 seed; } Particle;
#define FX_MAX 160
void FxInit(void);
void FxStep(void);
void FxDraw(void);
void FxBurst(int kind, float x, float y, int n, float spread, float up);

// ---------------------------------------------------------------- audio
// Synthesized at startup, nothing loaded. Sfx() is the one call: an id, a volume,
// a pitch, a pan (0.5 is centre). Nothing here is ever read by a rule.
enum { SFX_STEP_STONE, SFX_STEP_SHELF, SFX_LAND, SFX_JUMP, SFX_SPLASH_IN, SFX_SPLASH_OUT,
       SFX_SWIM, SFX_DRIP, SFX_BULB, SFX_BULB_TIMED,
       SFX_RUSTLE, SFX_WING, SFX_CHIRP, SFX_PAD, SFX_CHIRR, SFX_PLANT0, SFX_PLANT1, SFX_PLANT2,
       SFX_PICKUP, SFX_SETDOWN, SFX_STONE, SFX_STONE_UP,
       SFX_COUNT };
void  AudioInit(int mute);
void  AudioStep(void);
void  Sfx(int id, float vol, float pitch, float pan);
void  AudioAmbience(int room);
float AudioRnd(void);          // -1..1, for pitch and level variation
int   AudioExportMontage(const char *path);
extern const char *dbgLastSfx;
extern int sfxCount[SFX_COUNT];

// ---------------------------------------------------------------- render
void RenderInit(void);
void RenderBegin(void);
void RenderPresent(void);
extern RenderTexture2D screenRT;

// ---------------------------------------------------------------- palette
extern Color palVoid, palBack, palBackLit, palRock, palRockDeep, palRockLit;
extern Color palLedge, palLedgeLit, palVein, palVeinHot, palMoss;
extern Color palSkin, palSkinDeep, palEye, palPupil, palDrop, palBulb, palBulbLit, palBulbDeep;
extern Color palWater, palWaterLit, palWaterFleck, palSkinWet;
extern Color palBush, palBushLit, palBerry, palStalk, palLeaf, palPod, palPodLit, palPodDeep;
extern Color palBird, palBirdLight, palFur, palFurLight, palEyeGreen;
extern Color palLampIron, palLampGlass, palLampHot, palStone, palStoneLit, palStoneDeep;

// ---------------------------------------------------------------- debug
extern long frameNo;
extern int  dbgFixedStep;
extern int  dbgLabels;      // L toggles: every standable run gets a two-character tag
extern const char *dbgOutDir;
void DebugLabelsDraw(void);  // after the light pass; this is scaffolding, not the game
void DebugLabelsPrint(void);

// A tiny deterministic hash, used for tile texture and for the specks. Same seed,
// same room, every run -- so a screenshot is a fact and not a coincidence.
static inline u32 Hash2(int x, int y) {
    u32 h = (u32)x * 374761393u + (u32)y * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

#endif
