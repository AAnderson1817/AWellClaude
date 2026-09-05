// audio.c -- every sound in the game, made from arithmetic at startup. No files are
// read; there is nothing to stream. The palette is small and quiet on purpose: this is
// a place where a drip is an event.
#include "aw.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

#define SR   22050
#define PI_F 3.14159265f
#define AMB_VOL 0.26f          // was 0.55; the user heard it and it was too much

static int ready = 0;
const char *dbgLastSfx = "-";
int sfxCount[SFX_COUNT];

// ---------------------------------------------------------------- storage
// One static pool, carved once. Sounds own a slice; raylib copies it into the device.
#define POOL_MAX (SR * 24)
static i16 pool[POOL_MAX];
static int poolUsed;
#define WORK_MAX (SR * 7)
static float work[WORK_MAX], dry[WORK_MAX];

typedef struct {
    const char *name;
    i16  *pcm; int n;
    Sound snd, alias[4]; int nAlias, rr;
    float base;                 // the sound's own level, before the caller's volume
} SfxDef;
static SfxDef sfx[SFX_COUNT];
static Sound amb[ROOM_COUNT]; static int ambRoom = -1, ambLen[ROOM_COUNT];
static i16 *ambPcm[ROOM_COUNT];

static u32 rng = 0xC0FFEE11u;
static float Rnd(void) {                       // -1..1
    rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
    return (float)(rng & 0xFFFFFFu) / 8388607.5f - 1.0f;
}

// ---------------------------------------------------------------- building blocks
static void Clear(int n) { memset(work, 0, sizeof(float) * n); }
static void Noise(int n, float amp) { for (int i = 0; i < n; i++) work[i] += Rnd() * amp; }
static void Sine(int n, float f0, float f1, float tSweep, float amp) {
    // frequency glides from f0 to f1 over tSweep seconds, then holds
    float ph = 0;
    for (int i = 0; i < n; i++) {
        float t = (float)i / SR, k = tSweep > 0 ? (t < tSweep ? t / tSweep : 1.0f) : 1.0f;
        float f = f0 + (f1 - f0) * k;
        ph += 2 * PI_F * f / SR;
        work[i] += sinf(ph) * amp;
    }
}
static void Env(int n, float attack, float tau) {
    for (int i = 0; i < n; i++) {
        float t = (float)i / SR;
        work[i] *= (attack > 0 ? 1.0f - expf(-t / attack) : 1.0f) * expf(-t / tau);
    }
}
static void LowPass(int n, float f0, float f1, float tSweep) {
    float y = 0;
    for (int i = 0; i < n; i++) {
        float t = (float)i / SR, k = tSweep > 0 ? (t < tSweep ? t / tSweep : 1.0f) : 1.0f;
        float a = expf(-2 * PI_F * (f0 + (f1 - f0) * k) / SR);
        y = a * y + (1 - a) * work[i]; work[i] = y;
    }
}
static void HighPass(int n, float fc) {
    float a = expf(-2 * PI_F * fc / SR), y = 0;
    for (int i = 0; i < n; i++) { y = a * y + (1 - a) * work[i]; work[i] -= y; }
}
static void Soft(int n, float drive) { for (int i = 0; i < n; i++) work[i] = tanhf(work[i] * drive) / drive; }

// The cave. Four combs and two allpasses (Schroeder), damped, a little pre-delay.
// Applied once, at synthesis: what you hear is the room's answer, baked into the sound.
static void Reverb(int n, float wet, float fb, float damp) {
    static float c[4][1024], ap[2][128];
    static const int cl[4] = { 655, 818, 907, 964 }, al[2] = { 113, 37 };
    float lp[4] = { 0, 0, 0, 0 };
    memset(c, 0, sizeof c); memset(ap, 0, sizeof ap);
    memcpy(dry, work, sizeof(float) * n);
    int pre = (int)(SR * 0.018f);
    for (int i = 0; i < n; i++) {
        float in = (i >= pre) ? dry[i - pre] : 0.0f, sum = 0;
        for (int k = 0; k < 4; k++) {
            int idx = i % cl[k];
            float out = c[k][idx];
            lp[k] = lp[k] * damp + out * (1 - damp);
            c[k][idx] = in + lp[k] * fb;
            sum += out;
        }
        sum *= 0.25f;
        for (int j = 0; j < 2; j++) {
            int idx = i % al[j];
            float bufout = ap[j][idx], y = bufout - 0.6f * sum;
            ap[j][idx] = sum + 0.6f * y;
            sum = y;
        }
        work[i] = dry[i] * (1.0f - wet * 0.35f) + sum * wet;
    }
}

