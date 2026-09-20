// Native visual fixture: run the real frame loop, physics and renderer. Only the
// initial placement changes, as with the game's existing --room/--at/--lamp flags.
// The extra sinking stone is fixture setup, never a player verb or game option.
#define main game_main
#include "../../src/main.c"
#undef main

static void Capture(const char *folder,const char *name) {
    char path[1024]; snprintf(path,sizeof path,"%s/%s.png",folder,name);
    Image image=LoadImageFromScreen(); int ok=ExportImage(image,path); UnloadImage(image);
    CityFace f; CityMural m; CityWindow w[3]; CityFish fish[8];
    int nf=CityFishViews(fish,8); float meanY=0;
    for(int i=0;i<nf;i++) meanY+=fish[i].y;
    CityWindowViews(w,3);
    printf("CAPTURE %s frame=%ld exported=%d windowLight=%.4f",name,frameNo,ok,w[0].light);
    if(CityFaceView(&f)) printf(" pulse=%.4f answer=%.4f answerFrames=%d responses=%d",f.pulse,f.acknowledgment,f.acknowledgmentFrames,f.responses);
    if(CityMuralView(&m)) printf(" mural=%.4f",m.visibility);
    if(nf) printf(" fishMeanY=%.4f gathering=%d",meanY/nf,fish[0].gathering);
    printf("\n"); fflush(stdout);
    if(!ok) exit(2);
}
int main(int argc,char **argv) {
    if(argc!=3) { fprintf(stderr,"usage: city-capture CASE DIRECTORY\n"); return 2; }
    const char *mode=argv[1],*folder=argv[2];
    int water=!strncmp(mode,"face",4)||!strncmp(mode,"shoal",5);
    int lampRoom=water?0:1, lampTx=1,lampTy=14;
    if(!strcmp(mode,"mural-lit")) { lampRoom=0; lampTx=8; lampTy=18; }
    if(!strcmp(mode,"window-dark")) { lampRoom=0; lampTx=34; lampTy=12; }
    if(!strcmp(mode,"shoal-lamp")) { lampRoom=1; lampTx=22; lampTy=6; }
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(GW*4,GH*4,"City response review"); SetTargetFPS(0); RenderInit();
    ItemsReset(); ItemsAdd(IT_LAMP,lampRoom,lampTx,lampTy); RoomLoad(); AudioInit(1);
    RoomEnter(water?1:0); PlayerInit(water?25:65,water?37:141);
    dbgFixedStep=1; depthEnabled=1; depthStill=1;
    char planText[]="-:1200"; ParsePlan(planText);
    if(!strcmp(mode,"face")) ItemsAdd(IT_STONE,1,24,10);
    int last=!strncmp(mode,"shoal",5)?900:(!strcmp(mode,"face")?180:150);
    for(int n=1;n<=last;n++) {
        Frame();
        if(!strcmp(mode,"face")) {
            if(n==20) Capture(folder,"face-before");
            if(n==48) Capture(folder,"face-answer");
            if(n==180) Capture(folder,"face-after");
        } else if(n==last) Capture(folder,mode);
    }
    CityPrintStats(); DepthUnload(); CloseWindow(); return 0;
}
