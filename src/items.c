// items.c -- the things you can hold. One hand: the lamp, or a stone, never both.
//
// They are objects, not powers. They fall, land on stone and shelves, drop through the
// grate, and stay where you leave them when you change rooms. The lamp floats and
// gives light. A stone sinks, and while you hold it so do you: you jump lower, fall
// harder, and in water you go to the bottom and walk it. Set it down there and you
// float back up without it. It is still there. You can see it. That is the point.
#include "aw.h"
#include <math.h>
#include <string.h>

Item items[ITEM_MAX];
int  itemCount;
int  heldItem = -1;

#define LAMP_R    7.4f
#define LAMP_PEAK 0.90f

static u32 rng = 0x1A5B7C9Du;
static float Rnd(void) { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return (float)(rng & 0xFFFF) / 65535.0f - 0.5f; }

static int W(const Item *it) { return it->kind == IT_LAMP ? 4 : 5; }
static int H(const Item *it) { return it->kind == IT_LAMP ? 5 : 4; }

int ItemsAdd(int kind, int room, int tx, int ty) {
    if (itemCount >= ITEM_MAX) return -1;
    Item *it = &items[itemCount];
    memset(it, 0, sizeof *it);
    it->kind = kind; it->room = room;
    it->x = tx * TS + (TS - W(it)) * 0.5f;
    it->y = (ty + 1) * TS - H(it);
    return itemCount++;
}
void ItemsReset(void) { itemCount = 0; heldItem = -1; }

int PlayerHolds(void) { return heldItem < 0 ? IT_NONE : items[heldItem].kind; }

// Where a held thing hangs: at your side, a little behind your lean; a stone lower.
static void HeldPos(const Item *it, float *x, float *y) {
    int bob = (player.onGround && fabsf(player.vx) > 0.05f) ? ((int)player.animT & 1) : 0;
    float lag = -player.leanX * 0.8f;
    if (lag >  2) lag =  2;
    if (lag < -2) lag = -2;
    *x = player.x + (player.facing > 0 ? player.w + 1.0f : -W(it) - 1.0f) + lag;
    *y = player.y + (it->kind == IT_LAMP ? 4.0f : 6.0f) + bob;
}

static int Blocked(const Item *it, float x, float y) {
    int tx0 = (int)floorf(x / TS), tx1 = (int)floorf((x + W(it) - 1) / TS);
    int ty0 = (int)floorf(y / TS), ty1 = (int)floorf((y + H(it) - 1) / TS);
    for (int ty = ty0; ty <= ty1; ty++)
        for (int tx = tx0; tx <= tx1; tx++)
            if (TileSolid(TileGet(tx, ty))) return 1;
    return 0;
}

// Falling, a thing is caught by whatever would catch you: stone, and a shelf's top
// edge crossed from above. In water the lamp rises to rest with its glass above the
// line; a stone goes down, slowly, to the floor.
static void Fall(Item *it) {
    float cx = it->x + W(it) * 0.5f;
    int wet = TileWater(TileAtPx(cx, it->y + H(it) - 1.0f));
    if (wet) {
        if (it->kind == IT_LAMP) {
            int under = TileWater(TileAtPx(cx, it->y + 3.5f));
            it->vy += under ? -0.10f : 0.05f;
            it->vy *= 0.86f;
        } else {
            it->vy += 0.06f;
            if (it->vy > 0.7f) it->vy = 0.7f;
        }
    } else {
        it->vy += 0.185f;
        if (it->vy > 2.9f) it->vy = 2.9f;
    }
    float rem = it->vy;
    it->onGround = 0;
    while (fabsf(rem) > 0.0001f) {
        float step = rem > 1 ? 1 : (rem < -1 ? -1 : rem);
        float ny = it->y + step;
        int blocked = Blocked(it, it->x, ny);
        if (!blocked && step > 0) {
            float ob = it->y + H(it), nb = ny + H(it);
            int ty0 = (int)floorf(ob / TS), ty1 = (int)floorf(nb / TS);
            int tx0 = (int)floorf(it->x / TS), tx1 = (int)floorf((it->x + W(it) - 1) / TS);
            for (int ty = ty0; ty <= ty1 && !blocked; ty++)
                for (int tx = tx0; tx <= tx1 && !blocked; tx++)
                    if (TileOneWay(TileGet(tx, ty)) && ob <= ty * TS + 0.001f && nb > ty * TS) blocked = 1;
        }
        if (blocked) {
            if (step > 0) {
                if (it->vy > 1.0f) {
                    if (it->kind == IT_LAMP) Sfx(SFX_SETDOWN, 0.5f + 0.2f * it->vy, 0.9f + Rnd() * 0.1f, cx / GW);
                    else { Sfx(SFX_STONE, 0.5f + 0.25f * it->vy, 0.9f + Rnd() * 0.1f, cx / GW);
                           FxBurst(FX_DUST, cx, ny + H(it), (int)(2 + it->vy * 2), 0.8f, 0.25f); }
                } else if (wet && it->kind == IT_STONE && it->vy > 0.3f) {
                    Sfx(SFX_STONE, 0.25f, 0.7f, cx / GW);              // meeting the floor, under
                }
                it->onGround = 1;
            }
            it->vy = 0; return;
        }
        it->y = ny; rem -= step;
    }
    if (it->y > RH * TS + 8 && it->room < ROOM_COUNT - 1) {      // through the floor: the room below
        it->room++; it->y -= RH * TS;
    }
}

