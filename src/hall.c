// hall.c -- the antechamber's own small lives and signs, from claude/LORE.md: the hunter at
// the camp, the sitters on the sill, the prints that come up on the door in the dark, the
// leaves from the flue, the mural in the threshold, the dead lamps, the spilled glass, the
// fish in the basin, the dust the draft carries, and the tall ones putting their light out.
//
// None of it is read by a rule, none of it counts anything, and none of it says anything
// in words (L5). Each is a thing that is there, doing what it does; most of them do it
// whether or not you are looking. Flat arrays, fixed sizes, no dispatch.
#include "aw.h"
#include <math.h>
#include <string.h>

static u32 rng = 0x2545F491u;
static f32 Rnd(void) { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return (f32)(rng & 0xFFFFFF) / 16777215.0f; }

// Where your lamp is, if it is anywhere: in hand or set down.
static int Lamp(f32 *x, f32 *y) { return LampPos(x, y); }
static f32 LampDist(f32 x, f32 y) {
    f32 lx, ly;
    if (!Lamp(&lx, &ly)) return 1e9f;
    return sqrtf((lx - x) * (lx - x) + (ly - y) * (ly - y));
}
static void Px(int x, int y, int pl, u8 tag) { Color c = PAL[pl]; c.a = tag; DrawRectangle(x, ROOM_Y + y, 1, 1, c); }
static int InView(f32 x, f32 y, f32 m) { return x > camX - m && x < camX + GW + m && y > camY - m && y < camY + GH + m; }

// ---------------------------------------------------------------- the sprites
// Your kind, sitting: the same bone-white body, a hat pulled down. The hunter faces his
// fire; the sitters face the window, so we see their backs, wrapped in bedrolls.
static const char *const HUNTER_ROWS[] = {
    "..www...",
    ".wWWWw..",
    "wwwwwww.",
    ".bbb1b..",
    ".bbbbb5.",
    ".bbbbb5.",
    "bbbbbbb5",
    ".bbbbbb5",
    ".55..55.",
};
static const Sprite HUNTER = { 8, 9, HUNTER_ROWS };
static const char *const HUNTER_WARM_ROWS[] = {     // both hands held out to the fire
    "..www.....",
    ".wWWWw....",
    "wwwwwww...",
    ".bbb1b....",
    ".bbbbbbbb.",
    ".bbbbb5.b5",
    "bbbbbbb5..",
    ".bbbbbb5..",
    ".55..55...",
};
static const Sprite HUNTER_WARM = { 10, 9, HUNTER_WARM_ROWS };
static const char *const SITTER_ROWS[] = {
    "..www..",
    ".wWWWw.",
    "wwwwwww",
    ".WWWWW.",
    "WWaWWWW",
    "WWWWWaW",
    "WWWWWWW",
    ".wwwww.",
};
static const Sprite SITTER = { 7, 8, SITTER_ROWS };
static const char *const SITTER_BONES_ROWS[] = {
    "..www..",
    ".wWWWw.",
    "wwwwwww",
    "..bbb..",
    "...b...",
    ".b5b5b.",
    ".5b5b5.",
    "..bbb..",
};
static const Sprite SITTER_BONES = { 7, 8, SITTER_BONES_ROWS };
static const char *const DEAD_LAMP_ROWS[] = {       // a lamp like yours, gone out: dull glass, dust
    ".22.",
    "2112",
    "2332",
    "2222",
    "2222",
    ".11.",
};
static const Sprite DEAD_LAMP = { 4, 6, DEAD_LAMP_ROWS };

