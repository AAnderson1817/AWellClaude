// Real native frame-loop A/B capture and bounded throughput measurement.
// AWELL_DEPTH_NO_AO is a build-time review control, not a gameplay option.
#define main game_main
#include "../../src/main.c"
#undef main
static int CompareDuration(const void *a,const void *b) {
    double x=*(const double *)a,y=*(const double *)b;return (x>y)-(x<y);
}
int main(int argc,char **argv) {
    if(argc!=3) { fprintf(stderr,"usage: architecture-capture ROOM DIRECTORY\n");return 2; }
    int room=atoi(argv[1]);if(room<0||room>=ROOM_COUNT)return 2;
    SetTraceLogLevel(LOG_WARNING);InitWindow(GW*6,GH*6,"Architecture review");SetTargetFPS(0);RenderInit();
    ItemsReset();ItemsAdd(IT_LAMP,0,1,14);RoomLoad();AudioInit(1);RoomEnter(room);
    int tx=room?22:RoomStartTx(),ty=room?3:RoomStartTy();
    PlayerInit(tx*TS+1.f,(ty+1)*TS-11.f);dbgFixedStep=1;depthEnabled=1;depthStill=1;
    char planText[]="-:1500";ParsePlan(planText);
    for(int i=0;i<120;i++)Frame();
    char path[1024];snprintf(path,sizeof path,"%s/room-%d.png",argv[2],room);
    Image image=LoadImageFromScreen();int ok=ExportImage(image,path);UnloadImage(image);if(!ok)return 2;
    double duration[600],sum=0;
    for(int i=0;i<600;i++){double start=GetTime();Frame();duration[i]=(GetTime()-start)*1000.;sum+=duration[i];}
    qsort(duration,600,sizeof(double),CompareDuration);
    printf("{\"room\":%d,\"frames\":600,\"warmup_frames\":120,\"window\":[1920,1080],\"mean_frame_ms\":%.4f,\"median_frame_ms\":%.4f,\"p95_frame_ms\":%.4f,\"p99_frame_ms\":%.4f,\"measurement\":\"wall time around real native Frame with vsync disabled; local host only\"}\n",room,sum/600.,duration[300],duration[569],duration[593]);
    DepthUnload();CloseWindow();return 0;
}
