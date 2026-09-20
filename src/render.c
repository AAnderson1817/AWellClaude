// render.c -- the palette, the 320x180 target, and the CRT pass over the top.
#include "aw.h"
#include "inhabitants.h"
#include "hunter_pose.h"
#include <math.h>

RenderTexture2D screenRT;
static Shader post;
static int uTexSize;

// Everything is authored a stop brighter than it wants to look, because the light
// pass multiplies the whole frame down. A colour here is what a thing would be if
// something were shining directly on it.
Color palVoid     = {   4,   4,   8, 255 };
Color palBack     = {  46,  43,  66, 255 };
Color palBackLit  = {  60,  56,  86, 255 };
Color palRock     = {  58,  54,  76, 255 };
Color palRockDeep = {  32,  30,  44, 255 };
Color palRockLit  = { 106, 101, 134, 255 };
Color palLedge    = {  96,  80,  62, 255 };
Color palLedgeLit = { 158, 136, 104, 255 };
Color palVein     = { 176, 122,  62, 255 };
Color palVeinHot  = { 255, 216, 142, 255 };
Color palMoss     = {  62,  98,  70, 255 };
Color palSkin     = { 224, 216, 196, 255 };
Color palSkinDeep = { 150, 143, 128, 255 };
Color palEye      = { 236, 244, 255, 255 };
Color palPupil    = {  22,  20,  32, 255 };
Color palDrop     = { 130, 168, 196, 255 };
Color palBulb     = { 156, 104, 148, 255 };   // something grown, not cut
Color palBulbLit  = { 224, 178, 214, 255 };
Color palBulbDeep = {  92,  58,  90, 255 };
Color palWater    = {  38,  74, 104, 255 };
Color palWaterLit = { 112, 164, 182, 255 };   // the surface line
Color palWaterFleck={  60, 108, 140, 255 };
Color palSkinWet  = { 124, 134, 132, 255 };   // the part of you that is under
Color palBush     = {  44,  84,  56, 255 };
Color palBushLit  = {  92, 148,  96, 255 };
Color palBerry    = { 204,  84,  96, 255 };
Color palStalk    = {  56, 100,  62, 255 };
Color palLeaf     = {  70, 128,  78, 255 };
Color palPod      = { 214, 112, 150, 255 };   // the fruit: something that would be sweet
Color palPodLit   = { 246, 186, 206, 255 };
Color palPodDeep  = { 120,  50,  80, 255 };
Color palBird     = {  60,  62,  80, 255 };
Color palBirdLight= { 128, 130, 148, 255 };
Color palFur      = {  86,  68,  60, 255 };
Color palFurLight = { 138, 116, 100, 255 };
Color palEyeGreen = { 170, 240, 190, 255 };
Color palLampIron = {  58,  54,  66, 255 };
Color palLampGlass= { 255, 226, 172, 255 };
Color palLampHot  = { 255, 250, 232, 255 };
Color palStone    = {  98,  92,  96, 255 };   // warmer than the walls: it is not part of them
Color palStoneLit = { 150, 142, 140, 255 };
Color palStoneDeep= {  54,  50,  56, 255 };
// the city: dressed stone, a shade greyer and greener than the rock it was cut from
Color palAshlar    = {  64,  66,  82, 255 };
Color palAshlarLit = { 118, 122, 142, 255 };
Color palMortar    = {  40,  40,  54, 255 };
Color palCornice   = {  92,  96, 104, 255 };
Color palCorniceLit= { 152, 158, 164, 255 };
Color palLichen    = { 108, 128, 108, 255 };
Color palDoor      = {  40,  56,  54, 255 };   // the door's face: iron gone green
Color palDoorGroove= {  66, 102,  90, 255 };
Color palCityGlass = { 120, 186, 150, 255 };   // their light. Not yours
Color palCityGlassLit={ 200, 244, 214, 255 };
Color palIron      = {  70,  68,  78, 255 };
Color palRope      = { 132, 104,  72, 255 };
Color palCloth     = {  48, 108, 110, 255 };   // dyed teal, the one saturated thing they left
Color palClothLit  = {  92, 160, 158, 255 };
Color palClay      = { 156,  96,  74, 255 };
Color palClayLit   = { 206, 146, 112, 255 };
Color palBone      = { 214, 206, 186, 255 };
Color palEmber     = { 196,  70,  40, 255 };
Color palFlame     = { 244, 150,  60, 255 };
Color palFlameHot  = { 255, 232, 170, 255 };