// ---------------------------------------------------------------- where things are
// In room px. They belong to the map; they are here because only this file draws them.
#define HUNTER_X  (29 * TS)          // the camp: between the cairn and the fire, on row 35
#define CAMP_Y    (35 * TS)
#define FIRE_X    (33 * TS + 4)
#define SILL_Y    (14 * TS)          // the sill's top
static const int SITX[3] = { 93 * TS, 96 * TS + 2, 99 * TS + 5 };
#define NICHE_X0  (4 * TS)           // the dead lamps, in a recess at the back of the undercroft
#define NICHE_N   5
#define PATCH_X   (NICHE_X0 + NICHE_N * 7)        // the clean patch at the end of the row
#define FLUE_X    (6 * TS + 8)                    // the flue slot's mouth
#define FLUE_Y    (4 * TS)
#define STEP_Y    (14 * TS)                       // the step at the door
#define MURAL_X0  (26 * TS)          // the threshold: the mural runs along its wall
#define MURAL_X1  (39 * TS)
#define MURAL_Y   (10 * TS + 2)
#define BASIN_X0  (51 * TS)
#define BASIN_X1  (96 * TS)
#define BASIN_Y0  (36 * TS)
#define BASIN_Y1  (43 * TS)

// ---------------------------------------------------------------- state
static int   hunterArm, hunterHum, hunterLook, hunterNoted;
static int   sitBreath[3], sitLook;
static f32   printsA;                           // 0..1: how far the prints have come up
static int   leafN, leafT;
static struct { f32 x, y, vx, ph; int down; } leaves[10];
static f32   douse;                             // 0 lit .. 1 put out, beyond the fireguard
static int   douseHold, nearer, douseWas;
static struct { f32 x, y, vx, ph; } fish[6];
int hallHums, hallLeaves, hallDouses;

void HallInit(void) {
    rng = 0x2545F491u;
    hunterArm = 0; hunterHum = 240; hunterLook = 0; hunterNoted = 0;
    for (int i = 0; i < 3; i++) sitBreath[i] = 900 + i * 700;
    sitLook = 0; printsA = 0; leafN = 0; leafT = 1800;
    douse = 0; douseHold = 0; nearer = 0; douseWas = 0;
    for (int i = 0; i < 6; i++) {
        fish[i].x = BASIN_X0 + 20 + Rnd() * (BASIN_X1 - BASIN_X0 - 40);
        fish[i].y = BASIN_Y0 + 14 + Rnd() * 30;
        fish[i].vx = (Rnd() < 0.5f ? -1 : 1) * (0.12f + Rnd() * 0.12f);
        fish[i].ph = Rnd() * 6.28f;
    }
}

