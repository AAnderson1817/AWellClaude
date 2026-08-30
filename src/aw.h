// aw.h — everything shared. C-style: flat structs, fixed-size arrays, no allocation
// in the game loop, no function pointers on entities, no inheritance.
#ifndef AW_H
#define AW_H

#include "raylib.h"
#include <stdint.h>
#include <stddef.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t  i32;
typedef float    f32;

// ---------------------------------------------------------------- resolution
#define GW 320          // internal render target width
#define GH 180          // internal render target height
#define TS 8            // tile size in pixels
#define RW 40           // room width  in tiles  (40*8 = 320)
#define RH 22           // room height in tiles  (22*8 = 176)
#define ROOM_Y 2        // room is centred in the 180px frame: 2px band top & bottom

#define WORLD_W 5       // L1: the ceiling. 5x5 = 25 rooms. This never rises.
#define WORLD_H 5
#define ROOM_COUNT (WORLD_W * WORLD_H)

#define DT (1.0f / 60.0f)

// ---------------------------------------------------------------- memory
// Three nested preallocated arenas. Global lives for the process, session for a
// save, room is wiped on every transition (bug containment comes free).
typedef struct { u8 *base; size_t size, used; } Arena;

void  ArenaInit(Arena *a, u8 *base, size_t size);
void *ArenaPush(Arena *a, size_t size, size_t align);
void  ArenaReset(Arena *a);
extern Arena globalArena, sessionArena, roomArena;
#define PushArray(a, T, n) (T *)ArenaPush((a), sizeof(T) * (n), _Alignof(T))
#define PushStruct(a, T)   (T *)ArenaPush((a), sizeof(T), _Alignof(T))

// ---------------------------------------------------------------- tiles
// The whole collision/lighting model lives in these flags, as in the original.
enum {
    TF_SOLID    = 1 << 0,   // blocks the player on all four sides
    TF_BLOCKS_L = 1 << 1,   // blocks light
    TF_OBSCURES = 1 << 2,   // drawn over the player
    TF_DARK     = 1 << 3,   // reads as black; anything may hide in it
    TF_ONEWAY   = 1 << 4,   // solid only from above
    TF_WATER    = 1 << 5,
    TF_CONTIG   = 1 << 6,   // autotiles against its own kind
    TF_RIM      = 1 << 7,   // a pan rim: the kettles can be filled or emptied here
};

enum {
    T_EMPTY = 0,
    T_ROCK,         // ordinary solid
    T_DARK,         // solid, reads as pure black — the hiding place
    T_LEDGE,        // one-way platform
    T_BRINE,        // deep brine: buoyancy is signed by load
    T_GRASS,        // decorative, non-solid
    T_CRUST,        // salt crust: holds a light body, shatters under a heavy one
    T_RIM,          // pan rim, solid; fill and empty the kettles here
    T_TIMBER,       // collapsing scaffold: one-way, and it is what the world is roofed with
    T_BELL,         // the diving bell. Layer 2 hook: visible in minute one, inert until the end.
    T_TILE_KINDS
};

extern const u8 tileFlags[T_TILE_KINDS];
static inline int TileSolid(u8 t)  { return (tileFlags[t] & TF_SOLID) != 0; }
static inline int TileOneWay(u8 t) { return (tileFlags[t] & TF_ONEWAY) != 0; }

// ---------------------------------------------------------------- room
typedef struct {
    u8 tiles[RH][RW];       // flat typed array, exactly as the original stores it
    u8 bgId;
    u8 lighting;
    u8 exists;
} Room;

// Room-arena scratch. Wiped on every transition, so crust recrystallises when you
// come back and no save state is needed to express that.
// A convex top corner of a solid tile: the only thing the gaff can bite.
// Derived from the tile grid, which is why both verbs deleting tiles is a coupling
// and not a coincidence.
typedef struct { i32 px, py; i32 tx, ty; u8 side; u8 worn; } Corner;
#define CORNER_MAX 256

typedef struct {
    u8  loadMap[RH][RW];    // cleared each frame; bodies stamp (1 + their load)
    u8  stress[RH][RW];     // crust fatigue
    f32 surfH[RW], surfV[RW];  // 1D brine surface wave, one column per tile
    u8  broken[RH][RW];     // tiles destroyed by either verb THIS visit
    Corner corners[CORNER_MAX];
    i32 cornerCount;
    i32 cornersDirty;
} RoomScratch;

// Room-scope state lives IN the room arena, not in a global that merely gets cleared.
// EnterRoom resets the arena and pushes a fresh one, so the wipe is the allocator's
// doing -- which is the whole point of the three-arena model.
extern RoomScratch *roomScratch;
#define scratch (*roomScratch)

#define CRUST_BREAK_LOAD 3  // loadMap value, i.e. load >= 2 (D3)
#define CRUST_STRESS_MAX 24

typedef struct {
    Room rooms[ROOM_COUNT];
    int cx, cy;             // current room coords
} World;

