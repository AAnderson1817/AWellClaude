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

// ---------------------------------------------------------------- zones
// Every tile belongs to the vault (raw rock, timber, amber) or the city (dressed masonry,
// stone cornices, green light). Authored as a handful of rectangles per room in room.c.
// Stone, shelves, seams and the back wall draw by zone; a seam's light is warm or cool by it.
enum { Z_VAULT = 0, Z_CITY };
int ZoneAt(int tx, int ty);
void LightAddPointCool(f32 px, f32 py, f32 R, f32 peak);   // the city's colour of light

// ---------------------------------------------------------------- the air
// A small fluid over the room (air.c). Presentational: nothing reads it. Things push it and
// put dust, smoke or mist into it; the motes ride it; it is drawn into the lit layer.
extern int airOff;          // headless runs: off
void AirInit(void);         // per room, after the tiles
void AirStep(void);
void AirDraw(void);
void AirPush(f32 px, f32 py, f32 vx, f32 vy, f32 radius);         // px per frame
void AirPuff(f32 px, f32 py, f32 amount, f32 radius, f32 warm);   // warm: 1 smoke, 0 dust/mist
void AirAt(f32 px, f32 py, f32 *vx, f32 *vy);

// ---------------------------------------------------------------- props
// The dressing that answers you: a second text grid per room in props.c. Set pieces are
// text sprites, rows of palette letters. Nothing here is read by a rule.
typedef struct { int w, h; const char *const *rows; } Sprite;
void  DrawSprite(const Sprite *s, int px, int py);
void  DrawSpriteEx(const Sprite *s, int px, int py, int flip);
// rows y0..y1-1 only; ink >= 0 draws every pixel in that palette colour (a silhouette)
void  DrawSpriteRows(const Sprite *s, int px, int py, int flip, int y0, int y1, int ink);
// tag: 255 the far wall, 254 a drawn thing, 253 stone and shelves (see sprites.c)
void  DrawSpriteTag(const Sprite *s, int px, int py, int flip, u8 tag);
void  DrawSpriteRect(const Sprite *s, int px, int py, int sx, int sy, int w, int h, int flip, u8 tag);
enum { TAG_WALL = 255, TAG_THING = 254, TAG_STONE = 253 };
extern const Sprite SPR_RK_I1, SPR_RK_I2, SPR_RK_I3, SPR_RK_I4, SPR_RK_T1, SPR_RK_T2, SPR_RK_T3, SPR_RK_L1, SPR_RK_L2;
extern const Sprite SPR_RK_OT, SPR_RK_NT, SPR_RK_B1, SPR_RK_B2, SPR_RK_OB, SPR_RK_NB;
extern const Sprite SPR_ASH1, SPR_ASH2, SPR_VEIN1, SPR_VEIN2, SPR_GLASS;
extern const Sprite SPR_PLANK1, SPR_PLANK2, SPR_PLANK_END, SPR_CORNICE, SPR_CORNICE_END;
extern const Sprite SPR_MOSS_H1, SPR_MOSS_H2, SPR_MOSS_F1, SPR_LICHEN, SPR_WALL_VAULT, SPR_WALL_CITY;
extern const Sprite SPR_PLAYER_IDLE, SPR_PLAYER_WALK1, SPR_PLAYER_WALK2, SPR_PLAYER_JUMP, SPR_PLAYER_FALL;
extern const Sprite SPR_BIRD_PERCH, SPR_BIRD_LOOK, SPR_BIRD_UP, SPR_BIRD_DOWN;
extern const Sprite SPR_BEAST_STAND, SPR_BEAST_WALK1, SPR_BEAST_WALK2, SPR_BEAST_SIT, SPR_BEAST_HEAD;
extern const Sprite SPR_POD, SPR_POD_OPEN, SPR_BUSH, SPR_LAMP, SPR_STONE;
extern const Sprite SPR_POT, SPR_BEDROLL, SPR_PACK, SPR_CAIRN, SPR_BONES, SPR_FIRE_RING, SPR_BALUSTRADE, SPR_DOOR;
void PropsInit(void);        // per room, after the tiles are known
void PropsStep(void);
void PropsDrawBack(void);    // after the back wall, before the tiles
void PropsDrawFront(void);   // after the tiles, before the living things
void PropsLight(void);       // called by LightStep
void PropsReset(void);       // the one persistent change (the fire) back to how it began
int  PropFireLit(int room);
int  PropsAge(void);         // frames since this room was entered

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
int  LifeBeastPos(f32 *x, f32 *y);   // the animal's centre, if it lives in this room
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
    f32 hx, hy; int hroom;  // home: where it was set at the start, for starting over
} Item;
extern Item items[ITEM_MAX];
extern int  itemCount, heldItem;
void ItemsReset(void);
int  ItemsAdd(int kind, int room, int tx, int ty);
void ItemsHome(void);       // every item back where it began, hands empty
void ItemsStep(void);
void ItemsLight(void);      // called by LightStep
void ItemsDrawBehind(void); // before the body, before the light pass
void ItemsDrawHeld(void);   // after the body, before the light pass
void ItemsDrawCore(void);   // after the light pass
int  PlayerHolds(void);     // IT_NONE, IT_LAMP or IT_STONE
int  LampPos(f32 *x, f32 *y);   // the lamp's glass, if the lamp is in this room or in hand

