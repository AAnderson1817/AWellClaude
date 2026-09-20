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
static int tendWait, humWait, watchWait, voiceFrames, voiceKind, voiceGap, voiceTotalFrames;
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
static int Busy(void) { return state==HUNTER_APPROACH||state==HUNTER_CARRY||state==HUNTER_PLACE||state==HUNTER_TEND||state==HUNTER_TEND_APPROACH; }
static void Request(int kind) { voiceRequests|=1u<<kind; }
static float Smooth(float t) { t=fminf(1,fmaxf(0,t));return t*t*(3-2*t); }

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
    voiceFrames=voiceKind=voiceGap=voicePending=voiceTotalFrames=0; voiceRequests=voiceSequence=0;
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
    // Incidental requests are valid only in their current physical context.
    // Crossing the bedroll during a transfer must not narrate it afterward.
    if(!wasNear||watchWait==0||Busy()||state==HUNTER_TAKEN||state==HUNTER_BEDROLL)
        voiceRequests&=~(1u<<HVOICE_GREETING);
    if(!wasBed||state!=HUNTER_BEDROLL)voiceRequests&=~(1u<<HVOICE_BEDROLL);
    if(state!=HUNTER_SIT&&state!=HUNTER_WARM)voiceRequests&=~(1u<<HVOICE_HUM);
    if(!wasFire)voiceRequests&=~(1u<<HVOICE_FIRE);

    int kind=HVOICE_NONE;
    // Actual stone actions answer on this tick, even during another utterance
    // or its quiet gap. AudioHunterPlay stops the previous single speaker.
    if(voiceRequests&(1u<<HVOICE_TAKEN))kind=HVOICE_TAKEN;
    else if(voiceRequests&(1u<<HVOICE_OFFER))kind=HVOICE_OFFER;
    else if((voiceRequests&(1u<<HVOICE_FIRE))&&!Busy()&&state!=HUNTER_TAKEN&&
            (!voiceFrames||(voiceKind!=HVOICE_TAKEN&&voiceKind!=HVOICE_OFFER)))kind=HVOICE_FIRE;
    if(kind!=HVOICE_NONE) {
        // Only the still-lit fire can retain context across a stone action.
        // Older greeting/bedroll/hum requests never follow as a speech backlog.
        voiceRequests&=kind==HVOICE_FIRE?0u:(1u<<HVOICE_FIRE);
    } else {
        if(voiceFrames||voiceGap||!voiceRequests)return;
        static const int social[]={HVOICE_BEDROLL,HVOICE_GREETING,HVOICE_HUM};
        for(unsigned k=0;k<sizeof social/sizeof social[0];k++)
            if(voiceRequests&(1u<<social[k])){kind=social[k];break;}
        if(kind==HVOICE_NONE)return;
        voiceRequests&=~(1u<<kind);
    }
    voiceKind=kind;
    int syllables=AudioHunterPhrase(kind)->syllables;
    float pitch=kind==HVOICE_FIRE?.72f:(kind==HVOICE_TAKEN?1.18f:(kind==HVOICE_HUM?.83f:1.0f));
    voice=(HunterVoiceEvent){kind,syllables,pitch+(Wait(-12,12)*.001f),kind==HVOICE_HUM?.12f:.20f,cx/GW,++voiceSequence};
    voiceFrames=voiceTotalFrames=AudioHunterDurationTicks(kind,voice.pitch);
    voiceGap=voiceFrames+24;voicePending=1;
}

void InhabitantsStep(void) {
    if(!initialized)return;
    Reconcile();
    if(roomIdx!=0) {voicePending=0;voiceRequests=0;voiceFrames=voiceGap=voiceTotalFrames=0;wasNear=wasBed=0;return;}
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
        if(Walk(campX-5.5f)) {facing=1;state=HUNTER_PLACE;phase=0;stoneFromX=items[carried].x;stoneFromY=items[carried].y;}
        break;
    case HUNTER_PLACE:
        if(phase>=36){Place();placed++;Request(HVOICE_OFFER);}
        break;
    case HUNTER_TEND_APPROACH:
        // Leave the top in the real, freely pickupable cairn until the body is
        // in reach. Walking may yield to the player for as long as necessary.
        if(!cairnCount||!Path(campX-5.5f)){state=HUNTER_RETURN;break;}
        lookX=campX;lookY=items[cairn[cairnCount-1]].y+STONE_H*.5f;
        if(Walk(campX-5.5f)) {
            carried=Remove(cairnCount-1);stoneFromX=items[carried].x;stoneFromY=items[carried].y;
            facing=1;state=HUNTER_TEND;phase=0;
        }
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
            state=HUNTER_TEND_APPROACH;phase=0;tendWait=Wait(1200,2400);
        }
        if(humWait==0){Request(HVOICE_HUM);humWait=wasFire?Wait(720,1320):Wait(1500,2700);}
        break;
    default:break;
    }
    if(carried>=0) {
        Item *it=&items[carried];it->vy=0;it->onGround=0;
        if(state==HUNTER_TEND) {
            // Unseat, clear the remaining pile horizontally, then inspect at
            // belly height. Retrace exactly; fixed 4.2+4.2px arms can reach even
            // the sixth stone without stretching or moving the body off-ground.
            float q=fminf((float)phase,96.f-phase),top=stoneFromY+STONE_H*.5f;
            float x=stoneFromX+STONE_W*.5f,y=top;
            if(q<=10)y=top-.3f*Smooth(q/10);
            else if(q<=28){x+=(campX-5.2f-x)*Smooth((q-10)/18);y=top-.3f;}
            else {
                x=campX-5.2f;float t=Smooth((q-28)/14);
                y=top-.3f+(floorY-5.5f-(top-.3f))*t;
                stoneTurn=.8f*sinf(fminf(1,(q-28)/14)*1.570796327f);
            }
            it->x=x-STONE_W*.5f;it->y=y-STONE_H*.5f;
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
    int elapsed=voiceTotalFrames-voiceFrames;
    HunterMouth mouth=AudioHunterMouthAt(voiceKind,voice.pitch,elapsed);
    *out=(HunterView){cx-BODY_W*.5f,floorY-BODY_H,BODY_W,BODY_H,handX,handY,lookX,lookY,walkPhase,stoneTurn,
        state,facing,carried,cairnCount,wasFire,mouth.open>.22f,voiceKind,voiceFrames,tendWait,placed,tended,taken,
        mouth.open,elapsed,voiceTotalFrames,mouth.syllable};
    return 1;
}
int InhabitantsPollVoice(HunterVoiceEvent *out) {
    if(!out||!voicePending||roomIdx!=0)return 0;
    *out=voice;voicePending=0;return 1;
}