// Not an overlay: scanline depth is modulated per pixel by luminosity, so a bright
// pixel blooms across the gap and a dark one sinks into it. X stays bilinear, Y is
// snapped to source pixel centres. The dither at the end is there to break up the
// banding that 8-bit colour puts across a large flat dark field.
#if defined(PLATFORM_WEB)
static const char *FS =
"#version 100\n"
"precision mediump float;\n"
"varying vec2 fragTexCoord;\n"
"varying vec4 fragColor;\n"
"uniform sampler2D texture0;\n"
"uniform vec2 uTexSize;\n"
"void main() {\n"
"  vec2 uv = fragTexCoord;\n"
"  float py = (floor(uv.y * uTexSize.y) + 0.5) / uTexSize.y;\n"
"  vec3 c = texture2D(texture0, vec2(uv.x, py)).rgb;\n"
"  float lum = dot(c, vec3(0.299, 0.587, 0.114));\n"
"  float f = fract(uv.y * uTexSize.y);\n"
"  float d = abs(f - 0.5) * 2.0;\n"
"  float depth = mix(0.30, 0.04, smoothstep(0.0, 0.70, lum));\n"
"  c *= 1.0 - depth * pow(d, 1.55);\n"
"  vec2 q = uv - 0.5;\n"
"  float vig = clamp(1.0 - dot(q * vec2(1.0, 0.86), q * vec2(1.0, 0.86)) * 0.55, 0.0, 1.0);\n"
"  c *= mix(1.0, vig, 0.50);\n"
"  float dth = fract(sin(dot(floor(gl_FragCoord.xy), vec2(12.9898, 78.233))) * 43758.5453);\n"
"  c += (dth - 0.5) * (1.6 / 255.0);\n"
"  gl_FragColor = vec4(c, 1.0);\n"
"}\n";
#else
static const char *FS =
"#version 330\n"
"in vec2 fragTexCoord;\n"
"in vec4 fragColor;\n"
"out vec4 finalColor;\n"
"uniform sampler2D texture0;\n"
"uniform vec2 uTexSize;\n"
"void main() {\n"
"  vec2 uv = fragTexCoord;\n"
"  float py = (floor(uv.y * uTexSize.y) + 0.5) / uTexSize.y;\n"
"  vec3 c = texture(texture0, vec2(uv.x, py)).rgb;\n"
"  float lum = dot(c, vec3(0.299, 0.587, 0.114));\n"
"  float f = fract(uv.y * uTexSize.y);\n"
"  float d = abs(f - 0.5) * 2.0;\n"
"  float depth = mix(0.30, 0.04, smoothstep(0.0, 0.70, lum));\n"
"  c *= 1.0 - depth * pow(d, 1.55);\n"
"  vec2 q = uv - 0.5;\n"
"  float vig = clamp(1.0 - dot(q * vec2(1.0, 0.86), q * vec2(1.0, 0.86)) * 0.55, 0.0, 1.0);\n"
"  c *= mix(1.0, vig, 0.50);\n"
"  float dth = fract(sin(dot(floor(gl_FragCoord.xy), vec2(12.9898, 78.233))) * 43758.5453);\n"
"  c += (dth - 0.5) * (1.6 / 255.0);\n"
"  finalColor = vec4(c, 1.0);\n"
"}\n";
#endif