static i16 *Commit(int n, float gain) {
    if (poolUsed + n > POOL_MAX) { TraceLog(LOG_ERROR, "audio pool full"); return 0; }
    i16 *out = pool + poolUsed; poolUsed += n;
    for (int i = 0; i < n; i++) {
        float v = work[i] * gain;
        if (v >  1.0f) v =  1.0f;
        if (v < -1.0f) v = -1.0f;
        out[i] = (i16)(v * 32767.0f);
    }
    return out;
}
static Sound MakeSound(i16 *pcm, int n) {
    Wave w = { (unsigned)n, SR, 16, 1, pcm };
    return LoadSoundFromWave(w);
}
static void Register(int id, const char *name, int n, float gain, float base, int aliases) {
    SfxDef *s = &sfx[id];
    s->name = name; s->n = n; s->base = base; s->nAlias = 0;
    s->pcm = Commit(n, gain);
    if (!s->pcm || !ready) return;
    s->snd = MakeSound(s->pcm, n);
    for (int i = 0; i < aliases && i < 4; i++) s->alias[s->nAlias++] = LoadSoundAlias(s->snd);
}

// ---------------------------------------------------------------- the palette
static void Synth(void) {
    int n;
    // footstep on stone: a short dull tap
    n = (int)(SR * 0.09f); Clear(n); Noise(n, 1.0f); LowPass(n, 1100, 1100, 0); Env(n, 0.001f, 0.011f);
    Register(SFX_STEP_STONE, "step", n, 0.9f, 0.32f, 3);
    // footstep on a shelf: hollower, a knock with a little ring
    n = (int)(SR * 0.12f); Clear(n); Noise(n, 0.7f); LowPass(n, 700, 700, 0); Env(n, 0.001f, 0.010f);
    { float save[3000]; memcpy(save, work, sizeof(float) * n); Clear(n); Sine(n, 250, 250, 0, 0.5f); Env(n, 0.001f, 0.045f);
      for (int i = 0; i < n; i++) work[i] += save[i]; }
    Register(SFX_STEP_SHELF, "shelf", n, 0.9f, 0.30f, 3);
    // landing: a thud. Sweep down, noise on top, a little of the room
    n = (int)(SR * 0.55f); Clear(n); Sine(n, 150, 52, 0.11f, 0.9f); Env(n, 0.002f, 0.075f);
    { static float save[13000]; memcpy(save, work, sizeof(float) * n); Clear(n); Noise(n, 0.8f); LowPass(n, 500, 500, 0); Env(n, 0.001f, 0.030f);
      for (int i = 0; i < n; i++) work[i] += save[i]; }
    Soft(n, 1.6f); Reverb(n, 0.22f, 0.78f, 0.35f);
    Register(SFX_LAND, "land", n, 0.8f, 0.70f, 2);
    // jump: barely a breath
    n = (int)(SR * 0.10f); Clear(n); Noise(n, 1.0f); HighPass(n, 500); LowPass(n, 2400, 2400, 0); Env(n, 0.018f, 0.045f);
    Register(SFX_JUMP, "jump", n, 0.8f, 0.14f, 2);
    // into the water: a bloop under a rush that darkens as it closes over you
    n = (int)(SR * 1.1f); Clear(n); Noise(n, 1.0f); LowPass(n, 3200, 500, 0.30f); HighPass(n, 180); Env(n, 0.004f, 0.16f);
    { static float save[25000]; memcpy(save, work, sizeof(float) * n); Clear(n); Sine(n, 330, 130, 0.14f, 0.7f); Env(n, 0.003f, 0.085f);
      for (int i = 0; i < n; i++) work[i] += save[i]; }
    Reverb(n, 0.30f, 0.80f, 0.30f);
    Register(SFX_SPLASH_IN, "splash", n, 0.8f, 0.62f, 2);
    // out of the water: lighter, wetter, a few drops after
    n = (int)(SR * 0.6f); Clear(n); Noise(n, 0.9f); LowPass(n, 1800, 900, 0.2f); HighPass(n, 300); Env(n, 0.003f, 0.09f);
    for (int k = 0; k < 4; k++) { int at = (int)(SR * (0.10f + 0.07f * k)); float f = 1500 + 600 * Rnd();
        for (int i = 0; i < (int)(SR * 0.05f) && at + i < n; i++) work[at + i] += sinf(2 * PI_F * f * i / SR) * 0.25f * expf(-i / (SR * 0.012f)); }
    Reverb(n, 0.25f, 0.78f, 0.35f);
    Register(SFX_SPLASH_OUT, "surface", n, 0.8f, 0.36f, 2);
    // swimming: water moved aside, softly
    n = (int)(SR * 0.42f); Clear(n); Noise(n, 1.0f); LowPass(n, 900, 500, 0.3f); HighPass(n, 200); Env(n, 0.06f, 0.16f);
    Register(SFX_SWIM, "swim", n, 0.8f, 0.20f, 2);
    // a drip: a plink whose pitch falls in its first few milliseconds, then the cave
    n = (int)(SR * 1.7f); Clear(n);
    { float ph = 0; for (int i = 0; i < n; i++) { float t = (float)i / SR; float f = 2100 * (1 + 0.7f * expf(-t / 0.0035f));
        ph += 2 * PI_F * f / SR; work[i] = (sinf(ph) + 0.35f * sinf(ph * 1.5f)) * expf(-t / 0.028f); } }
    Reverb(n, 0.62f, 0.86f, 0.25f);
    Register(SFX_DRIP, "drip", n, 0.7f, 0.24f, 4);
    // the bulb: rubber. A low tone with a wobble that settles, soft-clipped
    n = (int)(SR * 0.7f); Clear(n);
    { float ph = 0; for (int i = 0; i < n; i++) { float t = (float)i / SR; float f = 165 * (1 + 0.28f * sinf(2 * PI_F * 17 * t) * expf(-t / 0.10f));
        ph += 2 * PI_F * f / SR; work[i] = sinf(ph) * (1 - expf(-t / 0.004f)) * expf(-t / 0.16f); } }
    Soft(n, 2.2f); Noise((int)(SR * 0.02f), 0.15f); Reverb(n, 0.18f, 0.75f, 0.4f);
    Register(SFX_BULB, "bulb", n, 0.8f, 0.60f, 2);
    // the timed bulb: the same, a fifth up and brighter, with a second voice
    n = (int)(SR * 0.8f); Clear(n);
    { float ph = 0, ph2 = 0; for (int i = 0; i < n; i++) { float t = (float)i / SR; float f = 248 * (1 + 0.30f * sinf(2 * PI_F * 19 * t) * expf(-t / 0.10f));
        ph += 2 * PI_F * f / SR; ph2 += 2 * PI_F * f * 2.01f / SR;
        work[i] = (sinf(ph) + 0.4f * sinf(ph2) * expf(-t / 0.08f)) * (1 - expf(-t / 0.004f)) * expf(-t / 0.19f); } }
    Soft(n, 2.4f); Noise((int)(SR * 0.02f), 0.18f); Reverb(n, 0.22f, 0.78f, 0.35f);
    Register(SFX_BULB_TIMED, "bulb!", n, 0.8f, 0.66f, 2);

    // a bush pushed through: dry leaves
    n = (int)(SR * 0.20f); Clear(n); Noise(n, 1.0f); HighPass(n, 1400); LowPass(n, 6500, 6500, 0); Env(n, 0.012f, 0.05f);
    Register(SFX_RUSTLE, "rustle", n, 0.8f, 0.20f, 2);
    // wings: a few beats of air
    n = (int)(SR * 0.38f); Clear(n); Noise(n, 1.0f); HighPass(n, 350); LowPass(n, 2600, 2600, 0);
    for (int i = 0; i < n; i++) { float t = (float)i / SR; float beat = sinf(2 * PI_F * 16.0f * t); work[i] *= beat * beat * expf(-t / 0.13f); }
    Register(SFX_WING, "wing", n, 0.9f, 0.26f, 2);
    // a chirp: two notes, up then down
    n = (int)(SR * 0.17f); Clear(n); Sine((int)(SR * 0.07f), 2500, 3300, 0.06f, 1.0f);
    { float ph = 0; for (int i = (int)(SR * 0.085f); i < n; i++) { float t = (float)(i - (int)(SR * 0.085f)) / SR; float f = 3100 - 700 * (t / 0.08f);
        ph += 2 * PI_F * f / SR; work[i] += sinf(ph); } }
    Env(n, 0.003f, 0.07f); for (int i = (int)(SR * 0.06f); i < (int)(SR * 0.085f) && i < n; i++) work[i] *= 0.2f;
    Register(SFX_CHIRP, "chirp", n, 0.6f, 0.13f, 2);
    // a soft pad: the animal's foot
    n = (int)(SR * 0.06f); Clear(n); Noise(n, 1.0f); LowPass(n, 450, 450, 0); Env(n, 0.001f, 0.009f);
    Register(SFX_PAD, "pad", n, 0.9f, 0.11f, 2);
    // a chirr: the animal settling
    n = (int)(SR * 0.32f); Clear(n);
    for (int i = 0; i < n; i++) { float t = (float)i / SR; float g = sinf(2 * PI_F * 26.0f * t); work[i] = sinf(2 * PI_F * 92.0f * t) * g * g * 0.8f; }
    Noise(n, 0.25f); LowPass(n, 900, 900, 0); Env(n, 0.01f, 0.10f);
    Register(SFX_CHIRR, "chirr", n, 0.8f, 0.18f, 1);
    // the plant's three syllables: a voice-like tone through a moving formant
    for (int v = 0; v < 3; v++) {
        static const float F0[3] = { 230, 262, 300 }, FA[3] = { 620, 950, 1400 }, FB[3] = { 1150, 520, 800 };
        n = (int)(SR * 0.17f); Clear(n);
        { float ph = 0; for (int i = 0; i < n; i++) { float t = (float)i / SR;
            float f = F0[v] * (1.0f + 0.06f * (t / 0.17f) + 0.02f * sinf(2 * PI_F * 6.5f * t));
            ph += 2 * PI_F * f / SR;
            float s = sinf(ph); s = s > 0 ? powf(s, 0.6f) : -powf(-s, 0.6f);      // a little edge, like a reed
            work[i] = s; } }
        LowPass(n, FA[v], FB[v], 0.15f); LowPass(n, FA[v] * 1.3f, FB[v] * 1.3f, 0.15f);
        for (int i = 0; i < n; i++) work[i] *= 3.0f;
        Env(n, 0.014f, 0.062f); Soft(n, 1.4f);
        Register(SFX_PLANT0 + v, v == 0 ? "plant-a" : v == 1 ? "plant-b" : "plant-c", n, 0.9f, 0.34f, 3);
    }

    // ambience. Six seconds, looped, ends crossfaded so the seam is not a click.
    for (int r = 0; r < ROOM_COUNT; r++) {
        n = SR * 6; Clear(n);
        float b = 0;                                                    // brown noise
        for (int i = 0; i < n; i++) { b = b * 0.996f + Rnd() * 0.02f; work[i] = b; }
        LowPass(n, 170, 170, 0);
        for (int i = 0; i < n; i++) work[i] *= 0.30f;     // quiet. It is a room, not a sound.
        for (int i = 0; i < n; i++) {                                    // a drone, three partials, slowly breathing
            float t = (float)i / SR;
            work[i] += 0.011f * sinf(2 * PI_F * 55.0f * t) * (0.6f + 0.4f * sinf(2 * PI_F * 0.07f * t))
                     + 0.008f * sinf(2 * PI_F * 82.4f * t) * (0.6f + 0.4f * sinf(2 * PI_F * 0.11f * t + 1.0f))
                     + 0.005f * sinf(2 * PI_F * 110.0f * t) * (0.5f + 0.5f * sinf(2 * PI_F * 0.05f * t + 2.0f));
        }
        if (r == 1) {                                                    // water: a band of noise that swells
            float y = 0, hp = 0;
            for (int i = 0; i < n; i++) {
                float t = (float)i / SR, x = Rnd();
                float a = expf(-2 * PI_F * 650 / SR); y = a * y + (1 - a) * x;
                float a2 = expf(-2 * PI_F * 280 / SR); hp = a2 * hp + (1 - a2) * y;
                float band = y - hp;
                float swell = 0.5f + 0.5f * sinf(2 * PI_F * 0.21f * t) * sinf(2 * PI_F * 0.13f * t + 0.7f);
                work[i] += band * 0.10f * swell;
            }
        }
        int xf = SR / 2;                                                 // crossfade the tail into the head
        for (int i = 0; i < xf; i++) {
            float k = (float)i / xf;
            work[n - xf + i] = work[n - xf + i] * (1 - k) + work[i] * k;
        }
        ambPcm[r] = Commit(n, 1.0f); ambLen[r] = n;
        if (ready && ambPcm[r]) amb[r] = MakeSound(ambPcm[r], n);
    }
}

