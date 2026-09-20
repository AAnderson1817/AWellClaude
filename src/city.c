// The city's second reading of Hold: light can conceal; weight can be answered.
// Fixed state, independent random stream, and no writes into gameplay ownership.
#include "city.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

#define TAU 6.28318530718f
typedef struct { CityWindow view; int nextCross, crossAge, murmurWait, lampNear; } Window;
static Window windows[CITY_WINDOW_MAX];
static CityFish fish[CITY_FISH_COUNT];
static CityMural mural;
static CityFace face;
static int windowCount, activeRoom = -1;
static u32 age;
static u8 stoneAnswered[ITEM_MAX];
static u8 shoalWater[RH][RW];
static u32 rng;

static float Rnd(void) {
    rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
    return (rng & 0xffffu) / 65535.0f;
}
static float Clamp(float x, float lo, float hi) { return x < lo ? lo : x > hi ? hi : x; }
static float Dist2(float x, float y, float x2, float y2) { x -= x2; y -= y2; return x*x+y*y; }
static float RectDist2(float x, float y, float rx, float ry, float rw, float rh) {
    return Dist2(x, y, Clamp(x, rx, rx+rw), Clamp(y, ry, ry+rh));
}
static Color Ink(int r, int g, int b, float alpha) {
    return (Color){(u8)r,(u8)g,(u8)b,(u8)(255*Clamp(alpha,0,1))};
}

void CityInit(void) {
    activeRoom = roomIdx; age = 0; rng = 0xAD93742Bu ^ ((u32)roomIdx * 0x6c8e9cf5u);
    memset(windows,0,sizeof windows); memset(fish,0,sizeof fish);
    memset(&mural,0,sizeof mural); memset(&face,0,sizeof face);
    memset(stoneAnswered,0,sizeof stoneAnswered);
    memset(shoalWater,0,sizeof shoalWater);
    windowCount = roomIdx == 0 ? 1 : 3;
    static const float position[2][3][4] = {
        {{33*TS,10*TS,3*TS,3*TS},{0},{0}},
        {{5*TS,TS,3*TS,2*TS},{16*TS,TS,3*TS,2*TS},{37*TS,TS,2*TS,2*TS}}
    };
    for (int i=0;i<windowCount;i++) {
        CityWindow *v=&windows[i].view;
        v->x=position[roomIdx][i][0]; v->y=position[roomIdx][i][1];
        v->w=position[roomIdx][i][2]; v->h=position[roomIdx][i][3];
        v->light=1; v->facing=i&1 ? -1:1;
        windows[i].nextCross=240+(int)(Rnd()*480);
        windows[i].murmurWait=60+(int)(Rnd()*90);
    }
    mural=(CityMural){6*TS,16*TS,6*TS,3*TS,1};
    face=(CityFace){.x=21*TS,.y=16*TS,.w=8*TS,.h=5*TS,
        .eyeX={23.5f*TS,26.5f*TS},.eyeY=17.5f*TS,.pulse=.30f};
    // Both shoal halves are in actual water, on either side of the capital.
    for (int i=0;i<CITY_FISH_COUNT;i++) {
        fish[i].x=(i<4 ? 16.2f : 22.2f)*TS + (i%4)*2.3f;
        fish[i].y=(9.2f+(i%4)*.6f)*TS;
        fish[i].facing=i&1 ? -1:1;
    }
    if(roomIdx==1) {
        // A light in a disconnected outer pocket cannot pull these fish through
        // an island. Mark their actual basin once; the frozen map never changes.
        i16 qx[RW*RH],qy[RW*RH]; int head=0,tail=1;
        qx[0]=19; qy[0]=10; shoalWater[10][19]=1;
        static const int dx[4]={1,-1,0,0},dy[4]={0,0,1,-1};
        while(head<tail) {
            int x=qx[head],y=qy[head++];
            for(int k=0;k<4;k++) {
                int nx=x+dx[k],ny=y+dy[k];
                if(nx<0 || nx>=RW || ny<0 || ny>=RH || shoalWater[ny][nx] || !TileWater(tiles[ny][nx])) continue;
                shoalWater[ny][nx]=1; qx[tail]=(i16)nx; qy[tail++]=(i16)ny;
            }
        }
    }
}