static float HunterBound(float value,float low,float high){return fminf(high,fmaxf(low,value));}
static Color HunterInk(float u,float v,float gazeX,float gazeY,float mouth){
    Color color={0,0,0,0};
    const Color canvas={103,87,57,255},felt={150,126,83,255},leather={60,43,28,255};
    // Same seed dimensions as the player. The asymmetric rear pack and actual
    // brim silhouette distinguish this person without a new body archetype.
    if(u>=-4.8f&&u<-2.0f&&v>=2.7f&&v<8.8f)color=canvas;
    if(u>=-4.8f&&u<-2.0f&&v>=7.8f&&v<8.8f)color=leather;
    if((u>=-3&&u<3&&v>=1&&v<10)||(u>=-2&&u<2&&v>=0&&v<11)){
        color=palSkin;
        if(v<1.1f||(fabsf(u)>2.15f&&v<3))color=palSkinDeep;
        if(fabsf(fabsf(u)-2.25f)<.28f&&v>3.1f&&v<9.3f)color=leather;
        for(int side=-1;side<=1;side+=2){
            if(fabsf(u-(side*1.65f+gazeX))<.52f&&fabsf(v-(8.05f+gazeY))<.95f){
                color=palPupil;if(v>8.30f+gazeY)color=palEye;
            }
        }
        if(mouth>.22f&&fabsf(u-gazeX*.3f)<.55f&&fabsf(v-6.05f)<.42f)color=palPupil;
    }
    if(u>=-5.5f&&u<5.5f&&v>=10.45f&&v<11.35f)color=felt;
    if(u>=-3.0f&&u<2.5f&&v>=11.2f&&v<13.8f){
        color=felt;if(v<11.8f)color=leather;
        if(v>13.3f&&(u< -2.5f||u>2.0f))color=(Color){0,0,0,0};
    }
    return color;
}
void HunterDraw(void){
    HunterView h;if(!InhabitantsHunterView(&h))return;
    float cx=h.x+h.w*.5f,floor=h.y+h.h;
    int moving=HunterPoseMoving(&h),seated=HunterPoseSeated(&h);
    float roll=HunterPoseRoll(&h);
    float co=cosf(roll),si=sinf(roll),handsX[2],handsY[2];int handsValid[2]={0};
    for(int side=-1;side<=1;side+=2){
        HunterArm arm=HunterPoseArm(&h,side,0);int k=(side+1)/2;
        if(arm.reachable){
            int ex=(int)roundf(arm.elbow.x),ey=(int)roundf(-arm.elbow.y)+ROOM_Y;
            DrawLine((int)roundf(arm.shoulder.x),(int)roundf(-arm.shoulder.y)+ROOM_Y,ex,ey,palSkinDeep);
            DrawLine(ex,ey,(int)roundf(arm.hand.x),(int)roundf(-arm.hand.y)+ROOM_Y,palSkin);
            handsX[k]=arm.hand.x;handsY[k]=-arm.hand.y;handsValid[k]=1;
        }
        float stride=moving?sinf(h.walkPhase*2.f)*.60f:0;
        DrawRectangle((int)floorf(cx+side*1.44f+(seated?h.facing*.64f:0)-1),
            (int)floorf(floor-1-fmaxf(0,side*stride))+ROOM_Y,2,1,palSkinDeep);
    }
    float gazeX=HunterBound((h.lookX-cx)/(TS*3.f),-1,1)*.52f;
    float gazeY=HunterBound((floor-h.lookY-h.h*.73f)*.08f,-.28f,.28f);
    // Inverse rasterization rotates the rigid sprite, with no gaps from forward
    // pixel splats and no body scaling. The common pivot remains on the floor.
    for(int y=(int)floorf(floor)-16;y<=(int)ceilf(floor);y++)for(int x=(int)floorf(cx)-8;x<=(int)ceilf(cx)+8;x++){
        float dx=x+.5f-cx,dy=floor-(y+.5f);
        Color ink=HunterInk(dx*co+dy*si,-dx*si+dy*co,gazeX,gazeY,h.mouthOpen);
        if(ink.a)DrawPixel(x,y+ROOM_Y,ink);
    }
    for(int k=0;k<2;k++)if(handsValid[k])DrawPixel((int)roundf(handsX[k]),(int)roundf(handsY[k])+ROOM_Y,palSkin);
}
int HunterDrawStone(int item){
    HunterView h;
    if(item<0||item>=itemCount||item==heldItem||items[item].room!=roomIdx||items[item].kind!=IT_STONE||
       !InhabitantsHunterView(&h)||h.state!=HUNTER_TEND||h.carriedItem!=item)return 0;
    // Rotate the original five-by-four stone palette about its real position.
    // Returning one replaces its normal draw; no extra stone proxy is created.
    float cx=floorf(items[item].x)+2.5f,cy=floorf(items[item].y)+2.f;
    float co=cosf(h.stoneTurn),si=sinf(h.stoneTurn);
    for(int y=(int)cy-4;y<=(int)cy+4;y++)for(int x=(int)cx-4;x<=(int)cx+4;x++){
        float dx=x+.5f-cx,dy=y+.5f-cy;
        int u=(int)floorf(dx*co-dy*si+2.5f),v=(int)floorf(dx*si+dy*co+2.f);
        if(u<0||u>=5||v<0||v>=4||(v==0&&(u==0||u==4)))continue;
        Color ink=palStone;
        if(v==0&&(u==1||u==2))ink=palStoneLit;
        if(v==3||(u==3&&v==2))ink=palStoneDeep;
        DrawPixel(x,y+ROOM_Y,ink);
    }
    return 1;
}

void RenderInit(void) {
    screenRT = LoadRenderTexture(GW, GH);
    SetTextureFilter(screenRT.texture, TEXTURE_FILTER_BILINEAR);
    post = LoadShaderFromMemory(0, FS);
    uTexSize = GetShaderLocation(post, "uTexSize");
}

void RenderBegin(void) {
    BeginTextureMode(screenRT);
    ClearBackground(palVoid);
}

void RenderPresent(void) {
    EndTextureMode();
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    int s = sw / GW, sy = sh / GH;
    if (sy < s) s = sy;
    if (s < 1) s = 1;
    int ox = (sw - GW * s) / 2, oy = (sh - GH * s) / 2;
    float ts[2] = { (float)GW, (float)GH };

    BeginDrawing();
        ClearBackground(BLACK);
        SetShaderValue(post, uTexSize, ts, SHADER_UNIFORM_VEC2);
        BeginShaderMode(post);
            DrawTexturePro(screenRT.texture,
                (Rectangle){ 0, 0, (float)GW, -(float)GH },
                (Rectangle){ (float)ox, (float)oy, (float)(GW * s), (float)(GH * s) },
                (Vector2){ 0, 0 }, 0.0f, WHITE);
        EndShaderMode();
    EndDrawing();
}
