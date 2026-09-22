// sprites.c -- every drawn thing, as rows of palette letters. Generated once from the
// first drafts and edited by hand since; tools/sprites.py renders them all to a sheet.
//
// The letters name the palette of the new look (PAL in render.c), so what is drawn here
// is what the composite snaps to:
//   0 void  1 deep  2 dark  3 stone  4 stone-lit  5 stone-high
//   w warm-dark  W warm  a amber  A amber-high  c cool-dark  C cool  g city  G city-high
//   b bone  u water  U water-lit  d water-dark  r accent  . nothing
#include "aw.h"

static int Ink(char c) {
    switch (c) {
        case '0': return PL_VOID;  case '1': return PL_DEEP;   case '2': return PL_DARK;
        case '3': return PL_STONE; case '4': return PL_STONEL; case '5': return PL_STONEH;
        case 'w': return PL_WARMD; case 'W': return PL_WARM;   case 'a': return PL_AMBER; case 'A': return PL_AMBERH;
        case 'c': return PL_COOLD; case 'C': return PL_COOLM;  case 'g': return PL_CITY;  case 'G': return PL_CITYH;
        case 'b': return PL_BONE;  case 'u': return PL_WATER;  case 'U': return PL_WATERL; case 'd': return PL_WATERD;
        case 'r': return PL_ACCENT;
        default:  return -1;
    }
}

// The alpha a pixel goes down with tells the composite what it is: 255 the far wall, 254 a
// drawn thing (a clean band edge instead of dither, which on a body is a checkerboard), 253
// stone and shelves (a silhouette, with rim light on its drawn edge rather than the tile's).
static u8 tagNow = 254;
static Color Tag(int k) { Color c = PAL[k]; c.a = tagNow; return c; }

// Any rectangle of a sprite, as a given kind of pixel: the far wall tiles 16x16 art per 8x8
// tile this way.
void DrawSpriteRect(const Sprite *s, int px, int py, int sx, int sy, int w, int h, int flip, u8 tag) {
    u8 was = tagNow; tagNow = tag;
    for (int y = sy; y < sy + h && y < s->h; y++) {
        const char *row = s->rows[y];
        int x = sx;
        while (x < sx + w && x < s->w) {
            int k = Ink(row[flip ? s->w - 1 - x : x]);
            if (k < 0) { x++; continue; }
            int x0 = x;
            while (x < sx + w && x < s->w && Ink(row[flip ? s->w - 1 - x : x]) == k) x++;
            DrawRectangle(px + x0 - sx, py + y - sy, x - x0, 1, Tag(k));
        }
    }
    tagNow = was;
}
void DrawSpriteTag(const Sprite *s, int px, int py, int flip, u8 tag) {
    u8 was = tagNow; tagNow = tag; DrawSpriteEx(s, px, py, flip); tagNow = was;
}

// Draw a sprite with its top-left at (px, py); flip mirrors it, for things facing left.
// Runs of one letter go out as one rectangle, so a 40x48 door is not 1920 quads.
void DrawSpriteEx(const Sprite *s, int px, int py, int flip) {
    for (int y = 0; y < s->h; y++) {
        const char *row = s->rows[y];
        int x = 0;
        while (x < s->w) {
            int k = Ink(row[flip ? s->w - 1 - x : x]);
            if (k < 0) { x++; continue; }
            int x0 = x;
            while (x < s->w && Ink(row[flip ? s->w - 1 - x : x]) == k) x++;
            DrawRectangle(px + x0, py + y, x - x0, 1, Tag(k));
        }
    }
}
void DrawSprite(const Sprite *s, int px, int py) { DrawSpriteEx(s, px, py, 0); }

void DrawSpriteRows(const Sprite *s, int px, int py, int flip, int y0, int y1, int ink) {
    if (y0 < 0) y0 = 0;
    if (y1 > s->h) y1 = s->h;
    for (int y = y0; y < y1; y++) {
        const char *row = s->rows[y];
        for (int x = 0; x < s->w; x++) {
            int k = Ink(row[flip ? s->w - 1 - x : x]);
            if (k < 0) continue;
            DrawRectangle(px + x, py + y, 1, 1, Tag(ink >= 0 ? ink : k));
        }
    }
}

