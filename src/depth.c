// A fixed-stage 3D presentation of the existing rooms. All measurements originate
// in the existing tile/physics state. Nothing in this file is read by a game rule.
#include "aw.h"
#include "depth.h"
#include "raymath.h"
#include "rlgl.h"
#include "generated/environment_mattes.h"
#include "generated/foundry_assets.h"
#include "generated/terrain_assets.h"
#include "generated/life_assets.h"
#include <math.h>
#include <string.h>

#define PI_F 3.14159265359f
#define DW 1920
#define DH 1080
static int ready;
static Camera3D camera;
static RenderTexture2D target,waterTarget;
static Shader surface, finish,waterShader;
static Texture2D waterMask;
static Model cube, orb, cylinder, cone, torus, leaf;
static Texture2D mattes[2];
static Texture2D depthLight;
static Model doorModels[8],orreryModels[8];
static Model terrainModels[ROOM_COUNT][12];
static Model potModels[8],lampModels[8],seedModels[8],beastBodyModels[8],beastHeadModels[8],birdModels[8];
static int metalLoc, roughLoc, emissionLoc, cameraLoc, timeLoc;
static const Color BASALT = {60,72,82,255};
static const Color STONE = {95,108,112,255};
static const Color BRONZE = {139,114,73,255};
static const Color COPPER = {71,119,108,255};
static const Color COOL = {166,234,192,255};
static const Color WARM = {255,172,82,255};

#if defined(PLATFORM_WEB)
#define VHEADER "#version 100\nprecision highp float;\n#define IN attribute\n#define OUT varying\n"
#define FHEADER "#version 100\nprecision highp float;\n#define IN varying\n#define SAMPLE texture2D\n#define RESULT gl_FragColor\n"
#else
#define VHEADER "#version 330\n#define IN in\n#define OUT out\n"
#define FHEADER "#version 330\n#define IN in\n#define SAMPLE texture\nout vec4 finalColor;\n#define RESULT finalColor\n"
#endif
static const char *VERT = VHEADER
"IN vec3 vertexPosition; IN vec3 vertexNormal; IN vec2 vertexTexCoord; IN vec4 vertexColor;\n"
"uniform mat4 mvp; uniform mat4 matModel; uniform mat4 matNormal;\n"
"OUT vec3 p; OUT vec3 n; OUT vec2 uv; OUT vec4 vc;\n"
"void main(){ p=(matModel*vec4(vertexPosition,1.)).xyz; n=normalize((matNormal*vec4(vertexNormal,0.)).xyz);"
"uv=vertexTexCoord;vc=vertexColor;gl_Position=mvp*vec4(vertexPosition,1.);}\n";
static const char *FRAG = FHEADER
"IN vec3 p; IN vec3 n; IN vec2 uv; IN vec4 vc;\n"
"uniform vec4 colDiffuse; uniform sampler2D lightMap; uniform vec3 eye;\n"
"uniform float metalness; uniform float roughness; uniform float emission;\n"
"float hash(vec3 v){return fract(sin(dot(v,vec3(12.9898,78.233,32.21)))*43758.5453);}\n"
"float noise3(vec3 v){vec3 i=floor(v),f=fract(v);f=f*f*(3.-2.*f);\n"
" float a=mix(hash(i),hash(i+vec3(1,0,0)),f.x),b=mix(hash(i+vec3(0,1,0)),hash(i+vec3(1,1,0)),f.x);\n"
" float c=mix(hash(i+vec3(0,0,1)),hash(i+vec3(1,0,1)),f.x),d=mix(hash(i+vec3(0,1,1)),hash(i+vec3(1,1,1)),f.x);\n"
" return mix(mix(a,b,f.y),mix(c,d,f.y),f.z);}\n"
"void main(){\n"
" vec3 N=normalize(n); vec3 V=normalize(eye-p); vec3 L=normalize(vec3(-.45,.8,.6));vec3 H=normalize(L+V);\n"
" vec2 luv=vec2((p.x+.5)/41.,(22.-p.y+.5)/23.);\n"
" vec3 baked=SAMPLE(lightMap,clamp(luv,vec2(.01),vec2(.99))).rgb;\n"
" float ndl=max(dot(N,L),0.); float edge=pow(1.-max(dot(N,V),0.),3.);\n"
" float grain=hash(floor(p*23.))*.035+sin(p.x*6.1+p.y*8.4)*.019;\n"
" vec3 base=colDiffuse.rgb*vc.rgb*(1.+grain);\n"
" float age=noise3(p*3.4)*.7+noise3(p*12.)*.3;\n"
" float oxidation=smoothstep(.46,.69,age)*metalness*.42;\n"
" base=mix(base,base*vec3(.43,.81,.71),oxidation);\n"
" base*=.94+age*.12;\n"
" float spec=pow(max(dot(N,H),0.),mix(92.,13.,roughness))*(.035+metalness*.75);\n"
" vec3 light=vec3(.035,.055,.07)+baked*1.8;\n"
" vec3 c=base*light*(.48+ndl*.55)+mix(vec3(.65,.75,.78),base,metalness)*spec*(.03+length(baked)*.8);\n"
" c+=base*edge*.045+base*emission;\n"
" float fog=1.-exp(-max(-p.z-5.,0.)*.072);\n"
" c=mix(c,vec3(.075,.13,.145),fog);\n"
" RESULT=vec4(c,colDiffuse.a);}\n";
static const char *POST = FHEADER
"IN vec2 fragTexCoord; uniform sampler2D texture0; uniform float time;\n"
"void main(){vec2 uv=fragTexCoord;vec3 c=SAMPLE(texture0,uv).rgb;\n"
" vec2 q=(uv-.5)*vec2(1.,.84);float v=1.-dot(q,q)*.7;c*=v;\n"
" c=c/(c+vec3(.8))*1.1;c=pow(c,vec3(.91));\n"
" float g=fract(sin(dot(floor(uv*vec2(1920.,1080.)),vec2(12.9898,78.233)))*43758.5453);\n"
" c+=(g-.5)*.004;RESULT=vec4(c,1.);}\n";
static const char *WATER_FRAG = FHEADER
"IN vec2 fragTexCoord;uniform sampler2D texture0;uniform sampler2D mask;uniform float waterTime;\n"
"void main(){vec2 uv=fragTexCoord;vec3 c=SAMPLE(texture0,uv).rgb;\n"
" vec2 m=vec2(uv.x,(1.-uv.y)*22.5/22.-.25/22.);\n"
" if(SAMPLE(mask,m).r<.5){RESULT=vec4(c,1.);return;}\n"
" float line=.6777778;float d=max(0.,line-uv.y);\n"
" float wave=sin(uv.x*95.+waterTime*.7+d*55.)*.0015*d;\n"
" vec2 reflectUV=vec2(uv.x+wave,clamp(2.*line-uv.y,0.,1.));\n"
" vec3 reflected=SAMPLE(texture0,reflectUV).rgb;\n"
" c=c*vec3(.62,.82,.85)+vec3(.007,.032,.035);\n"
" c=mix(c,reflected*vec3(.62,.85,.8),.25*exp(-d*10.));\n"
" c+=vec3(.045,.08,.073)*max(0.,1.-d/.006);\n"
" float caustic=pow(max(0.,sin(uv.x*75.+sin(uv.y*48.+waterTime*.22)*1.3)),18.);\n"
" c+=vec3(.009,.018,.014)*caustic*exp(-d*3.);\n"
" RESULT=vec4(c,1.);}\n";