// ---------------------------------------------------------------- under the surface
// A low-pass on the whole mix, opened and closed smoothly, driven by whether the
// surface is above your head. The one effect that is not baked in, because it is about
// where you are, not what happened.
static volatile float muffleTarget = 0.0f;
static float muffle = 0.0f, lpL = 0.0f, lpR = 0.0f;
static void Mixed(void *buffer, unsigned int frames) {
    float *b = (float *)buffer;
    for (unsigned i = 0; i < frames; i++) {
        muffle += (muffleTarget - muffle) * 0.0015f;
        float a = muffle * 0.93f, g = 1.0f - 0.25f * muffle;
        lpL = lpL * a + b[2 * i] * (1 - a);     b[2 * i]     = lpL * g;
        lpR = lpR * a + b[2 * i + 1] * (1 - a); b[2 * i + 1] = lpR * g;
    }
}

// ---------------------------------------------------------------- api
void AudioInit(int mute) {
    if (!mute) {
        InitAudioDevice();
        ready = IsAudioDeviceReady();
        if (!ready) TraceLog(LOG_WARNING, "no audio device; the game is silent");
    }
    Synth();
    if (ready) {
        SetMasterVolume(0.8f);
        AttachAudioMixedProcessor(Mixed);
        AudioAmbience(roomIdx);
    }
}