static const char *const SPR_PLAYER_IDLE_ROWS[] = {
    ".bbbb.",
    "bbbbbb",
    "bbbbbb",
    "bbbbbb",
    "bbbbb5",
    ".bbb5.",
    "bbbbb5",
    "bbbbb5",
    "bbbb55",
    ".5554.",
    ".4..4.",
};
const Sprite SPR_PLAYER_IDLE = { 6, 11, SPR_PLAYER_IDLE_ROWS };

static const char *const SPR_PLAYER_WALK1_ROWS[] = {
    ".bbbb.",
    "bbbbbb",
    "bbbbbb",
    "bbbbbb",
    "bbbbb5",
    ".bbb5.",
    "bbbbb5",
    "bbbbb5",
    "bbbb55",
    ".5554.",
    "4...4.",
};
const Sprite SPR_PLAYER_WALK1 = { 6, 11, SPR_PLAYER_WALK1_ROWS };

static const char *const SPR_PLAYER_WALK2_ROWS[] = {
    ".bbbb.",
    "bbbbbb",
    "bbbbbb",
    "bbbbbb",
    "bbbbb5",
    ".bbb5.",
    "bbbbb5",
    "bbbbb5",
    "bbbb55",
    ".5554.",
    "..44..",
};
const Sprite SPR_PLAYER_WALK2 = { 6, 11, SPR_PLAYER_WALK2_ROWS };

static const char *const SPR_PLAYER_JUMP_ROWS[] = {
    ".bbbb.",
    "bbbbbb",
    "bbbbbb",
    "bbbbbb",
    "bbbbb5",
    ".bbb5.",
    "bbbbb5",
    "bbbbb5",
    "bbbb55",
    ".5555.",
    "......",
};
const Sprite SPR_PLAYER_JUMP = { 6, 11, SPR_PLAYER_JUMP_ROWS };

static const char *const SPR_PLAYER_FALL_ROWS[] = {
    ".bbbb.",
    "bbbbbb",
    "bbbbbb",
    "bbbbbb",
    "bbbbb5",
    ".bbb5.",
    "bbbbb5",
    "bbbbb5",
    "bbbb55",
    ".5554.",
    "4....4",
};
const Sprite SPR_PLAYER_FALL = { 6, 11, SPR_PLAYER_FALL_ROWS };

static const char *const SPR_BIRD_PERCH_ROWS[] = {
    ".....33.",
    "....333a",
    "33.3335.",
    ".333355.",
    "..3555..",
    "...2.2..",
};
const Sprite SPR_BIRD_PERCH = { 8, 6, SPR_BIRD_PERCH_ROWS };

static const char *const SPR_BIRD_LOOK_ROWS[] = {
    "........",
    ".....33.",
    "33..333a",
    ".333355.",
    "..3555..",
    "...2.2..",
};
const Sprite SPR_BIRD_LOOK = { 8, 6, SPR_BIRD_LOOK_ROWS };

static const char *const SPR_BIRD_UP_ROWS[] = {
    "3.....3",
    ".3...3.",
    "..333a.",
    "..35...",
};
const Sprite SPR_BIRD_UP = { 7, 4, SPR_BIRD_UP_ROWS };

static const char *const SPR_BIRD_DOWN_ROWS[] = {
    ".......",
    "..333a.",
    ".3.35.3",
    "3.....3",
};
const Sprite SPR_BIRD_DOWN = { 7, 4, SPR_BIRD_DOWN_ROWS };

static const char *const SPR_BEAST_STAND_ROWS[] = {
    "....WWWW....",
    "..WWwWWWWW..",
    ".WWWWWWWwWW.",
    "WWWWWWWWWWWw",
    ".wwwwWWWwww.",
    "..w.w..w.w..",
    "..w.w..w.w..",
};
const Sprite SPR_BEAST_STAND = { 12, 7, SPR_BEAST_STAND_ROWS };

