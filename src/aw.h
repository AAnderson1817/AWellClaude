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
};

enum {
    T_EMPTY = 0,
    T_ROCK,                 // ordinary stone
    T_LEDGE,                // one-way shelf
    T_VEIN,                 // stone with a lit mineral seam running through it
    T_MOSS,                 // hanging growth; not solid, not anything
    T_BULB,                 // authoring only: where a bulb sits. Becomes T_EMPTY + a Bulb
    T_KINDS
};

extern const u8 tileFlags[T_KINDS];
static inline int TileSolid(u8 t)  { return (tileFlags[t] & TF_SOLID)  != 0; }
static inline int TileOneWay(u8 t) { return (tileFlags[t] & TF_ONEWAY) != 0; }

// ---------------------------------------------------------------- the room
// One room. Level data is authored as text in room.c and read once at startup;
// nothing streams from disk, and nothing writes to it at runtime.
extern u8 tiles[RH][RW];

// Outside the room is stone. There is nowhere else to be.
static inline u8 TileGet(int tx, int ty) {
    if (tx < 0 || tx >= RW || ty < 0 || ty >= RH) return T_ROCK;
    return tiles[ty][tx];
}
u8 TileAtPx(float px, float py);

void RoomLoad(void);
void RoomDraw(void);
void LightStep(void);       // recompute the moving part of the light
void LightDraw(void);       // multiply the room by it
int  RoomStartTx(void);
int  RoomStartTy(void);

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
    f32 animT;
    f32 leanX, leanY;       // second-order lag. Lag only -- no squash, no stretch.
    i32 blink;
} Player;

extern Player player;
void PlayerInit(float x, float y);
void PlayerStep(void);
void PlayerDraw(void);
void PlayerDrawEyes(void);   // drawn after the light pass: you can always find yourself

// ---------------------------------------------------------------- input
// One indirection, so a scripted playtest and a keyboard take the same path.
typedef struct { int left, right, up, down, jump, jumpPressed; } Input;
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

// ---------------------------------------------------------------- render
void RenderInit(void);
void RenderBegin(void);
void RenderPresent(void);
extern RenderTexture2D screenRT;

// ---------------------------------------------------------------- palette
extern Color palVoid, palBack, palBackLit, palRock, palRockDeep, palRockLit;
extern Color palLedge, palLedgeLit, palVein, palVeinHot, palMoss;
extern Color palSkin, palSkinDeep, palEye, palPupil, palDrop, palBulb, palBulbLit, palBulbDeep;

// ---------------------------------------------------------------- debug
extern long frameNo;
extern int  dbgFixedStep;
extern const char *dbgOutDir;

// A tiny deterministic hash, used for tile texture and for the specks. Same seed,
// same room, every run -- so a screenshot is a fact and not a coincidence.
static inline u32 Hash2(int x, int y) {
    u32 h = (u32)x * 374761393u + (u32)y * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

#endif