static void WindowsStep(float lampX,float lampY,int hasLamp) {
    float px=player.x+player.w*.5f, py=player.y+player.h*.5f;
    for (int i=0;i<windowCount;i++) {
        Window *w=&windows[i]; CityWindow *v=&w->view;
        float cx=v->x+v->w*.5f,cy=v->y+v->h*.5f;
        int near=hasLamp && Dist2(lampX,lampY,cx,cy)<=40*40;
        if (near) {
            v->dark=1; v->returnFrames=0;
        } else if (w->lampNear) {
            // Wait is chosen on departure, and renewed if the lamp comes back.
            v->returnFrames=300+(int)(Rnd()*300);
        } else if (v->returnFrames>0 && --v->returnFrames==0) v->dark=0;
        w->lampNear=near;
        float target=v->dark ? 0:1;
        v->light += Clamp(target-v->light,-1.0f/18,1.0f/42);
        if (w->crossAge>0) {
            w->crossAge++;
            if (w->crossAge>150) { w->crossAge=0; w->nextCross=480+(int)(Rnd()*660); }
        } else if (--w->nextCross<=0) { w->crossAge=1; v->facing=Rnd()<.5f?-1:1; }
        v->crossing=w->crossAge>0;
        v->silhouette=w->crossAge/150.0f;
        if (w->murmurWait>0) w->murmurWait--;
        if (Dist2(px,py,cx,cy)<=24*24 && w->murmurWait<=0) {
            Sfx(SFX_CITY_MURMUR,.22f,.92f+i*.045f,cx/GW);
            w->murmurWait=720+(int)(Rnd()*480);
        }
    }
}

static void FaceStep(void) {
    face.pulse=.30f+.13f*(.5f-.5f*cosf(TAU*(age%420)/420.0f));
    if (face.acknowledgmentFrames>0) face.acknowledgmentFrames--;
    for (int i=0;i<itemCount;i++) {
        const Item *it=&items[i];
        if (it->kind!=IT_STONE) continue;
        float cx=it->x+2.5f,cy=it->y+2;
        int near=it->room==roomIdx && RectDist2(cx,cy,face.x,face.y,face.w,face.h)<=24*24;
        if (!near || i==heldItem) { stoneAnswered[i]=0; continue; }
        if (it->vy>.01f && !it->onGround && TileWater(TileAtPx(cx,cy)) && !stoneAnswered[i]) {
            stoneAnswered[i]=1;
            face.acknowledgmentFrames=120; face.responses++;
            Sfx(SFX_CITY_HUM,.48f,1,cx/GW);
        }
    }
    // Two-second answer with a soft release; it is not a repeating proximity alarm.
    float a=face.acknowledgmentFrames;
    face.acknowledgment=a>0 ? Clamp((121-a)/10.0f,0,1)*Clamp(a/30.0f,0,1) : 0;
}

