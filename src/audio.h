#ifndef AW_AUDIO_H
#define AW_AUDIO_H
#include <math.h>

enum { HVOICE_NONE, HVOICE_GREETING, HVOICE_OFFER, HVOICE_TAKEN,
       HVOICE_FIRE, HVOICE_BEDROLL, HVOICE_HUM, HVOICE_COUNT };
typedef struct {
    int kind, syllables;
    float pitch, volume, pan;
    unsigned sequence;
} HunterVoiceEvent;

// The source sample timeline is shared by startup synthesis and simulation.
// Millisecond durations are converted to integer PCM sample boundaries once by
// these pure helpers. No device, global state or random stream is consulted.
#define HUNTER_VOICE_SR 22050
typedef struct {
    int syllables, leadMs, voicedMs[3], gapMs[2], breathMs, tailMs;
} HunterPhrase;
typedef struct { float open; int syllable; } HunterMouth;
static inline const HunterPhrase *AudioHunterPhrase(int kind) {
    static const HunterPhrase phrase[HVOICE_COUNT] = {
        {0,0,{0,0,0},{0,0},0,0},
        {1,25,{145,0,0},{0,0},45,115},
        {3,35,{155,140,185},{65,85},60,160},
        {1,10,{110,0,0},{0,0},40,120},
        {3,60,{220,240,300},{100,115},120,200},
        {1,30,{150,0,0},{0,0},60,125},
        {1,35,{330,0,0},{0,0},65,160}
    };
    return &phrase[kind>HVOICE_NONE&&kind<HVOICE_COUNT?kind:HVOICE_NONE];
}
static inline int AudioHunterSamplesMs(int ms) { return ms*HUNTER_VOICE_SR/1000; }
static inline float AudioHunterPitch(float pitch) {
    return !isfinite(pitch)?1.0f:(pitch<.5f?.5f:(pitch>1.5f?1.5f:pitch));
}
static inline int AudioHunterSyllableStart(int kind,int syllable) {
    const HunterPhrase *p=AudioHunterPhrase(kind);
    int at=AudioHunterSamplesMs(p->leadMs);
    for(int k=0;k<syllable&&k<p->syllables;k++)
        at+=AudioHunterSamplesMs(p->voicedMs[k])+(k<2?AudioHunterSamplesMs(p->gapMs[k]):0);
    return at;
}
static inline int AudioHunterVoicedEnd(int kind) {
    const HunterPhrase *p=AudioHunterPhrase(kind);
    return p->syllables?AudioHunterSyllableStart(kind,p->syllables-1)+AudioHunterSamplesMs(p->voicedMs[p->syllables-1]):0;
}
static inline int AudioHunterSampleCount(int kind) {
    const HunterPhrase *p=AudioHunterPhrase(kind);
    return p->syllables?AudioHunterVoicedEnd(kind)+AudioHunterSamplesMs(p->breathMs+p->tailMs):0;
}
static inline int AudioHunterDurationTicks(int kind,float pitch) {
    return (int)ceil((double)AudioHunterSampleCount(kind)*60.0/(HUNTER_VOICE_SR*(double)AudioHunterPitch(pitch)));
}
// This is the actual dry voiced amplitude envelope used by the synthesizer.
// Outside voiced spans, including breath and reverberation, the mouth is closed.
static inline HunterMouth AudioHunterMouthSample(int kind,int sample) {
    const HunterPhrase *p=AudioHunterPhrase(kind);
    for(int k=0;k<p->syllables;k++) {
        int begin=AudioHunterSyllableStart(kind,k),length=AudioHunterSamplesMs(p->voicedMs[k]);
        if(sample>=begin&&sample<begin+length) {
            float t=(float)(sample-begin)/(length-1);
            float envelope=sinf(3.14159265f*t);
            return (HunterMouth){envelope>0?envelope:0,k};
        }
    }
    return (HunterMouth){0,-1};
}
static inline HunterMouth AudioHunterMouthAt(int kind,float pitch,int elapsedTicks) {
    if(elapsedTicks<0||elapsedTicks>=AudioHunterDurationTicks(kind,pitch))return (HunterMouth){0,-1};
    double position=(double)elapsedTicks*HUNTER_VOICE_SR*AudioHunterPitch(pitch)/60.0;
    HunterMouth m=AudioHunterMouthSample(kind,(int)position);
    if(kind==HVOICE_HUM)m.open*=.16f; // closed-lip hum; the jaw barely moves
    return m;
}

// Call Init after AudioInit, also in mute/headless mode. One hunter, one voice.
// Play returns pitch-adjusted total ticks, or zero for an invalid event.
void AudioHunterInit(void);
int AudioHunterPlay(const HunterVoiceEvent *event);
void AudioHunterStop(void);
void AudioHunterClose(void);
int AudioHunterCounts(unsigned *out,int max);
void AudioHunterPrintStats(void);
// Separate listening artifact, with the authored pitch and caller levels.
int AudioHunterExportMontage(const char *path);
#endif