// ---------------------------------------------------------------- step
void HallStep(void) {
    f32 lx = 0, ly = 0;
    int lamp = Lamp(&lx, &ly);
    int fire = PropFireLit(0);

    // The hunter. Cold: he tends his cairn -- lifts the top stone, turns it, sets it back. Lit:
    // he holds both hands to the fire and hums, now and then. Your lamp near, his head turns
    // a pixel toward it. Your lamp set down on the clean patch among the dead ones: he looks
    // up, and hums once.
    if (hunterArm > 0) hunterArm--;
    else if (!fire && Rnd() < 1.0f / 600) hunterArm = 50;
    if (hunterArm == 25) SfxAt(SFX_GRIT, 0.25f, 1.2f, HUNTER_X - 12, CAMP_Y);
    hunterLook = lamp && fabsf(lx - (HUNTER_X + 4)) < 5 * TS && fabsf(ly - CAMP_Y) < 4 * TS;
    int onPatch = lamp && heldItem != 0 && fabsf(lx - (PATCH_X + 2)) < 5 && fabsf(ly - (CAMP_Y - 4)) < 6;
    if (onPatch && !hunterNoted) { hunterNoted = 1; hunterHum = 20; }
    if (!onPatch) hunterNoted = 0;
    if (fire || hunterNoted == 1) {
        if (--hunterHum <= 0) {
            SfxAt(SFX_HUM, 0.7f, 0.94f + Rnd() * 0.1f, HUNTER_X + 4, CAMP_Y - 6);
            hallHums++;
            hunterHum = 420 + (int)(Rnd() * 420);
            if (hunterNoted == 1) hunterNoted = 2;
        }
    }

    // The sitters: two breathe, a puff of mist a minute apart; the nearest turns its head a
    // pixel toward your lamp and back. The third does neither.
    for (int i = 0; i < 2; i++)
        if (--sitBreath[i] <= 0) {
            AirPuff(SITX[i] + 3.0f, SILL_Y - 7.0f, 0.35f, 3.0f, 0.0f);
            sitBreath[i] = 3000 + (int)(Rnd() * 1200);
        }
    sitLook = lamp && fabsf(lx - SITX[0]) < 6 * TS && fabsf(ly - SILL_Y) < 3 * TS;

    // The prints: with no flame within about eight tiles of the door, they come up, slowly;
    // any flame and they are gone, quickly.
    int dark = LampDist(5 * TS, 11 * TS) > 8 * TS;
    printsA += dark ? 0.004f : -0.05f;
    if (printsA < 0) printsA = 0;
    if (printsA > 1) printsA = 1;

    // A leaf, now and then, out of the flue: it spins down and lies on the step with the rest.
    if (--leafT <= 0) {
        leafT = 3600 + (int)(Rnd() * 3600);
        if (leafN < 10) {
            leaves[leafN].x = FLUE_X + Rnd() * 6; leaves[leafN].y = (f32)FLUE_Y;
            leaves[leafN].vx = 0; leaves[leafN].ph = Rnd() * 6.28f; leaves[leafN].down = 0;
            leafN++;
        }
    }
    for (int i = 0; i < leafN; i++) {
        if (leaves[i].down) continue;
        leaves[i].ph += 0.07f;
        f32 ax, ay; AirAt(leaves[i].x, leaves[i].y, &ax, &ay);
        leaves[i].x += sinf(leaves[i].ph) * 0.35f + ax * 0.5f;
        leaves[i].y += 0.22f + ay * 0.3f;
        if (leaves[i].y >= STEP_Y - 1) {
            leaves[i].y = STEP_Y - 1; leaves[i].down = 1; hallLeaves++;
            SfxAt(SFX_LEAF, 0.35f, 0.9f + Rnd() * 0.2f, leaves[i].x, leaves[i].y);
        }
    }

    // Beyond the fireguard: bring a lamp near and their light goes out, with a murmur close to
    // the bars; it comes back five to ten seconds after you leave, and sometimes one of them
    // is a step nearer than it was.
    int near = lamp && lx > 78 * TS && lx < 99 * TS && ly > 22 * TS && ly < 43 * TS;
    if (near) douseHold = 300 + (int)(Rnd() * 300);
    else if (douseHold > 0) douseHold--;
    int want = near || douseHold > 0;
    douse += want ? 0.03f : -0.01f;
    if (douse < 0) douse = 0;
    if (douse > 1) douse = 1;
    if (douse > 0.9f && !douseWas) {
        douseWas = 1; hallDouses++;
        SfxAt(SFX_MURMUR, 0.8f, 0.9f + Rnd() * 0.1f, 88 * TS, 36 * TS);
    }
    if (douse < 0.05f && douseWas) { douseWas = 0; if (nearer < 2 && Rnd() < 0.6f) nearer++; }

    // The fish: small lights of theirs, in the basin and through the bars. They turn toward a
    // lamp in the water.
    for (int i = 0; i < 6; i++) {
        fish[i].ph += 0.03f;
        f32 tx = fish[i].vx;
        if (lamp && ly > BASIN_Y0 && fabsf(lx - fish[i].x) < 6 * TS) tx = (lx > fish[i].x ? 0.25f : -0.25f);
        fish[i].x += tx;
        fish[i].y += sinf(fish[i].ph) * 0.12f;
        if (fish[i].x < BASIN_X0 + 4) { fish[i].x = BASIN_X0 + 4; fish[i].vx = fabsf(fish[i].vx); }
        if (fish[i].x > BASIN_X1 + 60) { fish[i].x = BASIN_X1 + 60; fish[i].vx = -fabsf(fish[i].vx); }
        if (fish[i].y < BASIN_Y0 + 8) fish[i].y = BASIN_Y0 + 8;
        if (fish[i].y > BASIN_Y1 - 4) fish[i].y = BASIN_Y1 - 4;
    }

    // The draft: the air comes in low under the fireguard and carries a little dust with it.
    if ((frameNo % 24) == 0) AirPuff(94 * TS + Rnd() * 8, 34 * TS + Rnd() * 8, 0.10f, 4.0f, 0.0f);
    if ((frameNo % 40) == 0) AirPuff(60 * TS + Rnd() * 20 * TS, 35 * TS + 2, 0.12f, 6.0f, 0.0f);   // mist on the water
}