static const char *const SPR_BEAST_WALK1_ROWS[] = {
    "....WWWW....",
    "..WWwWWWWW..",
    ".WWWWWWWwWW.",
    "WWWWWWWWWWWw",
    ".wwwwWWWwww.",
    ".w..w.w..w..",
    "w...w.w...w.",
};
const Sprite SPR_BEAST_WALK1 = { 12, 7, SPR_BEAST_WALK1_ROWS };

static const char *const SPR_BEAST_WALK2_ROWS[] = {
    "....WWWW....",
    "..WWwWWWWW..",
    ".WWWWWWWwWW.",
    "WWWWWWWWWWWw",
    ".wwwwWWWwww.",
    "..ww....ww..",
    "..w.w..w.w..",
};
const Sprite SPR_BEAST_WALK2 = { 12, 7, SPR_BEAST_WALK2_ROWS };

static const char *const SPR_BEAST_SIT_ROWS[] = {
    "............",
    "............",
    ".....WWWW...",
    "..WWwWWWWWW.",
    ".WWWWWWWwWWW",
    "wwwwwwwwwwww",
    "........w.w.",
};
const Sprite SPR_BEAST_SIT = { 12, 7, SPR_BEAST_SIT_ROWS };

static const char *const SPR_BEAST_HEAD_ROWS[] = {
    ".W...",
    "WWW..",
    "WWWWW",
    ".ww2.",
};
const Sprite SPR_BEAST_HEAD = { 5, 4, SPR_BEAST_HEAD_ROWS };

static const char *const SPR_POD_ROWS[] = {
    ".r.",
    "rbr",
    "rrr",
    ".r.",
};
const Sprite SPR_POD = { 3, 4, SPR_POD_ROWS };

static const char *const SPR_POD_OPEN_ROWS[] = {
    ".r.",
    "r1r",
    "r1r",
    ".r.",
};
const Sprite SPR_POD_OPEN = { 3, 4, SPR_POD_OPEN_ROWS };

static const char *const SPR_BUSH_ROWS[] = {
    "...cc...",
    "..cCCc..",
    ".cCCrCc.",
    "cCCCCCCc",
    "cCrCCCrc",
    ".cCCCCc.",
    "..cccc..",
    "...22...",
};
const Sprite SPR_BUSH = { 8, 8, SPR_BUSH_ROWS };

static const char *const SPR_LAMP_ROWS[] = {
    ".33.",
    "3223",
    "3AA3",
    "3aa3",
    "3333",
    ".22.",
};
const Sprite SPR_LAMP = { 4, 6, SPR_LAMP_ROWS };

static const char *const SPR_STONE_ROWS[] = {
    ".445.",
    "44455",
    "34444",
    ".333.",
};
const Sprite SPR_STONE = { 5, 4, SPR_STONE_ROWS };

static const char *const SPR_POT_ROWS[] = {
    ".WaW.",
    ".w1w.",
    "WWWWW",
    "aWWWw",
    "WWWWw",
    ".www.",
};
const Sprite SPR_POT = { 5, 6, SPR_POT_ROWS };

static const char *const SPR_BEDROLL_ROWS[] = {
    ".WWWWWWWWWWWWWW.",
    "WaWWWWWwWWWWWWaW",
    "WWWWWWWwWWWWWWWW",
    ".wwwwwwwwwwwwww.",
};
const Sprite SPR_BEDROLL = { 16, 4, SPR_BEDROLL_ROWS };

static const char *const SPR_PACK_ROWS[] = {
    ".ww......",
    "wWWw..33.",
    "wWWw.3223",
    "w22w.3113",
    "wWWw.3333",
    ".ww...22.",
};
const Sprite SPR_PACK = { 9, 6, SPR_PACK_ROWS };

static const char *const SPR_CAIRN_ROWS[] = {
    "...45..",
    "..3455.",
    "..3333.",
    ".344455",
    ".333333",
    "3444455",
    "3333333",
};
const Sprite SPR_CAIRN = { 7, 7, SPR_CAIRN_ROWS };

static const char *const SPR_BONES_ROWS[] = {
    "......bb.",
    ".....b1bb",
    "......bb.",
    ".....5b5.",
    ".....5b5.",
    "..5bb.b..",
    "bbbb5.5b.",
};
const Sprite SPR_BONES = { 9, 7, SPR_BONES_ROWS };