// ---------------------------------------------------------------- input
// One indirection, so a scripted playtest and a keyboard take the same path.
typedef struct { int left, right, up, down, jump, jumpPressed, act, actPressed, reset; } Input;
extern Input in;
void InputPoll(void);

// ---------------------------------------------------------------- starting over
// The rule: you are never soft-locked without a way out. This is the way out, and it
// is always there. Hold R and the dark comes in -- your eyes are closing -- and if you
// hold it to the end you wake where you began, with everything where it began. Let go
// early and the room comes back. No text says so; the darkening says so.
extern f32 resetFade;       // 0 = eyes open, 1 = shut
void ResetStep(void);
void ResetDrawLids(void);   // after the light pass, before your eyes

// ---------------------------------------------------------------- fx
// A fixed pool of specks. Nothing here is ever read by a rule; it exists so the
// room looks like somewhere air moves and water finds its way down.
enum { FX_MOTE, FX_DRIP, FX_SPLASH, FX_DUST, FX_SPARK, FX_SHARD };
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
       SFX_PICKUP, SFX_SETDOWN, SFX_STONE, SFX_STONE_UP, SFX_WAKE,
       SFX_CREAK, SFX_CLINK, SFX_POT, SFX_SHATTER, SFX_GRIT, SFX_RATTLE, SFX_CATCH, SFX_CRACKLE, SFX_FLAP,
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
// Two looks. The new one (the default; V toggles, --oldlook starts in the old) draws the
// frame in layers and puts them together in one pass: light posterised into hard bands
// with dithering, rim light on edges that face a light, one palette. The old one is the
// blurred multiply it replaces, kept for comparing.
extern int lookNew;
extern int dbgAlbedo;      // --albedo: the art under flat light, for judging it
// Both rooms are drawn in it. (The flooded room was held back until its water had a ramp
// of its own: under the first cut the water went black and its lamps rang in rainbows.)
#define LOOK_NEW (lookNew)
enum { RL_EMIS = 1, RL_BACK };
void RenderInit(void);
void RenderBegin(void);
void RenderLayer(int which);
void RenderComposite(void);
void RenderPresent(void);
extern RenderTexture2D screenRT;
int  LightPoints(float *pos4, float *col4, int max);   // this frame's point lights, for the composite
Texture2D LightBakeTexture(void);
// the far city, room 0, seen through a break in the back wall (city.c)
int  CityBreachSpan(int y, int *x0, int *x1);
void CityErase(void);
void CityDraw(void);

// ---------------------------------------------------------------- the palette of the new look
enum { PL_VOID, PL_DEEP, PL_DARK, PL_STONE, PL_STONEL, PL_STONEH, PL_WARMD, PL_WARM, PL_AMBER,
       PL_AMBERH, PL_COOLD, PL_COOLM, PL_CITY, PL_CITYH, PL_BONE, PL_WATER, PL_WATERL, PL_ACCENT,
       PL_WATERD, PL_COUNT };
extern const Color PAL[PL_COUNT];

// ---------------------------------------------------------------- palette
extern Color palVoid, palBack, palBackLit, palRock, palRockDeep, palRockLit;
extern Color palLedge, palLedgeLit, palVein, palVeinHot, palMoss;
extern Color palSkin, palSkinDeep, palEye, palPupil, palDrop, palBulb, palBulbLit, palBulbDeep;
extern Color palWater, palWaterLit, palWaterFleck, palSkinWet;
extern Color palBush, palBushLit, palBerry, palStalk, palLeaf, palPod, palPodLit, palPodDeep;
extern Color palBird, palBirdLight, palFur, palFurLight, palEyeGreen;
extern Color palLampIron, palLampGlass, palLampHot, palStone, palStoneLit, palStoneDeep;
extern Color palAshlar, palAshlarLit, palMortar, palCornice, palCorniceLit, palLichen;
extern Color palDoor, palDoorGroove, palCityGlass, palCityGlassLit, palIron, palRope;
extern Color palCloth, palClothLit, palClay, palClayLit, palBone, palEmber, palFlame, palFlameHot;

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