void HallReset(void) { leafN = 0; nearer = 0; }

// ---------------------------------------------------------------- drawing
// The mural, carved: small figures with ears and hats, in shallow relief. Only your light
// shows it -- it is drawn into the lit layer, a shade off the wall. In four scenes, left to
// right: they come through a petalled door carrying lights; they are met; they sit in a row;
// the same row, worn almost smooth.
static void Small(int x, int y, int worn, int lamp) {
    int pl = worn ? PL_DARK : PL_STONE;
    Px(x + 1, y, pl, TAG_WALL); Px(x + 3, y, pl, TAG_WALL);                 // ears
    for (int j = 1; j < 7; j++) for (int i = 0; i < 5; i++) {
        if (worn && ((i + j) & 1)) continue;
        if ((j == 1 && (i == 0 || i == 4)) || (j == 6 && (i == 2))) continue;
        Px(x + i, y + j, pl, TAG_WALL);
    }
    if (!worn) { Px(x, y + 2, PL_STONEL, TAG_WALL); Px(x + 1, y + 2, PL_STONEL, TAG_WALL); Px(x + 2, y + 2, PL_STONEL, TAG_WALL); Px(x + 3, y + 2, PL_STONEL, TAG_WALL); Px(x + 4, y + 2, PL_STONEL, TAG_WALL); }   // the hat's brim
    if (lamp) { Px(x + 5, y + 3, PL_STONEL, TAG_WALL); Px(x + 5, y + 4, PL_STONEL, TAG_WALL); }  // a lamp carried
}
static void MuralRelief(void) {
    if (!InView(MURAL_X0, MURAL_Y, 120)) return;
    int y = MURAL_Y;
    // the band's frame: a fillet above and below
    for (int x = MURAL_X0; x < MURAL_X1; x++) { Px(x, y - 3, PL_STONEL, TAG_WALL); Px(x, y + 20, PL_STONEL, TAG_WALL); Px(x, y + 21, PL_DEEP, TAG_WALL); }
    int x = MURAL_X0 + 4;
    // 1: a petalled door, and three small figures coming through it with lights
    for (int j = 0; j < 14; j++) for (int i = 0; i < 10; i++) {
        f32 dx = i - 4.5f, dy = j - 7.0f;
        if (dx * dx / 25 + dy * dy / 49 < 1 && ((i + j) % 3)) Px(x + i, y + 4 + j, PL_DARK, TAG_WALL);
    }
    Small(x + 12, y + 11, 0, 1); Small(x + 19, y + 11, 0, 1); Small(x + 26, y + 11, 0, 1);
    // 2: met -- the gaps between the tall ones, which the phosphor draws
    Small(x + 44, y + 11, 0, 1); Small(x + 55, y + 11, 0, 0);
    // 3: sitting in a row
    for (int k = 0; k < 3; k++) Small(x + 70 + k * 7, y + 12, 0, 0);
    // 4: the same row, worn smooth
    for (int k = 0; k < 3; k++) Small(x + 95 + k * 7, y + 12, 1, 0);
}

