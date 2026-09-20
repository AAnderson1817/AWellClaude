// Real synthesis with only device I/O replaced. White-box bank access lets this
// test compare actual PCM and the inherited pool/RNG, not just helper outputs.
#include "../../src/aw.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
static int loaded,played,stopped,unloaded;
static float lastPitch,lastVolume,lastPan;
static Sound ProbeLoad(Wave w){loaded++;return (Sound){.frameCount=w.frameCount};}
static void ProbePlay(Sound s){assert(s.frameCount>0);played++;}
static void ProbeStop(Sound s){assert(s.frameCount>0);stopped++;}
static void ProbeUnload(Sound s){assert(s.frameCount>0);unloaded++;}
static void ProbePitch(Sound s,float value){lastPitch=value;}
static void ProbeVolume(Sound s,float value){lastVolume=value;}
static void ProbePan(Sound s,float value){lastPan=value;}
#define LoadSoundFromWave ProbeLoad
#define PlaySound ProbePlay
#define StopSound ProbeStop
#define UnloadSound ProbeUnload
#define SetSoundPitch ProbePitch
#define SetSoundVolume ProbeVolume
#define SetSoundPan ProbePan
#include "../../src/audio.c"
#undef LoadSoundFromWave
#undef PlaySound
#undef StopSound
#undef UnloadSound
#undef SetSoundPitch
#undef SetSoundVolume
#undef SetSoundPan
Player player;
int roomIdx;
float resetFade;