static float Y(float py) { return 22.f-py/TS; }
static float R(int x,int y) { return (Hash2(x,y)&65535)/65535.f; }
static Color Shade(Color c,float k) {
    return (Color){(u8)fminf(255,c.r*k),(u8)fminf(255,c.g*k),(u8)fminf(255,c.b*k),c.a};
}
static Mesh RoundedMesh(void) {
    Mesh m={0}; const int steps=4; int count=6*steps*steps*6;
    m.vertexCount=count;m.triangleCount=count/3;
    m.vertices=MemAlloc(count*3*sizeof(float));m.normals=MemAlloc(count*3*sizeof(float));
    m.texcoords=MemAlloc(count*2*sizeof(float)); int at=0;
    const int corners[6][2]={{0,0},{1,0},{1,1},{0,0},{1,1},{0,1}};
    for(int face=0;face<6;face++) for(int v=0;v<steps;v++) for(int u=0;u<steps;u++) for(int k=0;k<6;k++){
        float a=(u+corners[k][0])/(float)steps-.5f,b=(v+corners[k][1])/(float)steps-.5f;
        Vector3 p;
        switch(face){
            case 0:p=(Vector3){a,b,.5f};break;case 1:p=(Vector3){-a,b,-.5f};break;
            case 2:p=(Vector3){.5f,b,-a};break;case 3:p=(Vector3){-.5f,b,a};break;
            case 4:p=(Vector3){a,.5f,-b};break;default:p=(Vector3){a,-.5f,b};break;
        }
        Vector3 inner={Clamp(p.x,-.455f,.455f),Clamp(p.y,-.455f,.455f),Clamp(p.z,-.455f,.455f)};
        Vector3 norm=Vector3Normalize(Vector3Subtract(p,inner));p=Vector3Add(inner,Vector3Scale(norm,.045f));
        m.vertices[at*3]=p.x;m.vertices[at*3+1]=p.y;m.vertices[at*3+2]=p.z;
        m.normals[at*3]=norm.x;m.normals[at*3+1]=norm.y;m.normals[at*3+2]=norm.z;
        m.texcoords[at*2]=a+.5f;m.texcoords[at*2+1]=b+.5f;at++;
    }UploadMesh(&m,false);return m;
}
static Mesh LeafMesh(void){
    Mesh m={0};m.vertexCount=24;m.triangleCount=8;m.vertices=MemAlloc(72*sizeof(float));m.normals=MemAlloc(72*sizeof(float));m.texcoords=MemAlloc(48*sizeof(float));
    Vector3 points[6]={{0,0,0},{-.17f,.34f,0},{-.13f,.7f,-.04f},{0,1,-.15f},{.13f,.7f,-.04f},{.17f,.34f,0}};
    int idx[24]={0,1,2,0,2,3,0,3,4,0,4,5,2,1,0,3,2,0,4,3,0,5,4,0};
    for(int i=0;i<24;i++){Vector3 p=points[idx[i]];m.vertices[i*3]=p.x;m.vertices[i*3+1]=p.y;m.vertices[i*3+2]=p.z;m.normals[i*3]=0;m.normals[i*3+1]=.15f;m.normals[i*3+2]=i<12?1:-1;m.texcoords[i*2]=p.x+.5f;m.texcoords[i*2+1]=p.y;}
    UploadMesh(&m,false);return m;
}
static void Setup(Model *m){m->materials[0].shader=surface;}
static void LoadAsset(const FoundryAssetData *asset,Model *models){
    for(int i=0;i<asset->mesh_count;i++){
        const FoundryMeshData *src=&asset->meshes[i];Mesh mesh={0};
        mesh.vertexCount=src->vertex_count;mesh.triangleCount=src->index_count/3;
        mesh.vertices=MemAlloc(src->vertex_count*3*sizeof(float));memcpy(mesh.vertices,src->positions,src->vertex_count*3*sizeof(float));
        mesh.normals=MemAlloc(src->vertex_count*3*sizeof(float));memcpy(mesh.normals,src->normals,src->vertex_count*3*sizeof(float));
        mesh.texcoords=MemAlloc(src->vertex_count*2*sizeof(float));memset(mesh.texcoords,0,src->vertex_count*2*sizeof(float));
        mesh.indices=MemAlloc(src->index_count*sizeof(unsigned short));memcpy(mesh.indices,src->indices,src->index_count*sizeof(unsigned short));
        UploadMesh(&mesh,false);models[i]=LoadModelFromMesh(mesh);Setup(&models[i]);
    }
}
static void Init(void){
    surface=LoadShaderFromMemory(VERT,FRAG);finish=LoadShaderFromMemory(0,POST);
    waterShader=LoadShaderFromMemory(0,WATER_FRAG);
    surface.locs[SHADER_LOC_MATRIX_MODEL]=GetShaderLocation(surface,"matModel");
    surface.locs[SHADER_LOC_MATRIX_NORMAL]=GetShaderLocation(surface,"matNormal");
    surface.locs[SHADER_LOC_MAP_EMISSION]=GetShaderLocation(surface,"lightMap");
    metalLoc=GetShaderLocation(surface,"metalness");roughLoc=GetShaderLocation(surface,"roughness");emissionLoc=GetShaderLocation(surface,"emission");cameraLoc=GetShaderLocation(surface,"eye");timeLoc=GetShaderLocation(finish,"time");
    cube=LoadModelFromMesh(RoundedMesh());orb=LoadModelFromMesh(GenMeshSphere(1,16,24));
    cylinder=LoadModelFromMesh(GenMeshCylinder(1,1,24));cone=LoadModelFromMesh(GenMeshCone(1,1,16));
    torus=LoadModelFromMesh(GenMeshTorus(.035f,1.f,12,64));leaf=LoadModelFromMesh(LeafMesh());
    Setup(&cube);Setup(&orb);Setup(&cylinder);Setup(&cone);Setup(&torus);Setup(&leaf);
    LoadAsset(&FOUNDRY_VAULT_DOOR,doorModels);LoadAsset(&FOUNDRY_ORRERY,orreryModels);
    LoadAsset(&FOUNDRY_VAULT_TERRAIN,terrainModels[0]);LoadAsset(&FOUNDRY_DROWNED_TERRAIN,terrainModels[1]);
    LoadAsset(&FOUNDRY_POT,potModels);LoadAsset(&FOUNDRY_HUNTER_LANTERN,lampModels);
    LoadAsset(&FOUNDRY_PLAYER_SEED,seedModels);LoadAsset(&FOUNDRY_BEAST_BODY,beastBodyModels);
    LoadAsset(&FOUNDRY_BEAST_HEAD,beastHeadModels);LoadAsset(&FOUNDRY_BIRD_BODY,birdModels);
    target=LoadRenderTexture(DW,DH);SetTextureFilter(target.texture,TEXTURE_FILTER_BILINEAR);
    waterTarget=LoadRenderTexture(DW,DH);SetTextureFilter(waterTarget.texture,TEXTURE_FILTER_BILINEAR);
    Image maskImage=GenImageColor(RW,RH,BLACK);waterMask=LoadTextureFromImage(maskImage);UnloadImage(maskImage);
    SetTextureFilter(waterMask,TEXTURE_FILTER_POINT);SetTextureWrap(waterMask,TEXTURE_WRAP_CLAMP);
    Image lightImage=GenImageColor(RW+1,RH+1,WHITE);depthLight=LoadTextureFromImage(lightImage);UnloadImage(lightImage);
    SetTextureFilter(depthLight,TEXTURE_FILTER_BILINEAR);SetTextureWrap(depthLight,TEXTURE_WRAP_CLAMP);
    Image matte=LoadImageFromMemory(".jpg",vault_matte,sizeof vault_matte);
    mattes[0]=LoadTextureFromImage(matte);UnloadImage(matte);SetTextureFilter(mattes[0],TEXTURE_FILTER_BILINEAR);
#if DROWNED_MATTE_AVAILABLE
    matte=LoadImageFromMemory(".jpg",drowned_matte,sizeof drowned_matte);
    mattes[1]=LoadTextureFromImage(matte);UnloadImage(matte);SetTextureFilter(mattes[1],TEXTURE_FILTER_BILINEAR);
#endif
    // Perspective is calibrated to the original 40 x 22.5 stage at z=0.
    // The locked camera never follows the player. Deep layers provide scale and depth cues.
    camera=(Camera3D){{20,11,52},{20,11,0},{0,1,0},24.415162f,CAMERA_PERSPECTIVE};
    ready=1;
}
static void Draw(Model *m,Vector3 pos,Vector3 scale,Vector3 axis,float angle,Color color,float metal,float rough,float glow){
    m->materials[0].maps[MATERIAL_MAP_EMISSION].texture=depthLight;
    SetShaderValue(surface,metalLoc,&metal,SHADER_UNIFORM_FLOAT);SetShaderValue(surface,roughLoc,&rough,SHADER_UNIFORM_FLOAT);SetShaderValue(surface,emissionLoc,&glow,SHADER_UNIFORM_FLOAT);
    DrawModelEx(*m,pos,axis,angle,scale,color);
}
static void Box(float x,float y,float z,float w,float h,float d,Color c){Draw(&cube,(Vector3){x,y,z},(Vector3){w,h,d},(Vector3){0,0,1},0,c,0,.85f,0);}
static void MetalBox(float x,float y,float z,float w,float h,float d,Color c){Draw(&cube,(Vector3){x,y,z},(Vector3){w,h,d},(Vector3){0,0,1},0,c,.82f,.35f,0);}
static void Ellipse(float x,float y,float z,float w,float h,float d,Color c,float glow){Draw(&orb,(Vector3){x,y,z},(Vector3){w,h,d},(Vector3){0,0,1},0,c,0,.7f,glow);}
static void Rod(Vector3 a,Vector3 b,float r,Color c,float metal){
    Vector3 v=Vector3Subtract(b,a);float len=Vector3Length(v);if(len<.001f)return;
    Vector3 n=Vector3Scale(v,1/len),axis=Vector3CrossProduct((Vector3){0,1,0},n);float angle=acosf(Clamp(n.y,-1,1))*RAD2DEG;
    if(Vector3Length(axis)<.001f)axis=(Vector3){1,0,0};
    Draw(&cylinder,a,(Vector3){r,len,r},axis,angle,c,metal,metal?.35f:.8f,0);
}
static void Ring(float x,float y,float z,float r,Color c,Vector3 axis,float angle){Draw(&torus,(Vector3){x,y,z},(Vector3){r,r,r},axis,angle,c,.82f,.32f,0);}
static void AssetPose(const FoundryAssetData *asset,Model *models,Vector3 pos,float scale,Vector3 axis,float angle,float extraGlow){
    for(int i=0;i<asset->mesh_count;i++){
        const FoundryMeshData *m=&asset->meshes[i];Color c={m->color[0],m->color[1],m->color[2],m->color[3]};
        float glow=m->emission;
        if(pos.z<-5){c=Shade(c,.7f);glow*=.4f;}
        Draw(&models[i],pos,(Vector3){scale,scale,scale},axis,angle,c,m->metallic,m->roughness,glow+extraGlow);
    }
}
static void Asset(const FoundryAssetData *asset,Model *models,Vector3 pos,float scale){AssetPose(asset,models,pos,scale,(Vector3){0,1,0},0,0);}
static void Foliage(float x,float y,float z,float size,int seed,float shake){
    float t=depthStill?0:frameNo*DT;
    float dx=(player.x+player.w*.5f)/TS-x;
    float lean=(z==.15f&&fabsf(dx)<1.5f&&fabsf(Y(player.y+player.h)-y)<1.5f)?(dx>0?9.f:-9.f):0;
    for(int i=0;i<9;i++){
        float a=(i-4)*19.f+sinf(t*.65f+seed)*2+shake*sinf(t*15+i)*12+lean;
        Color c=i%3==0?(Color){83,125,107,255}:(Color){42,82,73,255};
        Draw(&leaf,(Vector3){x+(R(i,seed)-.5f)*size*.3f,y,z+(R(seed,i)-.5f)*.45f},(Vector3){size,size*(.55f+R(i+1,seed)),size},(Vector3){0,0,1},a,c,0,.88f,0);
    }
}
static void Background(void){
    Texture2D matte=mattes[roomIdx].id?mattes[roomIdx]:mattes[0];
    DrawBillboardRec(camera,matte,(Rectangle){0,0,matte.width,matte.height},(Vector3){20,11,-33},(Vector2){65.4f,36.8f},(Color){88,112,112,255});
    // Architecture is part of the distant matte. Additional sharply outlined
    // arches here would look like reachable ledges against the interactive stage.
    // The city's bronze instruments share their concentric form with the sealed door.
    float sx=roomIdx==0?25.f:27.f,sy=roomIdx==0?10.5f:3.7f,sz=-26.f;
    Asset(&FOUNDRY_ORRERY,orreryModels,(Vector3){sx,sy,sz},1.f);
    Box(sx,sy-8,sz,.7f,16,.8f,Shade(STONE,.6f));
    // Small apertures and star records; broad wall regions remain quiet.
    for(int i=0;i<20;i++){
        float x=3+R(i,42)*36,y=3+R(i,43)*19;
        Ellipse(x,y,-18,.025f,.025f,.025f,Shade(COOL,.65f),.4f);
    }
}
static int IsCity(int x,int y){return roomIdx==1||ZoneAt(x,y)==Z_CITY;}
static void Terrain(void){
    Asset(roomIdx==0?&FOUNDRY_VAULT_TERRAIN:&FOUNDRY_DROWNED_TERRAIN,terrainModels[roomIdx],(Vector3){0,0,0},1);
    for(int y=0;y<RH;y++)for(int x=0;x<RW;x++){
        int t=tiles[y][x],city=IsCity(x,y);float yy=22-y;
        if(TileSolid(t)){
            Color c=city?STONE:BASALT;c=Shade(c,.78f+R(x,y)*.28f);


            // Top facets are aligned to the exact collider. No foreground mesh
            // may cover this lip: it is the visual contract for every landing.
            if(y>0&&!TileSolid(tiles[y-1][x])){
                Box(x+.5f,yy-.045f,.015f,1.01f,.09f,.22f,Shade(c,1.4f));
                if(city){Box(x+.5f,yy-.19f,.09f,.92f,.06f,.22f,Shade(STONE,.83f));}
                else if(R(x+2,y)>.55f){Foliage(x+.5f,yy,-.7f,.34f,x+y,0);}
            }
            if(city && x%3==0 && y%2==0)MetalBox(x+.5f,yy-.5f,.03f,.035f,.62f,.04f,Shade(COPPER,.7f));
            if(t==T_VEIN){
                Color light=city?COOL:WARM;
                if(city){MetalBox(x+.5f,yy-.37f,.12f,.58f,.58f,.18f,BRONZE);Box(x+.5f,yy-.37f,.23f,.39f,.4f,.06f,COPPER);}
                for(int i=0;i<3;i++)Ellipse(x+.32f+i*.15f,yy-.45f+sinf(i*2.f)*.13f,.3f,.04f,.12f,.02f,light,1.1f);
            }
        }else if(TileOneWay(t)){
            int grate=(roomIdx==0&&y==20&&x>=21&&x<=26)||(roomIdx==1&&y==1&&x>=20&&x<=25);
            Color c=city?STONE:(Color){112,95,70,255};
            Box(x+.5f,yy-.105f,-.48f,1.02f,.21f,1.1f,c);
            Box(x+.5f,yy-.025f,.035f,1.02f,.05f,.17f,Shade(c,1.38f));
            if(city){Box(x+.5f,yy-.25f,-.05f,.9f,.12f,.65f,Shade(c,.78f));for(int i=0;i<3;i++)Box(x+.17f+i*.33f,yy-.37f,-.06f,.13f,.16f,.45f,c);}
            if(grate){for(int i=0;i<3;i++)MetalBox(x+.14f+i*.35f,yy+.025f,-.2f,.045f,.07f,1.3f,COPPER);}
        }else if(t==T_BUSH){Foliage(x+.5f,yy-1,.15f,1.1f,x+y,bushShake[y][x]/14.f);}
        else if(t==T_MOSS){
            for(int i=0;i<3;i++){float len=.5f+R(x+i,y);Rod((Vector3){x+.2f+i*.24f,yy,-.3f},(Vector3){x+.3f+i*.24f,yy-len,-.25f},.025f,(Color){56,92, 72,255},0);}
        }
    }
}