// The tall ones in procession, in phosphor: only in the dark, and fading as a light comes.
static void MuralPhosphor(void) {
    if (!InView(MURAL_X0, MURAL_Y, 120)) return;
    f32 a = 1.0f - LampDist((MURAL_X0 + MURAL_X1) * 0.5f, MURAL_Y + 10) / (7 * TS);
    a = a > 0 ? 1.0f - a * 1.6f : 1.0f;               // near a lamp it fades
    if (a <= 0.05f) return;
    int y = MURAL_Y, x0 = MURAL_X0 + 4;
    static const int TX[8] = { 34, 50, 62, 66, 88, 92, 116, 120 };   // where each tall one stands
    for (int k = 0; k < 8; k++) {
        int x = x0 + TX[k];
        for (int j = 0; j < 19; j++) {
            int w = j < 5 ? 2 : (j < 7 ? 1 : 3);          // a long head, a neck, a robe
            int ox = j < 5 ? (j < 2 ? 1 : 0) : (j < 7 ? 1 : 0);
            for (int i = 0; i < w; i++) {
                if (Hash2(x + i + ox, y + j + (int)(frameNo / 90)) % 100 > (u32)(a * 100)) continue;
                DrawRectangle(x + i + ox, ROOM_Y + y + j, 1, 1, PAL[(i == 0 && j > 6) ? PL_COOLM : PL_COOLD]);
            }
        }
        // the hands out, toward the small ones
        if (Hash2(k, 3) & 1) DrawRectangle(x - 2, ROOM_Y + y + 9, 2, 1, PAL[PL_COOLM]);
        else DrawRectangle(x + 3, ROOM_Y + y + 9, 2, 1, PAL[PL_COOLM]);
    }
}

// The prints on the door and the rock round it: palms, paws, three- and six-fingered hands, at
// your height and twice it, and one low. Their light is the stone's; it is only shaped like
// hands. One at your height is a band brighter, and fades over the first minutes.
static const u16 PRINT[4] = {
    0x5A7E,   // a palm:  .x.x / x.x. / xxxx / xxxx  (4x4, rows high nibble first)
    0xA5F6,   // a paw
    0x9969,   // three fingers
    0xDBF6,   // six
};
static void Prints(void) {
    if (printsA <= 0.02f || !InView(6 * TS, 11 * TS, 64)) return;
    for (int k = 0; k < 26; k++) {
        u32 h = Hash2(k, 77);
        int x = TS + 2 + (int)(h % (11 * TS)), y = 7 * TS + (int)((h >> 8) % (7 * TS));
        if (k == 0) { x = 6 * TS + 3; y = 12 * TS + 1; }            // yours: at your height, by the door
        if (k == 1) { x = 3 * TS; y = 13 * TS + 3; }                 // one low, where something crawled
        u16 p = PRINT[(h >> 16) & 3];
        f32 own = k == 0 ? 1.0f - fminf((f32)frameNo / (60 * 180), 1.0f) : 0;
        for (int j = 0; j < 4; j++) for (int i = 0; i < 4; i++) {
            if (!((p >> (15 - (j * 4 + i))) & 1)) continue;
            if ((Hash2(x + i, y + j) & 255) > (u32)(printsA * 255)) continue;
            DrawRectangle(x + i, ROOM_Y + y + j, 1, 1, PAL[own > 0.3f ? PL_COOLM : PL_COOLD]);
        }
    }
}

static void Leaves(void) {
    for (int i = 0; i < leafN; i++) {
        int x = (int)leaves[i].x, y = (int)leaves[i].y;
        if (!InView((f32)x, (f32)y, 8)) continue;
        int turn = leaves[i].down ? (i & 1) : ((int)(leaves[i].ph * 2) & 1);
        Px(x, y, PL_WARM, TAG_THING);
        Px(x + (turn ? 1 : -1), y, PL_WARMD, TAG_THING);
        if (!leaves[i].down) Px(x, y - 1, PL_WARMD, TAG_THING);
    }
}

// The dead lamps, in a row in a recess at the back of the undercroft, set down tidily; at the
// end of the row a clean patch in the dust, the size of a lamp's foot.
static void DeadLamps(void) {
    if (!InView(NICHE_X0, CAMP_Y, 64)) return;
    int y = CAMP_Y;
    for (int x = NICHE_X0 - 4; x < PATCH_X + 10; x++)
        for (int j = 1; j < 14; j++) {
            int edge = x == NICHE_X0 - 4 || x == PATCH_X + 9 || j == 13;
            Px(x, y - j, edge ? PL_STONE : (j < 3 ? PL_DEEP : PL_VOID), TAG_WALL);
        }
    for (int k = 0; k < NICHE_N; k++) {
        DrawSpriteTag(&DEAD_LAMP, NICHE_X0 + k * 7, ROOM_Y + y - 6, 0, TAG_THING);
        Px(NICHE_X0 + k * 7 + 1, y - 7, PL_DARK, TAG_THING);          // dust on the cap
    }
    for (int x = NICHE_X0 - 3; x < PATCH_X + 9; x++) if (x < PATCH_X - 1 || x > PATCH_X + 4) Px(x, y - 1, PL_DARK, TAG_WALL);   // the dust
}

