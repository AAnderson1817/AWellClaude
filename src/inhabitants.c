// The camp's inhabitant answers Hold. Stones stay in the real item array,
// remain available to the original pickup rule, and retain their reset homes.
#include "inhabitants.h"
#include <math.h>
#include <string.h>

#define CAIRN_INITIAL 4
#define BODY_W 6.0f
#define BODY_H 11.0f
#define STONE_W 5.0f
#define STONE_H 4.0f
#define STACK_STEP 2.2f
#define CAMP_RADIUS (4*TS)

static int initialized, seedIds[CAIRN_INITIAL], cairn[ITEM_MAX], cairnCount;
static int carried=-1, target=-1, watched=-1, state, phase, facing;
static int stable[ITEM_MAX], stolen[ITEM_MAX], wasNear, wasBed, wasFire;
static int tendWait, humWait, watchWait, voiceFrames, voiceKind, voiceGap;
static int placed, tended, taken, voicePending;
static unsigned voiceRequests, voiceSequence;
static float campX, floorY, fireX, coldHome, warmHome, cx, walkPhase;
static float stoneFromX, stoneFromY, lookX, lookY, handX, handY, stoneTurn;
static unsigned rng;
static HunterVoiceEvent voice;

static unsigned Rnd(void) { rng^=rng<<13; rng^=rng>>17; rng^=rng<<5; return rng; }
static int Wait(int low,int high) { return low+(int)(Rnd()%(unsigned)(high-low+1)); }
static int Stone(int i) { return i>=0 && i<itemCount && items[i].kind==IT_STONE; }
static int Slot(int i) { for(int k=0;k<cairnCount;k++)if(cairn[k]==i)return k; return -1; }
static float Home(void) { return wasFire?warmHome:coldHome; }
static int Busy(void) { return state==HUNTER_APPROACH||state==HUNTER_CARRY||state==HUNTER_PLACE||state==HUNTER_TEND; }
static void Request(int kind) { voiceRequests|=1u<<kind; }

static void CairnPose(void) {
    for(int k=0;k<cairnCount;k++) {
        int i=cairn[k];
        if(!Stone(i)||i==heldItem||items[i].room!=0)continue;
        items[i].x=campX-STONE_W*.5f;
        items[i].y=floorY-STONE_H-k*STACK_STEP;
        items[i].vy=0; items[i].onGround=1;
    }
}
static int Remove(int slot) {
    int item=cairn[slot];
    for(int k=slot;k<cairnCount-1;k++)cairn[k]=cairn[k+1];
    cairnCount--; return item;
}
static int SeedsValid(void) {
    if(!initialized)return 0;
    for(int k=0;k<CAIRN_INITIAL;k++) {
        int i=seedIds[k];
        if(!Stone(i)||items[i].hroom!=0||fabsf(items[i].hx-(campX-STONE_W*.5f))>.001f||
           fabsf(items[i].hy-(floorY-STONE_H-(CAIRN_INITIAL-1-k)*STACK_STEP))>.001f)return 0;
    }
    return 1;
}

void InhabitantsReset(void) {
    if(!SeedsValid())return;
    rng=0x71A4C39Du; carried=target=watched=-1; cairnCount=CAIRN_INITIAL;
    for(int k=0;k<CAIRN_INITIAL;k++)cairn[k]=seedIds[CAIRN_INITIAL-1-k];
    memset(stable,0,sizeof stable); memset(stolen,0,sizeof stolen);
    state=HUNTER_SIT; phase=0; facing=1; cx=coldHome; walkPhase=0;
    wasNear=wasBed=wasFire=0; watchWait=0; tendWait=Wait(1200,2400); humWait=Wait(1500,2700);
    voiceFrames=voiceKind=voiceGap=voicePending=0; voiceRequests=voiceSequence=0;
    placed=tended=taken=0; stoneTurn=0;
    handX=cx+4;handY=floorY-5;lookX=campX;lookY=floorY-5;
    memset(&voice,0,sizeof voice); CairnPose();
}