static void Lamp(float x,float y,float z,float size,int city,float phase){
    if(!city){
        for(int i=0;i<FOUNDRY_HUNTER_LANTERN.mesh_count;i++){
            const FoundryMeshData *m=&FOUNDRY_HUNTER_LANTERN.meshes[i];Color c={m->color[0],m->color[1],m->color[2],m->color[3]};
            float glow=m->emission;
            if(glow>0){if(phase < -2){glow=0;c=(Color){29,33,32,255};}else{c=WARM;glow*=.95f+phase*.08f;}}
            Draw(&lampModels[i],(Vector3){x,y,z},(Vector3){size,size,size},(Vector3){0,1,0},0,c,m->metallic,m->roughness,glow);
        }return;
    }
    Color light=city?COOL:WARM;
    MetalBox(x,y+size*.27f,z,size*.48f,size*.13f,size*.36f,Shade(BRONZE,.7f));
    MetalBox(x,y-size*.28f,z,size*.54f,size*.13f,size*.38f,Shade(BRONZE,.7f));
    for(int i=-1;i<=1;i+=2)Rod((Vector3){x+i*size*.21f,y-size*.25f,z+.15f},(Vector3){x+i*size*.21f,y+size*.25f,z+.15f},size*.025f,BRONZE,1);
    Ellipse(x,y,z,size*.17f,size*.23f,size*.12f,light,.8f+phase*.15f);
    Ring(x,y+size*.44f,z,size*.12f,BRONZE,(Vector3){0,1,0},0);
}
static void PlayerMesh(void){
    float x=(player.x+player.w*.5f)/TS,base=Y(player.y+player.h);
    float h=player.h/(float)TS,w=player.w/(float)TS;
    float bob=player.onGround&&fabsf(player.vx)>.05f?((int)player.animT&1)*.065f:0;
    Color body=palSkin; // Only the actual submerged pixels are tinted by the water pass.
    // Rigid dimensions and lagging nubs retain L10. No body squash or stretch.
    AssetPose(&FOUNDRY_PLAYER_SEED,seedModels,(Vector3){x,base-bob,0},1,(Vector3){0,0,1},-player.leanX*1.8f,.22f);
    for(int i=-1;i<=1;i+=2){
        float ex=x+i*w*.28f+player.leanX*.025f;
        Ellipse(ex,base+h+.025f,.01f,.06f,.105f,.065f,body,0);
        if(player.blink>=5 && resetFade<.85f){
            float ey=base+h*.73f;
            Ellipse(ex+player.facing*.035f,ey-player.leanY*.025f,.30f,.062f,resetFade>.45f?.018f:.092f,.025f,(Color){13,25,28,255},0);
            if(resetFade<.45f)Ellipse(ex+.014f+player.facing*.035f,ey+.038f,.34f,.023f,.024f,.015f,(Color){239,250,232,255},.95f);
        }
    }
    float stride=sinf(player.animT*PI_F)*.09f*(player.onGround?fabsf(player.vx)/1.45f:0);
    Ellipse(x-.19f,base+.065f+stride,.06f,.17f,.08f,.22f,Shade(body,.8f),0);
    Ellipse(x+.19f,base+.065f-stride,.06f,.17f,.08f,.22f,Shade(body,.8f),0);
}
static void ItemsMesh(void){
    for(int i=0;i<itemCount;i++){
        Item *it=&items[i];if(i!=heldItem&&it->room!=roomIdx)continue;
        float x=(it->x+(it->kind==IT_LAMP?2.f:2.5f))/TS,y=Y(it->y+(it->kind==IT_LAMP?2.5f:2.f));
        if(it->kind==IT_LAMP)Lamp(x,y,0.35f,1.f,0,it->flick);
        else{Draw(&orb,(Vector3){x,y,.22f},(Vector3){.30f,.22f,.25f},(Vector3){0,0,1},27, palStone,0,.93f,0);}
    }
}
static void BulbMeshes(void){
    for(int i=0;i<bulbCount;i++){
        Bulb *b=&bulbs[i];float x=b->x/(float)TS,y=Y(b->y);
        int press=b->squash>0?(b->squash>5?2:1):0;
        float height=(BULB_H-press)/(float)TS;
        for(int j=0;j<7;j++){
            float a=j*PI_F*2/7;
            Draw(&leaf,(Vector3){x+cosf(a)*.3f,y,-.1f+sinf(a)*.25f},(Vector3){.55f,.7f,.7f},(Vector3){0,0,1},a*RAD2DEG,(Color){73,119,99,255},0,.7f,0);
        }
        Ellipse(x,y+height*.5f,.01f,.73f,height*.5f,.6f,palBulb,b->flash>0?.48f:.03f);
        Ellipse(x-.13f,y+height-.045f,.19f,.16f,.04f,.13f,palBulbLit,b->timed?.7f:.1f);
    }
}