static int WaterPoint(float x,float y) {
    return TileWater(TileAtPx(x,y)) && TileWater(TileAtPx(x-1,y)) && TileWater(TileAtPx(x+1,y));
}
static int FloatingLamp(float *x,float *y) {
    if (itemCount<1 || items[0].kind!=IT_LAMP || heldItem==0 || items[0].room!=roomIdx) return 0;
    const Item *it=&items[0]; *x=it->x+2; *y=it->y+2;
    int tx=(int)floorf(*x/TS),ty=(int)floorf((it->y+5)/TS);
    return tx>=0 && tx<RW && ty>=0 && ty<RH && shoalWater[ty][tx]
        && !it->onGround && TileWater(TileAtPx(*x,it->y+5)) && !TileWater(TileAtPx(*x,it->y));
}
static void FishStep(void) {
    float lx=0,ly=0; int gathering=FloatingLamp(&lx,&ly);
    float px=player.x+player.w*.5f,py=player.y+player.h*.5f;
    for (int i=0;i<CITY_FISH_COUNT;i++) {
        CityFish *f=&fish[i];
        // A floating lantern pulls the shoal upward. Solid masonry still divides
        // its two sides: fish swim around the capital, never through a tile.
        float tx=gathering?lx:19.5f*TS, ty=gathering?ly+13:10.6f*TS;
        tx+=sinf(age*.009f+i*2.3f)*7; ty+=cosf(age*.011f+i*1.8f)*5;
        float dx=tx-f->x,dy=ty-f->y,d=sqrtf(dx*dx+dy*dy);
        float ax=d>1 ? dx/d*.009f:0, ay=d>1 ? dy/d*.009f:0;
        float awayX=f->x-px,awayY=f->y-py,near=sqrtf(awayX*awayX+awayY*awayY);
        f->scatter=near<24 ? 1-near/24 : f->scatter*.94f;
        if (near<24) {
            if (near<.1f) { awayX=i&1?1:-1; awayY=-1; near=1.414214f; }
            ax+=awayX/near*.055f; ay+=awayY/near*.055f;
        }
        for (int j=0;j<CITY_FISH_COUNT;j++) if (i!=j) {
            float sx=f->x-fish[j].x,sy=f->y-fish[j].y,sq=sx*sx+sy*sy;
            if(sq>0.01f && sq<20) { ax+=sx*.002f; ay+=sy*.002f; }
        }
        f->vx=(f->vx+ax)*.96f; f->vy=(f->vy+ay)*.96f;
        float speed=sqrtf(f->vx*f->vx+f->vy*f->vy), cap=near<24?.62f:.25f;
        if(speed>cap) { f->vx*=cap/speed; f->vy*=cap/speed; }
        if(WaterPoint(f->x+f->vx,f->y)) f->x+=f->vx; else f->vx*=-.65f;
        if(WaterPoint(f->x,f->y+f->vy)) f->y+=f->vy; else f->vy*=-.65f;
        if(fabsf(f->vx)>.012f) f->facing=f->vx>0?1:-1;
        f->gathering=gathering;
    }
}

void CityStep(void) {
    // Keep the inherited one-event trace as an exact gameplay comparison. The
    // city has a separate auditable sound tally; actual Sfx playback is unchanged.
    const char *originalLastSfx=dbgLastSfx;
    if(activeRoom!=roomIdx) CityInit();
    age++;
    float lx=0,ly=0; int lamp=LampPos(&lx,&ly);
    WindowsStep(lx,ly,lamp);
    if(roomIdx==0) {
        int lit=lamp && RectDist2(lx,ly,mural.x,mural.y,mural.w,mural.h)<=48*48;
        // The camp's five-tile light reaches the procession's right edge.
        if(PropFireLit(0)) lit=1;
        mural.visibility=Clamp(mural.visibility+(lit?-1.0f/30:1.0f/120),0,1);
    } else { FaceStep(); FishStep(); }
    dbgLastSfx=originalLastSfx;
}
void CityPrintStats(void) {
    printf("CITY SFX city-murmur=%d city-hum=%d\n",sfxCount[SFX_CITY_MURMUR],sfxCount[SFX_CITY_HUM]);
}

int CityWindowViews(CityWindow *out,int max) {
    if(!out || max<=0) return 0;
    int n=windowCount<max?windowCount:max;
    for(int i=0;i<n;i++) out[i]=windows[i].view;
    return n;
}
int CityFishViews(CityFish *out,int max) {
    if(!out || max<=0 || roomIdx!=1) return 0;
    int n=max<CITY_FISH_COUNT?max:CITY_FISH_COUNT;
    memcpy(out,fish,(size_t)n*sizeof *out); return n;
}
int CityMuralView(CityMural *out) { if(!out || roomIdx!=0) return 0; *out=mural; return 1; }
int CityFaceView(CityFace *out) { if(!out || roomIdx!=1) return 0; *out=face; return 1; }