int InhabitantsInit(void) {
    if(SeedsValid())return 1;
    initialized=0;
    if(roomIdx!=0||itemCount>ITEM_MAX-CAIRN_INITIAL)return 0;
    PropView props[96]; int n=PropsViews(props,96),found=0;
    for(int k=0;k<n;k++) {
        if(props[k].kind==PR_CAIRN) {campX=props[k].tx*TS+TS*.5f;floorY=(props[k].ty+1)*TS;found|=1;}
        if(props[k].kind==PR_FIRE) {fireX=props[k].tx*TS+TS*.5f;found|=2;}
    }
    if(found!=3)return 0;
    coldHome=campX-8;warmHome=fireX-6;
    // Top first gives the original stable nearest-item tie break a natural top
    // stone. Every stone can still be recovered, including an interrupted offer.
    for(int k=0;k<CAIRN_INITIAL;k++) {
        int i=ItemsAdd(IT_STONE,0,(int)(campX/TS),(int)(floorY/TS)-1);
        seedIds[k]=i;
        items[i].x=items[i].hx=campX-STONE_W*.5f;
        items[i].y=items[i].hy=floorY-STONE_H-(CAIRN_INITIAL-1-k)*STACK_STEP;
    }
    initialized=1;InhabitantsReset();return 1;
}

int InhabitantsPinsItem(int i) {
    return initialized&&Stone(i)&&i!=heldItem&&items[i].room==0&&(Slot(i)>=0||carried==i);
}

static int Standable(float x) {
    if(fabsf(x-campX)>CAMP_RADIUS)return 0;
    int x0=(int)floorf((x-BODY_W*.5f)/TS),x1=(int)floorf((x+BODY_W*.5f-.01f)/TS);
    int y0=(int)floorf((floorY-BODY_H)/TS),y1=(int)floorf((floorY-.01f)/TS);
    for(int ty=y0;ty<=y1;ty++)for(int tx=x0;tx<=x1;tx++)if(TileSolid(TileGet(tx,ty)))return 0;
    for(int tx=x0;tx<=x1;tx++)if(!TileSolid(TileGet(tx,(int)(floorY/TS)))&&!TileOneWay(TileGet(tx,(int)(floorY/TS))))return 0;
    return 1;
}
static int Path(float destination) {
    float distance=destination-cx;
    int samples=(int)ceilf(fabsf(distance))+1;
    for(int k=1;k<=samples;k++)if(!Standable(cx+distance*k/samples))return 0;
    return 1;
}
static int Walk(float destination) {
    float d=destination-cx;
    if(fabsf(d)<.2f)return 1;
    facing=d>0?1:-1;
    float step=fminf(fabsf(d),carried>=0?.19f:.24f)*facing,next=cx+step;
    // The hunter never pushes, blocks or pursues the player. Stop an approach
    // before their body; the player's movement and collision remain untouched.
    int overlapY=player.y+player.h>floorY-BODY_H&&player.y<floorY;
    int nextOverlap=next+BODY_W*.5f+1>player.x&&next-BODY_W*.5f-1<player.x+player.w;
    int oldOverlap=cx+BODY_W*.5f+1>player.x&&cx-BODY_W*.5f-1<player.x+player.w;
    if(!Standable(next)||(overlapY&&nextOverlap&&!oldOverlap))return 0;
    if(overlapY&&oldOverlap&&fabsf(next-(player.x+player.w*.5f))<fabsf(cx-(player.x+player.w*.5f)))return 0;
    cx=next;walkPhase+=fabsf(step)*.23f;return fabsf(destination-cx)<.2f;
}
static int Offer(int i) {
    if(!Stone(i)||i==heldItem||items[i].room!=0||Slot(i)>=0||i==carried)return 0;
    if(!items[i].onGround||items[i].cool||fabsf(items[i].y+STONE_H-floorY)>1.1f)return 0;
    float x=items[i].x+STONE_W*.5f;
    float range=stolen[i]?CAMP_RADIUS:2*TS;
    if(fabsf(x-(stolen[i]?campX:cx))>range||!Standable(x)||!Path(x))return 0;
    return 1;
}
static int FindOffer(void) {
    int best=-1;float distance=1e9f;
    for(int i=0;i<itemCount;i++)if(Offer(i)&&stable[i]>=6) {
        float d=fabsf(items[i].x+STONE_W*.5f-cx);
        if(d<distance){distance=d;best=i;}
    }
    return best;
}
static void Missing(int i) {
    // A second stone can be taken while one is in these hands. The startled
    // hunter releases the other physical stone instead of losing its ownership
    // under the new attention state. Ordinary falling/pickup resumes next tick.
    if(carried>=0&&carried!=i) {
        if(Stone(carried)&&carried!=heldItem&&items[carried].room==0) {
            items[carried].vy=0;items[carried].onGround=0;
            stable[carried]=0;stolen[carried]=1;
        }
        carried=-1;
    }
    if(Stone(i))stolen[i]=1;
    watched=i;target=-1;state=HUNTER_TAKEN;phase=0;taken++;Request(HVOICE_TAKEN);
}
static void Reconcile(void) {
    for(int k=cairnCount-1;k>=0;k--) {
        int i=cairn[k];
        if(!Stone(i)||heldItem==i||items[i].room!=0) {Remove(k);Missing(i);}
    }
    if(carried>=0&&(!Stone(carried)||heldItem==carried||items[carried].room!=0)) {
        int i=carried;carried=-1;Missing(i);
    }
}
static void Place(void) {
    if(carried>=0&&Stone(carried)&&carried!=heldItem&&items[carried].room==0) {
        cairn[cairnCount++]=carried;stolen[carried]=0;carried=-1;CairnPose();
    }
    target=watched=-1;phase=0;state=HUNTER_RETURN;
}
static void VoiceStep(void) {
    if(voiceFrames>0)voiceFrames--;
    if(voiceGap>0)voiceGap--;
    if(voiceFrames||voiceGap||!voiceRequests)return;
    static const int priority[]={HVOICE_TAKEN,HVOICE_FIRE,HVOICE_OFFER,HVOICE_BEDROLL,HVOICE_GREETING,HVOICE_HUM};
    for(unsigned k=0;k<sizeof priority/sizeof priority[0];k++) {
        int kind=priority[k];if(!(voiceRequests&(1u<<kind)))continue;
        voiceRequests&=~(1u<<kind);voiceKind=kind;
        int syllables=kind==HVOICE_OFFER||kind==HVOICE_FIRE?3:1;
        float pitch=kind==HVOICE_FIRE?.72f:(kind==HVOICE_TAKEN?1.18f:(kind==HVOICE_HUM?.83f:1.0f));
        voice=(HunterVoiceEvent){kind,syllables,pitch+(Wait(-12,12)*.001f),kind==HVOICE_HUM?.12f:.20f,cx/GW,++voiceSequence};
        voiceFrames=syllables*18;voiceGap=voiceFrames+24;voicePending=1;break;
    }
}

