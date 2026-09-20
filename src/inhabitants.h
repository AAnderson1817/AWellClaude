#ifndef AW_INHABITANTS_H
#define AW_INHABITANTS_H
#include "aw.h"
#include "audio.h"

enum { HUNTER_SIT, HUNTER_WATCH, HUNTER_APPROACH, HUNTER_CARRY,
       HUNTER_PLACE, HUNTER_TEND, HUNTER_TAKEN, HUNTER_WARM,
       HUNTER_BEDROLL, HUNTER_RETURN, HUNTER_TEND_APPROACH };

// Detached room-pixel views (Y down, no ROOM_Y). x/y is body top-left;
// hands and gaze are world points. No getter advances time or returns live data.
typedef struct {
    float x, y, w, h, handX, handY, lookX, lookY, walkPhase, stoneTurn;
    int state, facing, carriedItem, cairnCount, fireLit, mouth;
    int voiceKind, voiceFrames, tendInFrames, placed, tended, taken;
    float mouthOpen;
    int voiceElapsedFrames, voiceTotalFrames, voiceSyllable;
} HunterView;

// Init ONCE after RoomLoad (room 0): append four real, resettable cairn stones.
// Returns 0 without adding any if the camp or four free item slots are absent.
// Repeated Init on the same item world is idempotent. No room-entry init hook.
int InhabitantsInit(void);
// Call after ItemsHome on a full reset. Reuses the original four item IDs.
void InhabitantsReset(void);
// Call after ItemsStep and PropsStep. All original input handling stays in ItemsStep.
void InhabitantsStep(void);
// ItemsStep skips ONLY Fall(it) when this is true; original Hold selection stays.
int InhabitantsPinsItem(int item);
int InhabitantsCairnItems(int *out, int max);
int InhabitantsHunterView(HunterView *out);
// Event consumption is deliberately separate from detached presentation views.
// Poll after each simulation step; does not call Sfx or touch an inherited RNG.
int InhabitantsPollVoice(HunterVoiceEvent *out);
#endif
