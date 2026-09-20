// Real native frame-loop A/B capture and bounded throughput measurement.
// AWELL_DEPTH_NO_AO is a build-time review control, not a gameplay option.
#define main game_main
#include "main.c"
#undef main
static int CompareDuration(const void *a,const void *b) {
    double x=*(const double *)a,y=*(const double *)b;return (x>y)-(x<y);
}
int main(int argc,char **argv) {
    if(argc!=3) { fprintf(stderr,"usage: architecture-capture ROOM DIRECTORY\n");return 2; }
    int room=atoi(argv[1]);if(room<0||room>=ROOM_COUNT)return 2;
    SetTraceLogLevel(LOG_WARNING);InitWindow(GW*6,GH*6,"Architecture review");SetTargetFPS(60);RenderInit();
    ItemsReset();ItemsAdd(IT_LAMP,0,1,14);RoomLoad();if(!InhabitantsInit())return 2;AudioInit(1);AudioHunterInit();RoomEnter(room);
    int tx=room?22:RoomStartTx(),ty=room?3:RoomStartTy();
    PlayerInit(tx*TS+1.f,(ty+1)*TS-11.f);dbgFixedStep=1;depthEnabled=1;depthStill=1;
    char planText[]="-:1500";ParsePlan(planText);
    // Rest outside measured work so this review does not run an unrestricted
    // GPU saturation loop. Both comparison builds use the same 4 ms interval.
    for(int i=0;i<120;i++){WaitTime(.004);Frame();}
    char path[1024];snprintf(path,sizeof path,"%s/room-%d.png",argv[2],room);
    Image image=LoadImageFromScreen();int ok=ExportImage(image,path);UnloadImage(image);if(!ok)return 2;
    printf("CAPTURE_ONLY room=%d frames=120 fps_cap=60\n",room);
    DepthUnload();AudioHunterClose();CloseWindow();return 0;
}
