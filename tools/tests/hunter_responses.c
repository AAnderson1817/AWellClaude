// Real item pickup/drop/fall and prop responses, with only platform I/O stubbed.
#define main game_main
#include "../../src/main.c"
#undef main
#include "../../src/inhabitants.h"
#include <assert.h>
#include <stdint.h>

static Item homes[ITEM_MAX];
static int voices[HVOICE_HUM+1], checks;
static HunterView View(void) { HunterView h;assert(InhabitantsHunterView(&h));return h; }
static void At(float x,float y) {PlayerInit(x,y);player.onGround=1;player.facing=1;}
static void Conserved(void) {
    assert(itemCount==7);
    int ids[ITEM_MAX],n=InhabitantsCairnItems(ids,ITEM_MAX),seen[ITEM_MAX]={0};
    for(int k=0;k<n;k++){assert(ids[k]>=0&&ids[k]<itemCount);assert(!seen[ids[k]]++);assert(ids[k]!=heldItem);}
    HunterView h;
    if(InhabitantsHunterView(&h)) {
        assert(h.cairnCount==n && fabsf(h.x+h.w*.5f-124)<=32);
        if(h.carriedItem>=0){assert(h.carriedItem<itemCount);assert(!seen[h.carriedItem]++);assert(h.carriedItem!=heldItem);}
    }
    for(int i=0;i<itemCount;i++) {
        assert(items[i].kind==homes[i].kind && items[i].hroom==homes[i].hroom);
        assert(items[i].hx==homes[i].hx && items[i].hy==homes[i].hy);
        assert(isfinite(items[i].x)&&isfinite(items[i].y)&&isfinite(items[i].vy));
    }
    assert(!InhabitantsPinsItem(heldItem));checks++;
}
static void Poll(void) {
    HunterVoiceEvent v;
    if(InhabitantsPollVoice(&v)) {
        assert(v.kind>HVOICE_NONE&&v.kind<=HVOICE_HUM);assert(v.syllables>=1&&v.syllables<=3);
        assert(v.volume>0&&v.volume<=.2f&&v.pan>=0&&v.pan<=1);
        voices[v.kind]++;assert(!InhabitantsPollVoice(&v));
    }
}
static void Tick(void) {frameNo++;ItemsStep();PropsStep();InhabitantsStep();in.actPressed=0;Poll();Conserved();}
static void Steps(int n) {while(n-->0)Tick();}
static void Enter(void) {
    ItemsReset();ItemsAdd(IT_LAMP,0,1,14);RoomLoad();PropsReset();RoomEnter(0);At(30,100);
    memset(&in,0,sizeof in);memset(voices,0,sizeof voices);
    assert(itemCount==3);Item old[3];memcpy(old,items,sizeof old);
    assert(InhabitantsInit());assert(itemCount==7);assert(!memcmp(old,items,sizeof old));
    memcpy(homes,items,sizeof homes);Conserved();
}
static void Act(void) {in.actPressed=1;Tick();}
static void TakeLegacy(void) {
    At(items[1].x,items[1].y-7);Act();assert(heldItem==1);assert(player.heavy);
}
static void DropLegacy(float px) {At(px,149);Steps(9);Act();assert(heldItem==-1);At(60,149);}
static void OfferLegacy(void) {TakeLegacy();DropLegacy(98);}
static void WaitPlaced(int expected) {
    int t=0;while(View().placed<expected&&t++<700)Tick();
    assert(View().placed==expected);assert(InhabitantsPinsItem(1));
}
static void Creation(void) {
    Enter();Item before[ITEM_MAX];memcpy(before,items,sizeof before);
    assert(InhabitantsInit());assert(itemCount==7&&!memcmp(before,items,sizeof before));
    assert(View().cairnCount==4);
    for(int i=3;i<7;i++)assert(InhabitantsPinsItem(i));
    assert(!InhabitantsPinsItem(-1)&&!InhabitantsPinsItem(0)&&!InhabitantsPinsItem(ITEM_MAX));
    // A fresh world with insufficient slots is left byte-for-byte untouched.
    ItemsReset();ItemsAdd(IT_LAMP,0,1,14);RoomLoad();
    while(itemCount<5)ItemsAdd(IT_STONE,0,3,3);
    memcpy(before,items,sizeof before);assert(!InhabitantsInit());
    assert(itemCount==5&&!memcmp(before,items,sizeof before));
    puts("PASS initialization: 4 real cairn stones, original IDs/homes intact, 7/8 budget, idempotence and atomic capacity failure");
}
static void OfferTakeReturn(void) {
    Enter();OfferLegacy();WaitPlaced(1);assert(View().cairnCount==5);assert(voices[HVOICE_OFFER]==1);
    At(121,149);Act();assert(heldItem==1);assert(!InhabitantsPinsItem(1));
    assert(View().cairnCount==4&&View().state==HUNTER_TAKEN);
    // This return is beyond the ordinary 2-tile offer radius but within 4 of camp.
    DropLegacy(89);assert(fabsf(items[1].x+2.5f-124)>16);WaitPlaced(2);
    At(121,149);Act();assert(heldItem==1);DropLegacy(55);float x=items[1].x;
    Steps(600);assert(View().cairnCount==4&&View().placed==2);
    assert(items[1].x==x&&!InhabitantsPinsItem(1));assert(View().state!=HUNTER_TAKEN);
    assert(voices[HVOICE_TAKEN]==2);
    puts("PASS offer/take/return: original Hold transfers the same real stone, 4-tile reclaim works, distant stone remains recoverable");
}
static void Interruption(void) {
    Enter();OfferLegacy();int t=0;
    while(View().carriedItem!=1&&t++<300)Tick();assert(View().carriedItem==1);
    // Same input path as every other pickup; carried items have no hidden lock.
    At(items[1].x+.2f,149);Act();assert(heldItem==1);
    assert(View().carriedItem==-1&&!InhabitantsPinsItem(1)&&View().cairnCount==4);
    float x=items[1].x,y=items[1].y;InhabitantsStep();assert(items[1].x==x&&items[1].y==y);
    At(60,149);Steps(30);assert(heldItem==1&&View().placed==0);
    // A just-dropped stone cannot be seized during the original drop cooldown.
    Enter();OfferLegacy();for(int k=0;k<6;k++){Tick();assert(View().carriedItem==-1);}
    At(items[1].x,149);Steps(2);Act();assert(heldItem==1);Steps(30);assert(View().placed==0);
    // Taking a different cairn member must not strand the stone already carried.
    Enter();OfferLegacy();t=0;while(View().carriedItem!=1&&t++<300)Tick();assert(View().carriedItem==1);
    x=items[1].x;y=items[1].y;At(121,149);Act();assert(heldItem>=3&&heldItem<7);
    assert(View().carriedItem==-1&&!InhabitantsPinsItem(1)&&items[1].x==x&&items[1].y==y);
    At(60,149);Steps(1);assert(items[1].y>y);Steps(300);assert(View().placed==1&&InhabitantsPinsItem(1));
    // The same competing pickup while tending drops the lifted top stone visibly.
    Enter();t=0;while(View().state!=HUNTER_TEND&&t++<2401)Tick();assert(View().state==HUNTER_TEND);
    int top=View().carriedItem;Steps(24);x=items[top].x;y=items[top].y;At(121,149);Act();
    assert(heldItem>=3&&heldItem<7&&heldItem!=top);
    assert(View().carriedItem==-1&&!InhabitantsPinsItem(top)&&items[top].x==x&&items[top].y==y);
    At(60,149);Steps(1);assert(items[top].y>y);
    puts("PASS interruptions: pickup instantly wins over carrying/approach; cooldown honored, no held-item seizure");
    puts("PASS competing pickup: taking another member releases the hunter's carried/tended stone to real falling, with no stranded ownership");
}
static void IdleAndSocial(void) {
    Enter();int t=0;while(View().state!=HUNTER_TEND&&t++<2401)Tick();
    assert(t>=1200&&t<=2400);int stone=View().carriedItem;assert(stone>=3&&stone<7);
    float startY=items[stone].y;Steps(48);assert(items[stone].y<startY-3.5f);
    assert(View().stoneTurn>.7f&&View().cairnCount==3);Steps(48);
    assert(View().tended==1&&View().cairnCount==4&&InhabitantsPinsItem(stone));
    printf("TEND first=%d frames, actual item=%d; lift/turn/return=96 frames\n",t,stone);
    // Taking the top stone during the tending motion yields immediately too.
    t=0;while(View().state!=HUNTER_TEND&&t++<2401)Tick();assert(View().state==HUNTER_TEND);
    stone=View().carriedItem;At(121,149);Act();
    assert(heldItem==stone&&View().carriedItem==-1&&View().cairnCount==3);
    Enter();At(85,149);Steps(180);assert(voices[HVOICE_GREETING]==1);
    At(40,149);Steps(60);At(85,149);Steps(120);assert(voices[HVOICE_GREETING]==2);
    At(108,149);Steps(180);assert(View().state==HUNTER_BEDROLL&&voices[HVOICE_BEDROLL]==1);
    float hx=View().x;Steps(180);assert(View().x==hx&&voices[HVOICE_BEDROLL]==1);
    At(80,149);Steps(90);At(108,149);Steps(90);assert(voices[HVOICE_BEDROLL]==2);
    puts("PASS authored idle/social responses: 20-40s actual-stone tending, pickup during tending, greeting edges, bedroll waiting");
}
static void FireAndLamp(void) {
    Enter();items[0].x=130;items[0].y=155;items[0].vy=0;
    Steps(121);assert(PropFireLit(0));Steps(150);
    assert(View().fireLit&&View().state==HUNTER_WARM&&fabsf(View().x+3-126)<.3f);
    assert(voices[HVOICE_FIRE]==1&&View().carriedItem!=0);assert(!InhabitantsPinsItem(0));
    Steps(1320);assert(voices[HVOICE_HUM]>=1&&voices[HVOICE_FIRE]==1);
    // The lamp is never treated as an offering, including while the player holds it.
    At(items[0].x,149);Act();assert(heldItem==0);Steps(100);
    assert(heldItem==0&&View().carriedItem!=0&&View().cairnCount==4);
    puts("PASS persistent fire: real two-second lamp ignition, closer warm seat, one long phrase, quicker hum; lamp never appropriated");
}
static void PersistenceAndReset(void) {
    Enter();At(121,149);Act();int stone=heldItem;assert(stone>=3&&stone<7);
    RoomEnter(1);At(170,50);Steps(9);Act();assert(heldItem==-1&&items[stone].room==1);
    HunterView ignored;assert(!InhabitantsHunterView(&ignored));assert(!InhabitantsPollVoice(&(HunterVoiceEvent){0}));
    Steps(120);float y=items[stone].y;assert(y>50);
    RoomEnter(0);At(30,100);Steps(200);
    assert(View().cairnCount==3&&items[stone].room==1&&items[stone].y==y);
    assert(InhabitantsInit()&&itemCount==7&&View().cairnCount==3);
    ItemsHome();PropsReset();RoomEnter(0);InhabitantsReset();Conserved();
    assert(heldItem==-1&&View().cairnCount==4&&View().tended==0&&View().taken==0);
    for(int i=0;i<itemCount;i++)assert(items[i].room==homes[i].hroom&&items[i].x==homes[i].hx&&fabsf(items[i].y-homes[i].hy)<.001f);
    puts("PASS persistence/reset: real stone travels and sinks in room 1, room re-entry never respawns it, full reset restores homes and four seed IDs");
}
static void Exhaustion(void) {
    Enter();OfferLegacy();WaitPlaced(1);
    // Arrange the second legacy stone at the camp for this interaction fixture;
    // its original room-1 home remains intact throughout the transfer and reset.
    items[2].room=0;items[2].x=107;items[2].y=156;items[2].vy=0;
    At(105,149);Act();assert(heldItem==2);At(98,149);Steps(9);Act();assert(heldItem==-1);At(60,149);
    int t=0;while(View().placed<2&&t++<700)Tick();assert(View().placed==2&&View().cairnCount==6);
    int recovered[ITEM_MAX]={0};
    for(int k=0;k<6;k++) {
        RoomEnter(0);At(121,149);Act();int stone=heldItem;
        assert(stone>0&&stone<7&&!recovered[stone]++);
        assert(View().cairnCount==5-k);
        RoomEnter(1);At(178+k*6,54);Steps(9);Act();assert(heldItem==-1);Steps(9);
        assert(items[stone].room==1&&!InhabitantsPinsItem(stone));
    }
    RoomEnter(0);At(30,100);Steps(2600);assert(View().cairnCount==0&&View().carriedItem==-1);
    for(int i=1;i<7;i++)assert(recovered[i]==1&&items[i].room==1);
    ItemsHome();PropsReset();RoomEnter(0);InhabitantsReset();Conserved();
    assert(View().cairnCount==4&&items[2].room==1&&items[1].room==0);
    puts("PASS full cairn/exhaustion: both legacy stones offered, all six picked up exactly once, no invisible reserves, reset restores only the four seed members");
}
static void PurityAndBounds(void) {
    Enter();HunterView h=View(),copy;Item before[ITEM_MAX];Player p=player;
    memcpy(before,items,sizeof before);int audio[SFX_COUNT];memcpy(audio,sfxCount,sizeof audio);
    struct {int ids[2];int canary;} out={{0},0x7453};
    for(int k=0;k<1000;k++) {
        assert(InhabitantsHunterView(&copy));assert(!memcmp(&h,&copy,sizeof h));
        copy.x=-999;assert(InhabitantsCairnItems(out.ids,2)==2);assert(out.canary==0x7453);
    }
    assert(!InhabitantsHunterView(NULL)&&!InhabitantsCairnItems(NULL,8)&&!InhabitantsCairnItems(out.ids,-1));
    assert(!memcmp(before,items,sizeof before)&&!memcmp(&p,&player,sizeof p)&&!memcmp(audio,sfxCount,sizeof audio));
    // The module alone must never modify inherited entities, input, map or Sfx state.
    u8 map[RH][RW];for(int y=0;y<RH;y++)for(int x=0;x<RW;x++)map[y][x]=TileGet(x,y);
    for(int k=0;k<5000;k++){InhabitantsStep();Poll();Conserved();}
    assert(!memcmp(before,items,3*sizeof(Item))&&!memcmp(&p,&player,sizeof p)&&!memcmp(audio,sfxCount,sizeof audio));
    for(int y=0;y<RH;y++)for(int x=0;x<RW;x++)assert(map[y][x]==TileGet(x,y));
    // An item on a different vertical support is not a camp-floor offering.
    Enter();items[1].x=116;items[1].y=108;items[1].onGround=1;
    for(int k=0;k<300;k++)InhabitantsStep();assert(View().carriedItem==-1&&View().placed==0);
    // Hunter motion stops at the player without moving them or adding collision.
    Enter();OfferLegacy();int t=0;while(View().carriedItem!=1&&t++<300)Tick();assert(View().carriedItem==1);At(111,149);p=player;
    Steps(100);assert(player.x==p.x&&player.y==p.y&&View().placed==0);
    At(60,149);WaitPlaced(1);
    puts("PASS detached views, bounded copies, inherited item/player/map/audio isolation, supported floor and player-space yielding");
}
static uint64_t Mix(uint64_t h,const void *p,size_t n) {
    const unsigned char *s=p;while(n--){h^=*s++;h*=UINT64_C(1099511628211);}return h;
}
static void TraceOriginal(int enabled) {
    ItemsReset();ItemsAdd(IT_LAMP,0,1,14);RoomLoad();PropsReset();RoomEnter(0);At(30,100);memset(&in,0,sizeof in);
    if(enabled)assert(InhabitantsInit());
    for(int k=0;k<6000;k++) {
        frameNo++;ItemsStep();PropsStep();if(enabled)InhabitantsStep();
        uint64_t h=UINT64_C(14695981039346656037);h=Mix(h,items,3*sizeof(Item));h=Mix(h,&player,sizeof player);
        h=Mix(h,sfxCount,sizeof sfxCount);PropView props[96];int n=PropsViews(props,96);
        for(int i=0;i<n;i++){h=Mix(h,&props[i].state,sizeof props[i].state);h=Mix(h,&props[i].timer,sizeof props[i].timer);}
        for(int y=0;y<RH;y++)for(int x=0;x<RW;x++){u8 t=TileGet(x,y);h=Mix(h,&t,1);}
        printf("%d %016llx\n",k,(unsigned long long)h);
    }
}
int main(int argc,char **argv) {
    AudioInit(1);
    if(argc==2){TraceOriginal(!strcmp(argv[1],"--enabled"));return 0;}
    Creation();OfferTakeReturn();Interruption();IdleAndSocial();FireAndLamp();PersistenceAndReset();Exhaustion();PurityAndBounds();
    printf("PASS conservation checked after %d steps; no renderer or audible-voice acceptance claimed\n",checks);return 0;
}