void InhabitantsStep(void) {
    if(!initialized)return;
    Reconcile();
    if(roomIdx!=0) {voicePending=0;voiceRequests=0;voiceFrames=voiceGap=0;wasNear=wasBed=0;return;}
    CairnPose();phase++;stoneTurn=0;
    for(int i=0;i<itemCount;i++)stable[i]=Offer(i)?(stable[i]<12?stable[i]+1:12):0;
    PropView props[96];int n=PropsViews(props,96),bed=0;
    for(int k=0;k<n;k++)if(props[k].kind==PR_BEDROLL)bed|=props[k].state!=0;
    int fire=PropFireLit(0),near=hypotf(player.x+player.w*.5f-cx,player.y+player.h*.5f-(floorY-6))<=5*TS;
    if(near&&!wasNear){watchWait=120;Request(HVOICE_GREETING);}
    if(!near)voiceRequests&=~(1u<<HVOICE_GREETING);
    if(bed&&!wasBed)Request(HVOICE_BEDROLL);
    if(fire&&!wasFire){Request(HVOICE_FIRE);humWait=Wait(720,1320);if(!Busy()&&state!=HUNTER_TAKEN)state=HUNTER_RETURN;}
    wasNear=near;wasBed=bed;wasFire=fire;
    if(tendWait>0)tendWait--;
    if(humWait>0)humWait--;
    if(watchWait>0)watchWait--;
    lookX=campX;lookY=floorY-5;
    if(near&&watchWait>0){lookX=player.x+player.w*.5f;lookY=player.y+3;}

    if(state==HUNTER_TAKEN) {
        if(Stone(watched)&&items[watched].room==0&&fabsf(items[watched].x+STONE_W*.5f-campX)<=CAMP_RADIUS) {
            lookX=items[watched].x+STONE_W*.5f;lookY=items[watched].y+2;
        } else {watched=-1;state=HUNTER_RETURN;}
    }
    if(!Busy()) {
        int offer=FindOffer();
        if(offer>=0){target=offer;state=HUNTER_APPROACH;phase=0;}
        else if(bed&&state!=HUNTER_TAKEN)state=HUNTER_BEDROLL;
        else if(state==HUNTER_BEDROLL)state=HUNTER_RETURN;
    }
    switch(state) {
    case HUNTER_APPROACH:
        if(!Offer(target)){target=-1;state=HUNTER_RETURN;break;}
        lookX=items[target].x+STONE_W*.5f;lookY=items[target].y+2;
        if(fabsf(lookX-cx)<=6.0f) {
            carried=target;items[carried].vy=0;items[carried].onGround=0;state=HUNTER_CARRY;phase=0;
        } else Walk(lookX+(lookX>cx?-5.5f:5.5f));
        break;
    case HUNTER_CARRY:
        if(Walk(campX-6)) {state=HUNTER_PLACE;phase=0;stoneFromX=items[carried].x;stoneFromY=items[carried].y;}
        break;
    case HUNTER_PLACE:
        if(phase>=36){Place();placed++;Request(HVOICE_OFFER);}
        break;
    case HUNTER_TEND:
        if(phase>=96){Place();tended++;}
        break;
    case HUNTER_RETURN:
        if(Walk(Home()))state=wasFire?HUNTER_WARM:HUNTER_SIT;
        break;
    case HUNTER_SIT: case HUNTER_WARM: case HUNTER_WATCH:
        if(fabsf(cx-Home())>.3f){state=HUNTER_RETURN;break;}
        state=wasFire?HUNTER_WARM:(watchWait?HUNTER_WATCH:HUNTER_SIT);
        if(tendWait==0&&cairnCount) {
            carried=Remove(cairnCount-1);stoneFromX=items[carried].x;stoneFromY=items[carried].y;
            state=HUNTER_TEND;phase=0;tendWait=Wait(1200,2400);
        }
        if(humWait==0){Request(HVOICE_HUM);humWait=wasFire?Wait(720,1320):Wait(1500,2700);}
        break;
    default:break;
    }
    if(carried>=0) {
        Item *it=&items[carried];it->vy=0;it->onGround=0;
        if(state==HUNTER_TEND) {
            float t=phase/96.f,lift=sinf(t*3.14159265f);
            it->x=stoneFromX+sinf(t*6.2831853f)*.8f;it->y=stoneFromY-lift*4;
            stoneTurn=lift*.8f;
        } else if(state==HUNTER_PLACE) {
            float t=phase/36.f;t=t*t*(3-2*t);
            it->x=stoneFromX+(campX-STONE_W*.5f-stoneFromX)*t;
            it->y=stoneFromY+(floorY-STONE_H-cairnCount*STACK_STEP-stoneFromY)*t;
        } else {it->x=cx+facing*4-STONE_W*.5f;it->y=floorY-8;}
        handX=it->x+STONE_W*.5f;handY=it->y+STONE_H*.5f;lookX=handX;lookY=handY;
    } else {handX=cx+facing*4;handY=floorY-(state==HUNTER_WARM?7:4);}
    if(state==HUNTER_WARM){lookX=fireX;lookY=floorY-3;}
    if(state==HUNTER_BEDROLL){lookX=player.x+player.w*.5f;lookY=player.y+4;}
    VoiceStep();
}

int InhabitantsCairnItems(int *out,int max) {
    if(!out||max<=0||!initialized)return 0;
    int n=max<cairnCount?max:cairnCount;for(int k=0;k<n;k++)out[k]=cairn[k];return n;
}
int InhabitantsHunterView(HunterView *out) {
    if(!out||!initialized||roomIdx!=0)return 0;
    *out=(HunterView){cx-BODY_W*.5f,floorY-BODY_H,BODY_W,BODY_H,handX,handY,lookX,lookY,walkPhase,stoneTurn,
        state,facing,carried,cairnCount,wasFire,voiceFrames>0&&(voiceFrames%18)<10,voiceKind,voiceFrames,tendWait,placed,tended,taken};
    return 1;
}
int InhabitantsPollVoice(HunterVoiceEvent *out) {
    if(!out||!voicePending||roomIdx!=0)return 0;
    *out=voice;voicePending=0;return 1;
}