static const char *const SPR_FIRE_RING_ROWS[] = {
    "33.2.33",
    ".33333.",
};
const Sprite SPR_FIRE_RING = { 7, 2, SPR_FIRE_RING_ROWS };

static const char *const SPR_BALUSTRADE_ROWS[] = {
    "55555555",
    ".4..4..4",
    ".3..3..3",
    ".4..4..4",
    ".3..3..3",
    ".3..3..3",
    "44444444",
};
const Sprite SPR_BALUSTRADE = { 8, 7, SPR_BALUSTRADE_ROWS };

static const char *const SPR_DOOR_ROWS[] = {
    "........................................",
    "..............555555555555..............",
    "...........555444444444444555...........",
    "..........55444444444444444455..........",
    "........554444222222222222444455........",
    ".......55444222cccccccccc22244455.......",
    "......544442cccccccccccccccc244445......",
    ".....544422cccccccccccccccccc224445.....",
    "....554422cccccccccccccccccccc224455....",
    "....54422cccccccccccccccccccccc22445....",
    "...54442cccccccccccCCCCCcccccccc24445...",
    "..55442ccccccccccccCccccCCccccccc24455..",
    "..5442cccccccccccccCccccccCccccccc2445..",
    "..5442cccccccccccccCcccccccCCccccc2445..",
    ".54422cccccccccccccCccccccccCCcccc22445.",
    ".5442cccccccccccccCCccccccccccCcccc2445.",
    ".5442cccccccccCCCCcCCCCcccccccCCccc2445.",
    ".5442cccccccCCcccccCcccCCccccccCccc2445.",
    ".5442ccCCccCCccccccCccccCCcccccCCcc2445.",
    ".5442ccccCCCcccccccCcccccCcccCCcCcc2445.",
    "5442ccccccCCcccccccCccccccCcCCccCccc2445",
    "5442cccccCccCCcccCCCccccccCCcccccCcc2445",
    "5342cccccCccccCCCCcCCCccCCcCcccccCcc2435",
    "5442ccccCccccccCCccccCCCcccCcccccCcc2445",
    "5442ccccCcccccCCccgggCcccccCcccccCcc2445",
    "5442ccccCcccccCcccgggCcccccCcccccCcc2445",
    "5442ccccCcccccCcccgggccccccCcccccCcc2445",
    "5442ccccCcccccCcCcccccCcccCCcccccCcc2445",
    "5342ccccCcccccCCcccccccCCCCcccccCccc2435",
    "5442ccccCccCCCcCcccCcccccCCCccccCccc2445",
    "5442cccccCCcccccCccCccccCCccCCccCccc2445",
    "5442ccccCCcccccccCCCcCCCCccccCCCcccc2445",
    "5442cccCccCccccccccCCcccccccccCCcccc2445",
    "5442ccccccCCcccccccCcccccccccCCccccc2445",
    "5342cccccccCCccccccCcccccccccCcccccc2435",
    "5442ccccccccCCcccccCcccccccCCccccccc2445",
    "5442cccccccccCCCcccCcccccCCccccccccc2445",
    "5442cccccccccccCCCCCCCCCCCcccccccccc2445",
    "5442cccccccccccccccCcccccccccccccccc2445",
    "5442cccccccccccccccCcccccccccccccccc2445",
    "5342cccccccccccccccccccccccccccccccc2435",
    "5442cccccccccccccccccccccccccccccccc2445",
    "5442cccccccccccccccccccccccccccccccc2445",
    "5442cccccccccccccccccccccccccccccccc2445",
    "5442cccccccccccccccccccccccccccccccc2445",
    "5442cccccccccccccccccccccccccccccccc2445",
    "5442222222222222222222222222222222222445",
    "5555555555555555555555555555555555555555",
};
const Sprite SPR_DOOR = { 40, 48, SPR_DOOR_ROWS };

static const char *const SPR_RK_I1_ROWS[] = {
    "3333",
    "3233",
    "3332",
    "3333",
};
const Sprite SPR_RK_I1 = { 4, 4, SPR_RK_I1_ROWS };