static uint64_t Hash(const void *data,size_t size){
    uint64_t h=1469598103934665603ULL;const unsigned char *p=data;
    for(size_t i=0;i<size;i++){h^=p[i];h*=1099511628211ULL;}return h;
}
static void Wav(const char *directory,int kind,float pitch,float volume){
    char path[1024];snprintf(path,sizeof path,"%s/hunter-%d.wav",directory,kind);
    FILE *f=fopen(path,"wb");assert(f);
    HunterSound *s=&hunterSounds[kind];unsigned n=(unsigned)ceil(s->samples/(double)pitch);
    unsigned data=n*2,size=36+data,rate=HUNTER_VOICE_SR,byterate=rate*2,fmt=16;
    unsigned short one=1,bits=16,align=2;
    fwrite("RIFF",1,4,f);fwrite(&size,4,1,f);fwrite("WAVEfmt ",1,8,f);fwrite(&fmt,4,1,f);
    fwrite(&one,2,1,f);fwrite(&one,2,1,f);fwrite(&rate,4,1,f);fwrite(&byterate,4,1,f);
    fwrite(&align,2,1,f);fwrite(&bits,2,1,f);fwrite("data",1,4,f);fwrite(&data,4,1,f);
    for(unsigned i=0;i<n;i++){
        double at=i*(double)pitch;int j=(int)at;float blend=(float)(at-j);
        float a=j<s->samples?s->pcm[j]:0,b=j+1<s->samples?s->pcm[j+1]:0;
        i16 v=(i16)((a+(b-a)*blend)*hunterBase*volume);fwrite(&v,2,1,f);
    }
    assert(!fclose(f));
}
int main(int argc,char **argv){
    AudioInit(1);assert(!ready);
    uint64_t oldPool=Hash(pool,(size_t)poolUsed*sizeof *pool);
    u32 oldRng=rng;int oldPoolUsed=poolUsed;const char *oldLast=dbgLastSfx;
    int oldCounts[SFX_COUNT];memcpy(oldCounts,sfxCount,sizeof oldCounts);
    // Muted synthesis still exists and records the same authored events.
    AudioHunterInit();assert(!loaded);
    HunterVoiceEvent event={HVOICE_FIRE,3,.72f,.20f,.35f,1};
    assert(AudioHunterPlay(&event)>100&&!played);
    AudioHunterClose();assert(!unloaded);
    ready=1;AudioHunterInit();assert(loaded==6);AudioHunterInit();assert(loaded==6);
    static const int milliseconds[HVOICE_COUNT]={0,330,885,280,1355,365,590};
    int poseChecks=0;
    for(int kind=1;kind<HVOICE_COUNT;kind++){
        HunterSound *s=&hunterSounds[kind];
        assert(fabs((double)s->samples/HUNTER_VOICE_SR-milliseconds[kind]/1000.0)<.00025);
        assert(s->samples>0&&s->samples<HUNTER_PCM_MAX);
        assert(!s->pcm[0]&&!s->pcm[s->samples-1]);
        double sum=0,squares=0,peak=0;
        for(int i=0;i<s->samples;i++){
            double v=s->pcm[i]/32768.;sum+=v;squares+=v*v;if(fabs(v)>peak)peak=fabs(v);
            assert(s->pcm[i]!=32767&&s->pcm[i]!=-32768);
        }
        double rms=sqrt(squares/s->samples);assert(peak>.1&&peak<.8&&rms>.025&&rms<.3&&fabs(sum/s->samples)<.006);
        printf("PCM kind=%d samples=%d peak=%.8f rms=%.8f dc=%.8f hash=%016llx\n",kind,s->samples,peak,rms,sum/s->samples,(unsigned long long)Hash(s->pcm,s->samples*sizeof *s->pcm));
        for(int hundredths=50;hundredths<=150;hundredths++){
            float pitch=hundredths/100.f;
            int duration=AudioHunterDurationTicks(kind,pitch);
            double exact=s->samples/(HUNTER_VOICE_SR*(double)pitch);
            assert(duration/60.0>=exact&&(duration-1)/60.0<exact);
            for(int frame=0;frame<=duration;frame++){
                HunterMouth m=AudioHunterMouthAt(kind,pitch,frame);
                assert(isfinite(m.open)&&m.open>=0&&m.open<=1);
                // Independently locate each playback frame against the authored
                // source boundaries. Breath, phrase pauses and tail stay closed.
                double sourceSeconds=frame*(double)pitch/60;
                const HunterPhrase *p=AudioHunterPhrase(kind);double at=p->leadMs/1000.;int voiced=0;
                for(int k=0;k<p->syllables;k++){
                    double end=at+p->voicedMs[k]/1000.;
                    if(sourceSeconds>at+.0003&&sourceSeconds<end-.0003)voiced=1;
                    at=end+(k<2?p->gapMs[k]/1000.:0);
                }
                if(m.open>.01)assert(voiced||fabs(sourceSeconds-at)<.0003);
                if(frame==duration)assert(m.open==0&&m.syllable==-1);
                poseChecks++;
            }
        }
        float pitch=kind==HVOICE_FIRE?.72f:(kind==HVOICE_TAKEN?1.18f:(kind==HVOICE_HUM?.83f:1));
        event=(HunterVoiceEvent){kind,AudioHunterPhrase(kind)->syllables,pitch,kind==HVOICE_HUM?.12f:.20f,.35f,(unsigned)kind};
        assert(AudioHunterPlay(&event)==AudioHunterDurationTicks(kind,pitch));
        assert(lastPitch==pitch&&lastPan==.35f&&fabsf(lastVolume-hunterBase*event.volume)<1e-7f);
        if(argc>1)Wav(argv[1],kind,pitch,event.volume);
    }
    assert(played==6&&stopped==5);
    assert(AudioHunterPlay(NULL)==0);event.kind=HVOICE_COUNT;assert(!AudioHunterPlay(&event));
    event.kind=HVOICE_GREETING;event.pitch=NAN;event.volume=INFINITY;event.pan=NAN;
    assert(AudioHunterPlay(&event)==AudioHunterDurationTicks(event.kind,1));
    assert(lastPitch==1&&lastVolume==hunterBase*.20f&&lastPan==.5f);
    unsigned counts[HVOICE_COUNT];assert(AudioHunterCounts(counts,HVOICE_COUNT)==HVOICE_COUNT);
    assert(counts[0]==0&&counts[HVOICE_GREETING]==2&&counts[HVOICE_FIRE]==1);
    assert(!AudioHunterCounts(NULL,2)&&!AudioHunterCounts(counts,0));
    AudioHunterStop();int stops=stopped;AudioHunterStop();assert(stopped==stops);
    AudioHunterClose();assert(unloaded==6);AudioHunterClose();assert(unloaded==6);
    assert(oldPoolUsed==poolUsed&&oldRng==rng&&oldLast==dbgLastSfx);
    assert(oldPool==Hash(pool,(size_t)poolUsed*sizeof *pool)&&!memcmp(oldCounts,sfxCount,sizeof oldCounts));
    printf("PASS %d pitch-scaled mouth/phrase samples; actual PCM bounds/DC/edges; muted and device lifecycle; original PCM/RNG/Sfx isolation\n",poseChecks);
    return 0;
}