void Sfx(int id, float vol, float pitch, float pan) {
    if (id < 0 || id >= SFX_COUNT) return;
    SfxDef *s = &sfx[id];
    dbgLastSfx = s->name; sfxCount[id]++;
    if (!ready || !s->pcm) return;
    Sound snd = s->nAlias ? s->alias[s->rr++ % s->nAlias] : s->snd;
    SetSoundVolume(snd, vol * s->base);
    SetSoundPitch(snd, pitch);
    SetSoundPan(snd, pan);
    PlaySound(snd);
}

void AudioAmbience(int room) {
    if (!ready || room < 0 || room >= ROOM_COUNT) return;
    if (ambRoom >= 0) StopSound(amb[ambRoom]);
    ambRoom = room;
    SetSoundVolume(amb[room], AMB_VOL);
    PlaySound(amb[room]);
}

void AudioStep(void) {
    // head under: the surface line sits within the top few pixels of the body, or above it
    muffleTarget = (player.waterY >= 0 && player.waterY <= (int)player.y + 3) ? 1.0f : 0.0f;
    if (ready && ambRoom >= 0 && !IsSoundPlaying(amb[ambRoom])) PlaySound(amb[ambRoom]);
}

float AudioRnd(void) { return Rnd(); }

// Every sound, one after another with a gap, as a .wav -- the only way to listen to the
// palette in isolation, and the only way to look at it (a spectrogram of this file).
int AudioExportMontage(const char *path) {
    static i16 out[POOL_MAX + SFX_COUNT * SR];
    int gap = (int)(SR * 0.45f), n = 0;
    printf("%-8s %6s %5s %5s\n", "sound", "sec", "peak", "rms");
    for (int id = 0; id < SFX_COUNT; id++) {
        SfxDef *s = &sfx[id];
        if (!s->pcm) continue;
        float peak = 0, sq = 0;
        for (int i = 0; i < s->n; i++) { float v = s->pcm[i] / 32768.0f * s->base; if (fabsf(v) > peak) peak = fabsf(v); sq += v * v; }
        printf("%-8s %6.2f %5.2f %5.3f\n", s->name, (float)s->n / SR, peak, sqrtf(sq / s->n));
        for (int i = 0; i < s->n; i++) out[n++] = (i16)(s->pcm[i] * s->base);
        for (int i = 0; i < gap; i++) out[n++] = 0;
    }
    for (int r = 0; r < ROOM_COUNT; r++) {
        if (!ambPcm[r]) continue;
        printf("amb%d     %6.2f\n", r, (float)ambLen[r] / SR);
        int take = ambLen[r] < SR * 3 ? ambLen[r] : SR * 3;
        for (int i = 0; i < take; i++) out[n++] = (i16)(ambPcm[r][i] * AMB_VOL);
        for (int i = 0; i < gap; i++) out[n++] = 0;
    }
    Wave w = { (unsigned)n, SR, 16, 1, out };
    return ExportWave(w, path);
}