static const char *const SPR_RK_I2_ROWS[] = {
    "3343",
    "3333",
    "2333",
    "3332",
};
const Sprite SPR_RK_I2 = { 4, 4, SPR_RK_I2_ROWS };

static const char *const SPR_RK_I3_ROWS[] = {
    "3333",
    "3333",
    "3323",
    "3233",
};
const Sprite SPR_RK_I3 = { 4, 4, SPR_RK_I3_ROWS };

static const char *const SPR_RK_I4_ROWS[] = {
    "3333",
    "3334",
    "3333",
    "3333",
};
const Sprite SPR_RK_I4 = { 4, 4, SPR_RK_I4_ROWS };

static const char *const SPR_RK_T1_ROWS[] = {
    "5544",
    "4443",
    "3333",
    "3323",
};
const Sprite SPR_RK_T1 = { 4, 4, SPR_RK_T1_ROWS };

static const char *const SPR_RK_T2_ROWS[] = {
    "4554",
    "3444",
    "3333",
    "3333",
};
const Sprite SPR_RK_T2 = { 4, 4, SPR_RK_T2_ROWS };

static const char *const SPR_RK_T3_ROWS[] = {
    ".554",
    "5443",
    "3333",
    "3233",
};
const Sprite SPR_RK_T3 = { 4, 4, SPR_RK_T3_ROWS };

static const char *const SPR_RK_L1_ROWS[] = {
    "4333",
    "4333",
    "4323",
    "4333",
};
const Sprite SPR_RK_L1 = { 4, 4, SPR_RK_L1_ROWS };

static const char *const SPR_RK_L2_ROWS[] = {
    ".433",
    "4333",
    "4333",
    ".433",
};
const Sprite SPR_RK_L2 = { 4, 4, SPR_RK_L2_ROWS };

static const char *const SPR_RK_OT_ROWS[] = {
    "..54",
    ".544",
    "5443",
    "4333",
};
const Sprite SPR_RK_OT = { 4, 4, SPR_RK_OT_ROWS };

static const char *const SPR_RK_NT_ROWS[] = {
    "4333",
    "3333",
    "3333",
    "3332",
};
const Sprite SPR_RK_NT = { 4, 4, SPR_RK_NT_ROWS };

static const char *const SPR_RK_B1_ROWS[] = {
    "3333",
    "3332",
    "2222",
    ".11.",
};
const Sprite SPR_RK_B1 = { 4, 4, SPR_RK_B1_ROWS };

static const char *const SPR_RK_B2_ROWS[] = {
    "3323",
    "3333",
    "2222",
    "1..1",
};
const Sprite SPR_RK_B2 = { 4, 4, SPR_RK_B2_ROWS };

static const char *const SPR_RK_OB_ROWS[] = {
    "4333",
    "4332",
    ".222",
    "..1.",
};
const Sprite SPR_RK_OB = { 4, 4, SPR_RK_OB_ROWS };

static const char *const SPR_RK_NB_ROWS[] = {
    "3333",
    "3333",
    "3333",
    "2333",
};
const Sprite SPR_RK_NB = { 4, 4, SPR_RK_NB_ROWS };

static const char *const SPR_ASH1_ROWS[] = {
    "24444444",
    "23333333",
    "23333333",
    "22222222",
    "44442444",
    "33332333",
    "33332333",
    "22222222",
};
const Sprite SPR_ASH1 = { 8, 8, SPR_ASH1_ROWS };

static const char *const SPR_ASH2_ROWS[] = {
    "24444444",
    "23333323",
    "23332333",
    "22222222",
    "44442444",
    "33432333",
    "33332333",
    "22222222",
};
const Sprite SPR_ASH2 = { 8, 8, SPR_ASH2_ROWS };

static const char *const SPR_VEIN1_ROWS[] = {
    "........",
    "....A...",
    "...aAa..",
    "..aAa...",
    "..aa.a..",
    ".....Aa.",
    "....aa..",
    "........",
};
const Sprite SPR_VEIN1 = { 8, 8, SPR_VEIN1_ROWS };