void CityLight(void) {
    for(int i=0;i<windowCount;i++) {
        CityWindow *v=&windows[i].view;
        LightAddPointCool(v->x+v->w*.5f,v->y+v->h*.5f,3.4f,.19f*v->light);
    }
    if(roomIdx==1) for(int i=0;i<2;i++)
        LightAddPointCool(face.eyeX[i],face.eyeY,3.7f,face.pulse*.6f+face.acknowledgment*.45f);
}

void CityDrawBack(void) {
    for(int i=0;i<windowCount;i++) {
        CityWindow *v=&windows[i].view;
        int x=(int)v->x,y=(int)v->y+ROOM_Y,w=(int)v->w,h=(int)v->h;
        DrawRectangle(x-1,y-1,w+2,h+2,Ink(27,47,47,1));
        DrawRectangle(x,y,w,h,Ink(5,13,18,1));
        DrawRectangle(x+2,y+2,w-4,h-4,Ink(74,114,98,.7f*v->light));
        if(v->crossing && v->light>.01f) {
            float t=v->facing>0?v->silhouette:1-v->silhouette;
            int sx=x+2+(int)(t*(w-7));
            DrawCircle(sx+2,y+5,2,Ink(7,16,22,.9f*v->light));
            DrawRectangle(sx,y+7,5,h-9,Ink(7,16,22,.9f*v->light));
        }
        for(int gx=x+3;gx<x+w-2;gx+=5) DrawRectangle(gx,y+1,1,h-2,Ink(24,45,46,1));
        DrawRectangle(x+1,y+h/2,w-2,1,Ink(24,45,46,1));
    }
    if(roomIdx==0) {
        Color c=Ink(104,155,123,mural.visibility*.62f);
        int y=(int)mural.y+ROOM_Y;
        // Tall bearers carry a dark sun: an image, never a sign or instruction.
        for(int i=0;i<4;i++) {
            int x=(int)mural.x+5+i*11;
            DrawRectangle(x,y+8,2,12,c); DrawRectangle(x-2,y+20,1,3,c);
            DrawRectangle(x+2,y+19,1,4,c); DrawRectangle(x-1,y+5,3,3,c);
            DrawLine(x,y+11,x+5,y+4,c); DrawLine(x,y+12,x-3,y+8,c);
        }
        DrawCircleLines((int)mural.x+24,y+5,8,c);
    } else {
        int x=(int)face.x,y=(int)face.y+ROOM_Y;
        DrawRectangle(x+4,y+2,56,34,Ink(21,39,43,1));
        DrawRectangle(x+9,y,46,3,Ink(43,61,58,1));
        DrawLine(x+9,y+7,x+4,y+31,Ink(49,64,59,1));
        DrawLine(x+54,y+7,x+59,y+31,Ink(49,64,59,1));
        DrawLine(x+32,y+9,x+29,y+26,Ink(43,61,58,1));
        DrawRectangle(x+19,y+30,26,2,Ink(7,22,29,1));
    }
}
void CityDrawFront(void) { /* City life glows after the light pass. */ }
void CityDrawGlow(void) {
    if(roomIdx!=1) return;
    float level=face.pulse+face.acknowledgment*.50f;
    for(int i=0;i<2;i++) DrawRectangle((int)face.eyeX[i]-3,ROOM_Y+(int)face.eyeY,6,2,Ink(142,207,166,level));
    for(int i=0;i<CITY_FISH_COUNT;i++) {
        CityFish *f=&fish[i];
        DrawRectangle((int)f->x,ROOM_Y+(int)f->y,2,1,Ink(138,196,164,.58f));
        DrawPixel((int)f->x-f->facing,ROOM_Y+(int)f->y,Ink(67,113,101,.52f));
    }
}