extern World world;
static inline Room *RoomAt(int x, int y) { return &world.rooms[y * WORLD_W + x]; }
static inline Room *CurRoom(void)        { return RoomAt(world.cx, world.cy); }
// Level data is immutable. Everything either verb destroys is recorded in the room
// arena instead, so it all comes back when you leave and return -- which is what the
// premise means by the crust recrystallising, and it costs no save state.
static inline u8 TileGet(int tx, int ty) {
    // Just outside the room is open when a room lies that way and solid when it does
    // not, so a body can step across a shared edge but the world's rim is rock. The
    // authored wall at the last column still blocks everywhere it is drawn solid.
    if (tx < 0)   return world.cx > 0            ? T_EMPTY : T_ROCK;
    if (tx >= RW) return world.cx < WORLD_W - 1  ? T_EMPTY : T_ROCK;
    if (ty < 0)   return world.cy > 0            ? T_EMPTY : T_ROCK;
    if (ty >= RH) return world.cy < WORLD_H - 1  ? T_EMPTY : T_ROCK;
    if (scratch.broken[ty][tx]) return T_EMPTY;
    return world.rooms[world.cy * WORLD_W + world.cx].tiles[ty][tx];
}
static inline void TileBreak(int tx, int ty) {
    if (tx < 0 || tx >= RW || ty < 0 || ty >= RH) return;
    scratch.broken[ty][tx] = 1;
    scratch.cornersDirty = 1;
}
u8 TileAtPx(float px, float py);   // world pixel -> tile id in the current room
void LoadWorld(void);
void EnterRoom(int nx, int ny);
void RoomArenaFresh(void);
int  RoomTransition(void);

// ---------------------------------------------------------------- player
typedef struct {
    f32 x, y;               // top-left of the hitbox, in room pixels
    f32 vx, vy;
    i32 w, h;
    int onGround;
    int facing;             // -1 / +1
    int coyote;             // frames of ground-memory remaining
    int jumpBuf;            // frames of buffered jump remaining
    int jumpHeld;
    int inWater;
    f32 animT;              // procedural animation clock
    f32 leanX, leanY;       // second-order lag, drives the procedural pass

    // --- Verb A: the kettles ---------------------------------------------
    u8  load;               // 0..4 kettles' worth of brine. The whole verb.
    i32 fillTimer;          // frames of the current fill/empty action
    int atRim;              // a TF_RIM tile overlaps the body this frame
    int submerged;          // body centre is inside brine
    i32 waterY;             // room-pixel y of the brine line cutting the body, or -1
    f32 sloshX, sloshY;     // second-order lag of the liquid inside the kettles
    f32 yokeAng;            // yoke swing, lags the body. Never squash-and-stretch.
    i32 landImpact;         // frames left of the last landing's dust

    // --- Verb B: the gaff -------------------------------------------------
    i32 hooked;             // index into scratch.corners, or -1
    f32 theta;              // 0 is straight below the corner
    f32 omega;              // angular velocity, rad/frame
    f32 r;                  // grip length, clamped
    i32 pivotFrames;        // for corner wear
    i32 clack;              // frames left of a failed catch
    i32 hookHeldPrev;
    i32 stuck;
    i32 hookTx, hookTy;     // the corner's identity, which survives a rebuild
    u8  hookSide;           // its index does not
} Player;

#define GAFF_REACH   24.0f
#define GAFF_RMIN    16.0f
#define GAFF_RMAX    28.0f
#define CORNER_WEAR_MAX 32
#define GAFF_ARC     1.48f   // radians either side of straight-down; never above the corner

#define LOAD_MAX 4

// Weight touches the gravity axis only (D4). Run speed, acceleration, friction and
// turnaround are identical at every load -- the moment weight changes horizontal
// handling it starts doing the gaff's job.
extern const f32 JUMP_V[LOAD_MAX + 1];
extern const f32 GRAV_L[LOAD_MAX + 1];
extern const f32 TERM_L[LOAD_MAX + 1];
extern const f32 SINK_L[LOAD_MAX + 1];   // signed: negative floats, positive sinks
extern const f32 JCUT_L[LOAD_MAX + 1];

extern Player player;

void PlayerInit(float x, float y);
void PlayerStep(void);
void PlayerDraw(void);
int  RectHitsSolidPublic(float x, float y, int w, int h);

// ---------------------------------------------------------------- input
// One indirection so scripted playtests and the keyboard share a path.
typedef struct { int left, right, up, down, jump, jumpPressed, a, b; } Input;
extern Input in;
void InputPoll(void);

// ---------------------------------------------------------------- render
// ---------------------------------------------------------------- fx
// Fixed pool. Purely presentational: nothing here is ever read by a rule.
enum { FX_DUST, FX_SALT, FX_DROP, FX_SPLINTER };
typedef struct { f32 x, y, vx, vy; u8 life, maxLife, kind; } Particle;
#define FX_MAX 192
void FxReset(void);
void FxSpawn(int kind, float x, float y, float vx, float vy, int life);
void FxBurst(int kind, float x, float y, int n, float spread, float up);
void FxStep(void);
void FxDraw(void);

// ---------------------------------------------------------------- room step
void BuildCorners(void);
void GaffStep(void);
void GaffDraw(void);

void RoomStepBegin(void);   // clears loadMap
void RoomStepEnd(void);     // crust fatigue, brine surface wave
void BrineDisturb(float px, float strength);

void RenderInit(void);
void RenderBeginRoom(void);
void RenderEndRoomAndPresent(void);
void DrawRoom(Room *r);
extern RenderTexture2D screenRT;

// ---------------------------------------------------------------- palette
extern Color palRock, palRockDeep, palRockLit, palDark, palBg, palBgDeep, palBrine, palGrass, palSkin;
extern Color palBrineL, palSalt, palSaltLit, palDust, palTimber, palIron, palSkinWet;

// ---------------------------------------------------------------- debug
extern int dbgShotFrames[16];
extern int dbgShotCount;
extern const char *dbgOutDir;
extern int dbgFixedStep;    // 1 = exactly one sim step per rendered frame
extern long frameNo;

#endif