static const char *const SPR_VEIN2_ROWS[] = {
    "........",
    "..A.....",
    ".aAa....",
    "..aa.A..",
    "....aAa.",
    "...aAa..",
    "........",
    "........",
};
const Sprite SPR_VEIN2 = { 8, 8, SPR_VEIN2_ROWS };

static const char *const SPR_GLASS_ROWS[] = {
    "23333332",
    "3cCCCCc3",
    "3CgGGgC3",
    "3CGggGC3",
    "3CGggGC3",
    "3CgGGgC3",
    "3cCCCCc3",
    "23333332",
};
const Sprite SPR_GLASS = { 8, 8, SPR_GLASS_ROWS };

static const char *const SPR_PLANK1_ROWS[] = {
    "WaWWWWaW",
    "WWWwWWWW",
    "wwwwwwww",
};
const Sprite SPR_PLANK1 = { 8, 3, SPR_PLANK1_ROWS };

static const char *const SPR_PLANK2_ROWS[] = {
    "WWWWaWWW",
    "WwWWWWwW",
    "wwwwwwww",
};
const Sprite SPR_PLANK2 = { 8, 3, SPR_PLANK2_ROWS };

static const char *const SPR_PLANK_END_ROWS[] = {
    ".aWWWWWW",
    "wWWwWWWW",
    ".wwwwwww",
};
const Sprite SPR_PLANK_END = { 8, 3, SPR_PLANK_END_ROWS };

static const char *const SPR_CORNICE_ROWS[] = {
    "55555555",
    "44444444",
    "33333333",
    "2.2.2.2.",
};
const Sprite SPR_CORNICE = { 8, 4, SPR_CORNICE_ROWS };

static const char *const SPR_CORNICE_END_ROWS[] = {
    ".5555555",
    "44444444",
    ".3333333",
    "..2.2.2.",
};
const Sprite SPR_CORNICE_END = { 8, 4, SPR_CORNICE_END_ROWS };

static const char *const SPR_MOSS_H1_ROWS[] = {
    ".c..C.c.",
    ".C..C.C.",
    ".C..c.C.",
    ".c....c.",
    "......C.",
    "......c.",
    "........",
    "........",
};
const Sprite SPR_MOSS_H1 = { 8, 8, SPR_MOSS_H1_ROWS };

static const char *const SPR_MOSS_H2_ROWS[] = {
    "c.C..cC.",
    "C.C...C.",
    "c.C...c.",
    "..c.....",
    "........",
    "........",
    "........",
    "........",
};
const Sprite SPR_MOSS_H2 = { 8, 8, SPR_MOSS_H2_ROWS };

static const char *const SPR_MOSS_F1_ROWS[] = {
    "........",
    "........",
    "........",
    "........",
    "...C....",
    ".c.Cc.C.",
    ".CcCC.Cc",
    "cCCcCCCc",
};
const Sprite SPR_MOSS_F1 = { 8, 8, SPR_MOSS_F1_ROWS };

static const char *const SPR_LICHEN_ROWS[] = {
    "........",
    "..cC....",
    ".cCCc...",
    "..cc....",
    "......C.",
    ".....cCc",
    "......c.",
    "........",
};
const Sprite SPR_LICHEN = { 8, 8, SPR_LICHEN_ROWS };

static const char *const SPR_WALL_VAULT_ROWS[] = {
    "3333332333333333",
    "3333332333343333",
    "3433332333333333",
    "3333332222222233",
    "2222223333333323",
    "3333323333333323",
    "3343323333433323",
    "3333323333333322",
    "3333322222333333",
    "2222233332333333",
    "3333333332343333",
    "3333433332333333",
    "3333333332222222",
    "3333333332333333",
    "2222222222334333",
    "3333323333333333",
};
const Sprite SPR_WALL_VAULT = { 16, 16, SPR_WALL_VAULT_ROWS };

static const char *const SPR_WALL_CITY_ROWS[] = {
    "3333333233333332",
    "3333333233333332",
    "3433333233343332",
    "2222222222222222",
    "3332333333323333",
    "3332333343323333",
    "3332333333323333",
    "2222222222222222",
};
const Sprite SPR_WALL_CITY = { 16, 8, SPR_WALL_CITY_ROWS };