void HallDrawBack(void) {
    MuralRelief();
    DeadLamps();
}

void HallDraw(void) {
    // the hunter
    if (InView(HUNTER_X, CAMP_Y, 40)) {
        int fire = PropFireLit(0);
        const Sprite *s = fire ? &HUNTER_WARM : &HUNTER;
        int x = HUNTER_X, y = ROOM_Y + CAMP_Y - s->h;
        DrawSpriteTag(s, x, y, 0, TAG_THING);
        if (hunterLook && !fire) { Color c = PAL[PL_DEEP]; c.a = TAG_THING; DrawRectangle(x + 5, y + 3, 1, 1, c); }   // the eye, a pixel over
        if (hunterArm > 0 && !fire) {                                    // reaching to the cairn's top stone
            int up = hunterArm > 35 ? 50 - hunterArm : (hunterArm < 15 ? hunterArm : 15);
            Color c = PAL[PL_BONE]; c.a = TAG_THING;
            DrawRectangle(x - 1 - up / 5, y + 5 - up / 6, 2, 1, c);
        }
    }
    // the sitters, their backs to you, facing the city
    for (int i = 0; i < 3; i++) {
        if (!InView(SITX[i], SILL_Y, 40)) continue;
        const Sprite *s = i == 2 ? &SITTER_BONES : &SITTER;
        int y = ROOM_Y + SILL_Y - s->h;
        DrawSpriteTag(s, SITX[i], y, 0, TAG_THING);
        if (i == 0 && sitLook) { Color c = PAL[PL_WARMD]; c.a = TAG_THING; DrawRectangle(SITX[i] - 1, y + 2, 1, 1, c); }   // the brim, turned a pixel
    }
    Leaves();
}

void HallDrawEmis(void) {
    Prints();
    MuralPhosphor();
    // the spilled glass by the pack: chips prised from the vault's throat, brighter as your
    // lamp comes near -- they drink from it
    if (InView(35 * TS, CAMP_Y, 40)) {
        f32 d = LampDist(36 * TS, CAMP_Y - 2);
        for (int k = 0; k < 14; k++) {
            u32 h = Hash2(k, 919);
            int x = 34 * TS + (int)(h % 40), y = CAMP_Y - 1 - (int)((h >> 8) % 2);
            int pl = d < 3 * TS ? PL_CITYH : (d < 7 * TS ? PL_CITY : ((h >> 12) & 1 ? PL_COOLM : PL_COOLD));
            DrawRectangle(x, ROOM_Y + y, 1, 1, PAL[pl]);
        }
    }
    // the fish
    for (int i = 0; i < 6; i++) {
        if (!InView(fish[i].x, fish[i].y, 8)) continue;
        int x = (int)fish[i].x, y = ROOM_Y + (int)fish[i].y, dir = fish[i].vx > 0 ? 1 : -1;
        DrawRectangle(x, y, 2, 1, PAL[PL_CITY]);
        DrawRectangle(x - dir, y, 1, 1, PAL[PL_COOLM]);
    }
}

void HallLights(void) {
    // the glass by the pack gives a breath of their light
    LightAddPointCool(36 * TS, CAMP_Y - 2, 2.5f, 0.25f);
    // and once the fire is lit, its glow reaches the colossus's fingertips over it: they take
    // an amber rim, held out over the fire as the hunter's hands are
    if (PropFireLit(0)) LightAddPoint(36 * TS + 4, 31 * TS, 5.0f, 0.95f + 0.1f * sinf(frameNo * 0.19f));
}

// For city.c: how far the light beyond the fireguard is out, and how many steps nearer.
f32 HallDouse(void) { return douse; }
int HallNearer(void) { return nearer; }
