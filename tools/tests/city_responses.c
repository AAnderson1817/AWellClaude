// Exercise the real response module and item physics. Stub only platform I/O.
#define main game_main
#include "../../src/main.c"
#undef main
#include <assert.h>

static void Enter(int room) {
    ItemsReset(); ItemsAdd(IT_LAMP,0,1,14); RoomLoad(); RoomEnter(room);
    PlayerInit(9,9); PropsReset(); memset(&in,0,sizeof in);
    memset(sfxCount,0,sizeof sfxCount);
}
static void Steps(int n) { while(n-->0) CityStep(); }
static float FishMeanY(void) {
    CityFish f[CITY_FISH_COUNT]; assert(CityFishViews(f,CITY_FISH_COUNT)==CITY_FISH_COUNT);
    float y=0; for(int i=0;i<CITY_FISH_COUNT;i++) y+=f[i].y;
    return y/CITY_FISH_COUNT;
}
static void CheckWaterFish(void) {
    CityFish f[CITY_FISH_COUNT]; assert(CityFishViews(f,CITY_FISH_COUNT)==CITY_FISH_COUNT);
    for(int i=0;i<CITY_FISH_COUNT;i++) {
        assert(isfinite(f[i].x)&&isfinite(f[i].y));
        assert(TileWater(TileAtPx(f[i].x,f[i].y)));
        assert(fabsf(f[i].vx)<.63f && fabsf(f[i].vy)<.63f);
    }
}
static void Windows(void) {
    for(int room=0;room<2;room++) {
        Enter(room); CityWindow w[3]; int count=CityWindowViews(w,3);
        assert(count==(room?3:1));
        for(int k=0;k<count;k++) {
            RoomEnter(room); CityWindowViews(w,3);
            items[0].room=room; items[0].x=w[k].x+w[k].w*.5f-2;
            items[0].y=w[k].y+w[k].h*.5f-2;
            Steps(30); CityWindowViews(w,3);
            assert(w[k].dark && w[k].light==0 && w[k].returnFrames==0);
            Steps(700); CityWindowViews(w,3); assert(w[k].dark && w[k].light==0);
            items[0].room=1-room;
            CityStep(); CityWindowViews(w,3);
            int wait=w[k].returnFrames; assert(wait>=300 && wait<=600);
            printf("WINDOW room=%d index=%d returnFrames=%d\n",room,k,wait);
            Steps(wait-1); CityWindowViews(w,3); assert(w[k].dark && w[k].returnFrames==1);
            CityStep(); CityWindowViews(w,3); assert(!w[k].dark);
            Steps(42); CityWindowViews(w,3); assert(w[k].light==1);
            // An interrupted return waits a complete new interval after departure.
            items[0].room=room; Steps(20); items[0].room=1-room; CityStep();
            CityWindowViews(w,3); assert(w[k].returnFrames>=300);
        }
        // There is life behind the glass, whether or not the player is there.
        RoomEnter(room); items[0].room=1-room;
        int sawCross=0; for(int t=0;t<1400;t++) { CityStep(); CityWindowViews(w,3); sawCross|=w[0].crossing; }
        assert(sawCross);
        // An occasional quiet murmur near the grille, never a repeating alert.
        player.x=w[0].x+w[0].w*.5f-3; player.y=w[0].y+w[0].h*.5f-5;
        int old=sfxCount[SFX_CITY_MURMUR]; Steps(1200);
        int calls=sfxCount[SFX_CITY_MURMUR]-old; assert(calls>=1 && calls<=2);
    }
    puts("PASS windows: all four lamp responses, 5–10s return, renewed wait, crossings and quiet proximity voice");
}
static void Mural(void) {
    Enter(0); CityMural m;
    items[0].room=1; Steps(120); CityMuralView(&m); assert(m.visibility==1);
    items[0].room=0; items[0].x=8*TS; items[0].y=17*TS;
    Steps(31); CityMuralView(&m); assert(m.visibility==0);
    items[0].room=1; Steps(60); CityMuralView(&m); assert(m.visibility>.49f && m.visibility<.51f);
    Steps(61); CityMuralView(&m); assert(m.visibility==1);
    // The real persistent fire, lit through the original two-second lamp rule,
    // conceals the mural even after the lamp leaves this room.
    items[0].room=0; items[0].x=16*TS+2; items[0].y=20*TS-5;
    for(int t=0;t<130;t++) { PropsStep(); CityStep(); }
    assert(PropFireLit(0)); items[0].room=1; Steps(31);
    CityMuralView(&m); assert(m.visibility==0);
    PropsReset(); Steps(121); CityMuralView(&m); assert(m.visibility==1);
    puts("PASS mural: light conceals, darkness restores over 2s, persistent fire changes what can be seen");
}
static void Face(void) {
    Enter(1); CityFace f;
    assert(bulbCount==0);
    for(int y=0;y<RH;y++) for(int x=0;x<RW;x++) assert(ZoneAt(x,y)==Z_CITY);
    CityFaceView(&f); float start=f.pulse; Steps(210); CityFaceView(&f);
    assert(f.pulse>start+.12f); Steps(210); CityFaceView(&f); assert(fabsf(f.pulse-start)<.0001f);
    int stone=ItemsAdd(IT_STONE,1,24,10);
    int t=0; do { ItemsStep(); CityStep(); CityFaceView(&f); t++; } while(!f.responses && t<180);
    assert(f.responses==1 && sfxCount[SFX_CITY_HUM]==1 && f.acknowledgmentFrames==120);
    assert(items[stone].vy>0 && !items[stone].onGround && TileWater(TileAtPx(items[stone].x+2,items[stone].y+2)));
    printf("FACE firstAnswerFrame=%d stoneY=%.3f stoneVy=%.3f remaining=%d\n",t,items[stone].y,items[stone].vy,f.acknowledgmentFrames);
    for(t=0;t<119;t++) { ItemsStep(); CityStep(); }
    CityFaceView(&f); assert(f.acknowledgmentFrames==1 && f.acknowledgment>0);
    ItemsStep(); CityStep(); CityFaceView(&f);
    assert(f.acknowledgmentFrames==0 && f.acknowledgment==0);
    for(t=0;t<250;t++) { ItemsStep(); CityStep(); }
    CityFaceView(&f); assert(f.responses==1 && items[stone].onGround);
    // Resting and held stones receive no repeated note; release is a new offer.
    RoomEnter(1); Steps(60); CityFaceView(&f); assert(f.responses==0);
    heldItem=stone; items[stone].onGround=0; items[stone].vy=.7f; Steps(10);
    CityFaceView(&f); assert(f.responses==0);
    heldItem=-1; items[stone].y=18*TS; items[stone].vy=.5f; CityStep();
    CityFaceView(&f); assert(f.responses==1);
    puts("PASS face: exact 7s pulse, actual sinking-stone note, 2s answer, no resting/held retrigger");
}
static void Shoal(void) {
    Enter(1); float initial=FishMeanY();
    // Lamp floats under the real item physics; fish approach its underside.
    items[0].room=1; items[0].x=22*TS; items[0].y=7*TS-3;
    for(int t=0;t<900;t++) { ItemsStep(); CityStep(); CheckWaterFish(); }
    float gathered=FishMeanY(); assert(gathered<initial-9);
    printf("SHOAL initialMeanY=%.3f floatingLampMeanY=%.3f lampY=%.3f\n",initial,gathered,items[0].y);
    CityFish f[CITY_FISH_COUNT]; CityFishViews(f,CITY_FISH_COUNT);
    for(int i=0;i<CITY_FISH_COUNT;i++) assert(f[i].gathering);
    // The same lamp held by a swimmer does not act as a floating beacon.
    heldItem=0; CityStep(); CityFishViews(f,CITY_FISH_COUNT);
    for(int i=0;i<CITY_FISH_COUNT;i++) assert(!f[i].gathering);
    heldItem=-1;
    for(int pocket=0;pocket<2;pocket++) {
        items[0].x=(pocket?37:1)*TS; items[0].y=7*TS-3; items[0].onGround=0;
        CityStep(); CityFishViews(f,CITY_FISH_COUNT);
        for(int i=0;i<CITY_FISH_COUNT;i++) assert(!f[i].gathering);
    }
    items[0].room=0;
    Steps(1000); CityFishViews(f,CITY_FISH_COUNT);
    float x=f[0].x,y=f[0].y;
    player.x=x-1-player.w*.5f; player.y=y-player.h*.5f;
    CityStep(); CityFishViews(f,CITY_FISH_COUNT); assert(f[0].scatter>.9f);
    Steps(25); CityFishViews(f,CITY_FISH_COUNT);
    assert(hypotf(f[0].x-(player.x+player.w*.5f),f[0].y-(player.y+player.h*.5f))>2);
    for(int t=0;t<6000;t++) { CityStep(); CheckWaterFish(); }
    puts("PASS shoal: eight water-constrained fish, actual floating-lamp gathering, held/disconnected-lamp exclusion, body scatter, 6000-frame bounds");
}
static void Purity(void) {
    Enter(1); Steps(50);
    CityWindow w[3],w2[3]; CityFish f[8],f2[8]; CityFace face,face2;
    memset(w,0,sizeof w); memset(w2,0,sizeof w2);
    CityWindowViews(w,3); CityFishViews(f,8); CityFaceView(&face);
    Player before=player; Item beforeItems[ITEM_MAX]; memcpy(beforeItems,items,sizeof items);
    u8 beforeTiles[RH][RW]; memcpy(beforeTiles,tiles,sizeof tiles);
    int beforeSounds[SFX_COUNT]; memcpy(beforeSounds,sfxCount,sizeof sfxCount);
    for(int i=0;i<100;i++) { CityWindowViews(w2,3); CityFishViews(f2,8); CityFaceView(&face2); }
    assert(!memcmp(w,w2,sizeof w) && !memcmp(f,f2,sizeof f) && !memcmp(&face,&face2,sizeof face));
    memset(w2,0,sizeof w2); CityWindowViews(w2,3); assert(!memcmp(w,w2,sizeof w));
    assert(CityWindowViews(NULL,3)==0 && CityWindowViews(w2,-1)==0 && CityFishViews(NULL,8)==0);
    assert(CityFishViews(f2,0)==0 && CityFaceView(NULL)==0 && CityMuralView(NULL)==0);
    CityWindow windowCanary=w2[1]; assert(CityWindowViews(w2,1)==1); assert(!memcmp(&windowCanary,&w2[1],sizeof windowCanary));
    CityFish canary=f2[1]; assert(CityFishViews(f2,1)==1); assert(!memcmp(&canary,&f2[1],sizeof canary));
    // State updates themselves cannot write into the existing body/items/tiles.
    Steps(100);
    assert(!memcmp(&before,&player,sizeof before) && !memcmp(beforeItems,items,sizeof items));
    assert(!memcmp(beforeTiles,tiles,sizeof tiles));
    for(int i=0;i<SFX_CITY_MURMUR;i++) assert(beforeSounds[i]==sfxCount[i]);
    puts("PASS ownership: detached bounded snapshots; city updates preserve body, items, geometry and original sound counts");
}
int main(void) {
    AudioInit(1);
    Windows(); Mural(); Face(); Shoal(); Purity();
    puts("PASS authored build-2 responses (native and human listening acceptance remain separate)");
    return 0;
}