void ItemsStep(void) {
    for (int i = 0; i < itemCount; i++) {
        Item *it = &items[i];
        if (it->kind == IT_LAMP) { it->flick += Rnd() * 0.16f; it->flick *= 0.90f; }
        if (it->cool > 0) it->cool--;
    }
    if (heldItem >= 0) {
        Item *it = &items[heldItem];
        it->room = roomIdx;                          // it goes where you go
        HeldPos(it, &it->x, &it->y);
        it->vy = 0;
        if (in.actPressed && it->cool == 0) {
            // Set down at your feet, on the side you face; if that is inside stone, at
            // your feet exactly. Airborne or afloat, it simply leaves your hand.
            float x = player.x + (player.facing > 0 ? player.w + 1.0f : -W(it) - 1.0f);
            float y = player.y + player.h - H(it);
            if (Blocked(it, x, y)) x = player.x + (player.w - W(it)) * 0.5f;
            it->x = x; it->y = y; it->cool = 8;
            heldItem = -1;
            Sfx(it->kind == IT_LAMP ? SFX_SETDOWN : SFX_STONE, 0.6f, 1.0f + Rnd() * 0.1f, x / GW);
        }
    }
    // Everything not in hand falls. Then, if your hands are free and you asked, take
    // the nearest thing within reach.
    int best = -1; float bestD = 1e9f;
    for (int i = 0; i < itemCount; i++) {
        Item *it = &items[i];
        if (i == heldItem || it->room != roomIdx) continue;
        Fall(it);
        float cx = it->x + W(it) * 0.5f, cy = it->y + H(it) * 0.5f, pcx = player.x + player.w * 0.5f;
        float d = fabsf(cx - pcx);
        if (d < 11.0f && cy > player.y - 6.0f && cy < player.y + player.h + 6.0f && d < bestD) { best = i; bestD = d; }
    }
    if (heldItem < 0 && best >= 0 && in.actPressed && items[best].cool == 0) {
        heldItem = best; items[best].cool = 8;
        Item *it = &items[best];
        Sfx(it->kind == IT_LAMP ? SFX_PICKUP : SFX_STONE_UP, 0.7f, 0.95f + Rnd() * 0.1f, it->x / GW);
    }
    player.heavy = PlayerHolds() == IT_STONE;
}

void ItemsLight(void) {
    for (int i = 0; i < itemCount; i++) {
        Item *it = &items[i];
        if (it->kind != IT_LAMP || (i != heldItem && it->room != roomIdx)) continue;
        LightAddPoint(it->x + W(it) * 0.5f, it->y + 2.0f, LAMP_R, LAMP_PEAK * (1.0f + it->flick * 0.35f));
    }
}

static void DrawOne(const Item *it) {
    int x = (int)floorf(it->x), y = ROOM_Y + (int)floorf(it->y);
    if (it->kind == IT_LAMP) {
        DrawRectangle(x + 1, y - 1, 2, 1, palLampIron);            // the loop you hold it by
        DrawRectangle(x, y, 4, 5, palLampIron);
        DrawRectangle(x + 1, y + 1, 2, 3, palLampGlass);
    } else {
        // a stone: a lump with a lit top and a dark underside, wider at the bottom
        DrawRectangle(x + 1, y, 3, 1, palStone);
        DrawRectangle(x, y + 1, 5, 3, palStone);
        DrawRectangle(x + 1, y, 2, 1, palStoneLit);
        DrawRectangle(x, y + 3, 5, 1, palStoneDeep);
        DrawRectangle(x + 3, y + 2, 1, 1, palStoneDeep);
    }
}
void ItemsDrawBehind(void) { for (int i = 0; i < itemCount; i++) if (i != heldItem && items[i].room == roomIdx) DrawOne(&items[i]); }
void ItemsDrawHeld(void)   { if (heldItem >= 0) DrawOne(&items[heldItem]); }

// The lamp's glass again, after the light pass: the one thing that is always bright.
void ItemsDrawCore(void) {
    for (int i = 0; i < itemCount; i++) {
        Item *it = &items[i];
        if (it->kind != IT_LAMP || (i != heldItem && it->room != roomIdx)) continue;
        int x = (int)floorf(it->x), y = ROOM_Y + (int)floorf(it->y);
        DrawRectangle(x + 1, y + 1, 2, 3, palLampGlass);
        DrawRectangle(x + 1 + (it->flick > 0.05f ? 1 : 0), y + 2, 1, 1, palLampHot);
    }
}