static void Door(float x,float top,float phase,int lit){
    float y=top-6,z=-.48f,cx=x+2.5f,cy=y+2.7f;
    Asset(&FOUNDRY_VAULT_DOOR,doorModels,(Vector3){cx,y,-1.1f},1);
    if(lit){float r=.25f+phase*.11875f;Ellipse(cx+cosf(phase)*r,cy+sinf(phase)*r,z+.07f,.05f,.05f,.03f,COOL,.6f);}
}
static void PropMeshes(int back){
    PropView pp[96];int count=PropsViews(pp,96);
    for(int i=0;i<count;i++){
        PropView *p=&pp[i];float x=p->tx,y=22-p->ty,z=.15f;
        if(back){
            if(p->kind==PR_DOOR)Door(x,y,p->timer*(14.2f/240.f),p->state==1);
            if(p->kind==PR_LINTEL)Box(x+2,y-.25f,-.3f,4.1f,.5f,.5f,Shade(STONE,.7f));
            continue;
        }
        switch(p->kind){
        case PR_ROPE:case PR_ROOT:case PR_CHAINLAMP:{
            float len=p->len,dx=sinf(p->angle)*len,dy=cosf(p->angle)*len;
            Color color=p->kind==PR_ROOT?(Color){74,107,79,255}:p->kind==PR_ROPE?palRope:BRONZE;
            if(p->kind==PR_CHAINLAMP){
                for(float k=0;k<len-.4f;k+=.17f)Ring(x+.5f+sinf(p->angle)*k,y-cosf(p->angle)*k,-.1f,.07f,color,(Vector3){0,1,0},((int)(k*6)&1)*70.f);
                Lamp(x+.5f+dx,y-dy+.25f,-.03f,.75f,1,sinf(frameNo*.03f));
            }else{
                Vector3 last={x+.5f,y,z};
                for(int j=1;j<=10;j++){
                    float f=j/10.f;Vector3 q={x+.5f+dx*f,y-dy*f,z+sinf(f*PI_F)*.06f};Rod(last,q,p->kind==PR_ROOT?.055f:.035f,color,0);last=q;
                    if(p->kind==PR_ROOT&&j%3==1)Draw(&leaf,q,(Vector3){.3f,.4f,.4f},(Vector3){0,0,1},45*((j&1)?1:-1),color,0,.8f,0);
                }
            }break;
        }
        case PR_BEDROLL:{float dent=p->state?.07f:0;Draw(&cube,(Vector3){x+1,y-.85f,.35f},(Vector3){1.8f,.22f-dent,.72f},(Vector3){0,0,1},0,palLedge,0,.98f,0);Ellipse(x+.2f,y-.73f,.4f,.22f,.2f,.4f,palLedgeLit,0);break;}
        case PR_FIRE:
            for(int k=0;k<7;k++){float a=k*PI_F*2/7;Ellipse(x+.5f+cosf(a)*.38f,y-.89f,.2f+sinf(a)*.24f,.13f,.12f,.15f,palStone,0);}
            for(int k=0;k<3;k++)Rod((Vector3){x+.2f,y-.85f+k*.04f,.07f+k*.15f},(Vector3){x+.8f,y-.85f+k*.04f,.45f-k*.14f},.07f,(Color){49,37,31,255},0);
            if(p->fireLit)for(int k=0;k<3;k++){float h=.26f+sinf(p->phase*(1+.3f*k)+k*2.1f)*.12f;Ellipse(x+.3f+k*.19f,y-.75f+h*.5f,.23f,.075f,h,.07f,WARM,.9f);}
            break;
        case PR_PACK:{
            Box(x+.3f,y-.65f,-.08f,.58f,.65f,.38f,palRope);MetalBox(x+.3f,y-.65f,.14f,.04f,.6f,.045f,BRONZE);Lamp(x+.78f,y-.78f,.2f,.38f,0,-7);
            float lx,ly;
            if(LampPos(&lx,&ly)&&fabsf(lx-(p->tx*TS+6))<40&&fabsf(ly-(p->ty*TS+5))<24&&((frameNo/3)&3)==0)
                Ellipse(x+.78f,y-.68f,.3f,.025f,.035f,.015f,palLampGlass,.7f);
            break;}
        case PR_CAIRN:
            for(int k=0;k<4;k++)Ellipse(x+.5f+(R(k,17)-.5f)*.12f,y-.91f+k*.12f,.2f,.34f-k*.065f,.085f,.25f-k*.04f,Shade(palStone,1+k*.06f),0);break;
        case PR_BONES:{
            float tilt=p->timer>0?sinf(p->timer*.5f)*16:0;
            Draw(&orb,(Vector3){x+.38f,y-.61f,.28f},(Vector3){.19f,.21f,.17f},(Vector3){0,0,1},tilt,palBone,0,.8f,0);
            for(int k=0;k<4;k++)Rod((Vector3){x+.35f+k*.09f,y-.88f,.25f},(Vector3){x+.42f+k*.09f,y-.72f,.25f},.025f,palBone,0);break;}
        case PR_POT:{
            if(p->state==POT_GONE)break;
            float px=p->state==POT_FALLING?p->x/TS+.3f:x+.4f,py=p->state==POT_FALLING?Y(p->y)-.4f:y-.6f;
            float rock=p->timer>0?sinf(p->timer*.7f)*.04f:0;
            Asset(&FOUNDRY_POT,potModels,(Vector3){px+rock,py-.36f,.28f},1);break;
        }
        case PR_BANNER:
            MetalBox(x+.42f,y,.14f,.7f,.08f,.1f,BRONZE);
            for(int j=0;j<16;j++){float f=j/16.f,wave=sinf(p->phase+f*9)*p->angle*.11f*f;Box(x+.43f+wave,y-f*p->len-.1f,.16f,.62f,p->len/16.f+.015f,.026f,j%5==2?palClothLit:palCloth);}
            break;
        case PR_BALUSTRADE:
            for(int k=0;k<4;k++)Rod((Vector3){x+k*.28f,y,.04f},(Vector3){x+k*.28f,y+.5f,.04f},.045f,STONE,0);
            Box(x+.45f,y+.53f,.04f,1.2f,.13f,.22f,STONE);break;
        case PR_CAPITAL:Box(x+p->len*.5f,y-.13f,.08f,p->len+.24f,.26f,.3f,Shade(STONE,1.3f));break;
        case PR_BASE:Box(x+p->len*.5f,y-.8f,.06f,p->len+.24f,.3f,.3f,Shade(STONE,1.15f));break;
        default:break;
        }
    }
}
static void LifeMeshes(void){
    BirdView birds[4];int n=LifeBirdViews(birds,4);
    for(int i=0;i<n;i++){
        BirdView *b=&birds[i];float x=b->x/TS,y=Y(b->y),f=b->facing;
        AssetPose(&FOUNDRY_BIRD_BODY,birdModels,(Vector3){x,y+.22f,.02f},1,(Vector3){0,1,0},f<0?180:0,0);
        Ellipse(x+f*.24f,y+.38f,.14f,.028f,.03f,.018f,COOL,.45f);
        Rod((Vector3){x+f*.3f,y+.31f,.01f},(Vector3){x+f*.39f,y+.29f,.01f},.035f,BRONZE,0);
        float a=b->state==B_FLY?sinf(b->flap*.8f)*55.f:12.f;
        Draw(&leaf,(Vector3){x,y+.17f,.13f},(Vector3){.4f,.5f,.4f},(Vector3){0,0,1},90+a,palBird,0,.7f,0);
    }
    BeastView b;if(LifeBeastView(&b)){
        float x=b.x/TS+.65f,y=Y(b.y)-(b.state==M_SIT?.12f:0),f=b.dir;
        AssetPose(&FOUNDRY_BEAST_BODY,beastBodyModels,(Vector3){x,y+.45f,.1f},1,(Vector3){0,1,0},f<0?180:0,0);
        for(int i=0;i<4;i++){if(b.state==M_SIT&&i>=2)continue;float step=b.state==M_WALK?sinf(b.legT*3+i*PI_F)*.08f:0;Rod((Vector3){x-.5f+i*.32f,y+.4f,.18f},(Vector3){x-.5f+i*.32f+step,y+.04f,.2f},.06f,palFurLight,0);}
        AssetPose(&FOUNDRY_BEAST_HEAD,beastHeadModels,(Vector3){x+f*.72f,y+.63f+b.headLift*.12f,.11f},1,(Vector3){0,1,0},f<0?180:0,0);
        if(b.blink>=5)Ellipse(x+f*.77f,y+.68f+b.headLift*.12f,.29f,.035f,.036f,.025f,COOL,.65f);
        Vector3 last={x-f*.65f,y+.5f,.1f};for(int i=0;i<5;i++){Vector3 q={x-f*(.8f+i*.13f),y+.55f+b.tail[i]*.08f+i*.04f,.1f};Rod(last,q,.07f-i*.01f,palFur,0);last=q;}
    }
    PlantView plants[4];n=LifePlantViews(plants,4);
    for(int i=0;i<n;i++){
        PlantView *p=&plants[i];Vector3 root={p->x/TS,Y(p->y),.01f};
        for(int j=0;j<3;j++){
            Vector3 pod={p->podX[j]/TS,Y(p->podY[j]),.04f};Rod(root,pod,.04f,palStalk,0);
            int speaking=p->mouth>0&&p->pod==j;
            Ellipse(pod.x,pod.y,pod.z,.19f,.25f,.17f,palPod,speaking?.5f:.08f);
            Ellipse(pod.x,pod.y-.05f,.21f,.07f,speaking?.09f:.015f,.01f,palPodDeep,0);
            Draw(&leaf,Vector3Lerp(root,pod,.5f),(Vector3){.6f,.8f,.4f},(Vector3){0,0,1},j==0?55:-55,palLeaf,0,.9f,0);
        }
    }
}
static void Water(void){
    // Transparent front pane follows the real water extent; it preserves submerged
    // silhouette and object positions instead of using a second visual simulation.
    if(roomIdx!=1)return;
    rlDisableBackfaceCulling();rlDisableDepthMask();
    for(int x=1;x<RW-1;x++){
        int top=-1;for(int y=0;y<RH;y++)if(TileWater(tiles[y][x])){top=y;break;}
        if(top<0)continue;
        float height=22-top-RoomWaterHeight(x)*3/TS;
        Draw(&cube,(Vector3){x+.5f,height,.51f},(Vector3){1.02f,.055f,.035f},(Vector3){0,0,1},0,(Color){154,216,202,230},.12f,.2f,.7f);
        float t=depthStill?0:frameNo*DT;
        for(int k=0;k<2;k++){
            float xx=x+R(x,k),yy=height-.12f-k*.25f;
            if(sinf(t*.6f+x+k)>0.3f)Box(xx,yy,.55f,.14f+.24f*R(x,k+3),.009f,.01f,(Color){83,147,141,130});
        }
    }
    rlEnableDepthMask();rlEnableBackfaceCulling();
}
static void Atmosphere(void){
    float t=depthStill?0:frameNo*DT;
    for(int i=0;i<34;i++){
        float x=1+R(i,74)*38+sinf(t*.2f+i)*.15f,y=1+R(i,75)*20+sinf(t*.16f+i*3)*.22f;
        Ellipse(x,y,-2-R(i,76)*6,.015f,.015f,.015f,Shade(COOL,.6f),.2f);
    }
    // Foreground is restricted to the outer frame, so silhouettes never conceal a
    // playable landing. Its dark, larger leaves complete the multiplane staging.
    for(int i=0;i<8;i++){
        float x=i<4?-.8f:40.8f,y=(i%4)*6.f;
        Foliage(x,y,3,1.7f,i+91,0);
    }
}
static void ResponseParticles(void){
    Particle particles[FX_MAX];int n=FxViews(particles,FX_MAX);
    for(int i=0;i<n;i++){
        Particle *p=&particles[i];float t=p->maxLife?p->life/(float)p->maxLife:0;
        Color c=palDrop;float sx=.035f,sy=.04f,glow=0;
        switch(p->kind){
        case FX_DRIP:sy=.10f;break;
        case FX_SPLASH:break;
        case FX_DUST:c=(Color){116,111,122,255};sx=.035f+.025f*(1-t);sy=sx;break;
        case FX_SPARK:c=t>.45f?palFlameHot:palEmber;glow=.6f;break;
        case FX_SHARD:c=palClay;sx=.065f;break;
        default:continue;
        }
        Ellipse(p->x/TS,Y(p->y),.62f,sx,sy,.025f,c,glow);
    }
}
static void ResetEyes(float left,float top,float width,float height){
    if(resetFade<=0||resetFade>.85f||player.blink<5)return;
    float x=(player.x+player.w*.5f)/TS,base=Y(player.y+player.h),h=player.h/(float)TS,w=player.w/(float)TS;
    for(int i=-1;i<=1;i+=2){
        float ex=x+i*w*.28f+player.leanX*.025f+player.facing*.035f;
        Vector2 p=GetWorldToScreenEx((Vector3){ex,base+h*.73f-player.leanY*.025f,.34f},camera,DW,DH);
        int px=(int)(left+p.x*width/DW),py=(int)(top+p.y*height/DH);
        DrawEllipse(px,py,width*.062f/40.f,height*(resetFade>.45f?.018f:.092f)/22.5f,palPupil);
        if(resetFade<=.45f)DrawEllipse(px,py-(int)(height*.035f/22.5f),width*.023f/40.f,height*.024f/22.5f,palEye);
    }
}
void DepthDraw(void){
    if(!ready)Init();
    Color lightPixels[(RW+1)*(RH+1)];RoomDepthLightColors(lightPixels);UpdateTexture(depthLight,lightPixels);
    float eye[3]={camera.position.x,camera.position.y,camera.position.z};SetShaderValue(surface,cameraLoc,eye,SHADER_UNIFORM_VEC3);
    BeginTextureMode(target);ClearBackground((Color){10,24,30,255});
    BeginMode3D(camera);
        Background();PropMeshes(1);Terrain();PropMeshes(0);BulbMeshes();LifeMeshes();ItemsMesh();PlayerMesh();Atmosphere();Water();ResponseParticles();
    EndMode3D();
    EndTextureMode();
    Texture2D present=target.texture;
    if(roomIdx==1){
        Color maskPixels[RW*RH];for(int y=0;y<RH;y++)for(int x=0;x<RW;x++)maskPixels[y*RW+x]=TileWater(tiles[y][x])?WHITE:BLACK;
        UpdateTexture(waterMask,maskPixels);
        float waterTime=depthStill?0:frameNo*DT;
        SetShaderValue(waterShader,GetShaderLocation(waterShader,"waterTime"),&waterTime,SHADER_UNIFORM_FLOAT);
        BeginTextureMode(waterTarget);ClearBackground(BLACK);BeginShaderMode(waterShader);
        SetShaderValueTexture(waterShader,GetShaderLocation(waterShader,"mask"),waterMask);
        DrawTexturePro(target.texture,(Rectangle){0,0,DW,-DH},(Rectangle){0,0,DW,DH},(Vector2){0,0},0,WHITE);
        EndShaderMode();EndTextureMode();present=waterTarget.texture;
    }
    BeginDrawing();ClearBackground(BLACK);
    float scale=fminf(GetScreenWidth()/(float)DW,GetScreenHeight()/(float)DH),w=DW*scale,h=DH*scale;
    float t=frameNo*DT;SetShaderValue(finish,timeLoc,&t,SHADER_UNIFORM_FLOAT);
    BeginShaderMode(finish);DrawTexturePro(present,(Rectangle){0,0,DW,-DH},(Rectangle){(GetScreenWidth()-w)*.5f,(GetScreenHeight()-h)*.5f,w,h},(Vector2){0,0},0,WHITE);EndShaderMode();
    if(resetFade>0)DrawRectangle(0,0,GetScreenWidth(),GetScreenHeight(),(Color){0,0,0,(u8)(Clamp(resetFade,0,1)*255)});
    ResetEyes((GetScreenWidth()-w)*.5f,(GetScreenHeight()-h)*.5f,w,h);
    EndDrawing();
}
void DepthUnload(void){
    if(!ready)return;
    // The shared shader and light texture are borrowed. UnloadModel only disposes
    // mesh/material arrays; shared shaders and presentation textures are owned here.
    UnloadModel(cube);UnloadModel(orb);UnloadModel(cylinder);UnloadModel(cone);UnloadModel(torus);UnloadModel(leaf);
    for(int i=0;i<FOUNDRY_VAULT_DOOR.mesh_count;i++)UnloadModel(doorModels[i]);
    for(int i=0;i<FOUNDRY_ORRERY.mesh_count;i++)UnloadModel(orreryModels[i]);
    for(int i=0;i<FOUNDRY_VAULT_TERRAIN.mesh_count;i++)UnloadModel(terrainModels[0][i]);
    for(int i=0;i<FOUNDRY_DROWNED_TERRAIN.mesh_count;i++)UnloadModel(terrainModels[1][i]);
    const FoundryAssetData *lifeAssets[]={&FOUNDRY_POT,&FOUNDRY_HUNTER_LANTERN,&FOUNDRY_PLAYER_SEED,&FOUNDRY_BEAST_BODY,&FOUNDRY_BEAST_HEAD,&FOUNDRY_BIRD_BODY};
    Model *lifeModels[]={potModels,lampModels,seedModels,beastBodyModels,beastHeadModels,birdModels};
    for(int a=0;a<6;a++)for(int i=0;i<lifeAssets[a]->mesh_count;i++)UnloadModel(lifeModels[a][i]);
    UnloadShader(surface);UnloadShader(finish);UnloadRenderTexture(target);ready=0;
    UnloadShader(waterShader);UnloadRenderTexture(waterTarget);UnloadTexture(waterMask);
    UnloadTexture(depthLight);
    for(int i=0;i<2;i++)if(mattes[i].id)UnloadTexture(mattes[i]);
}
