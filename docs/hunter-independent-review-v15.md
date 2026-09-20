# Hunter integration: independent code and native review

2026-09-20. **Verdict: rework the causal voice policy; retain the recorded item/ownership integration.** The native tending reach also needs a bounded gesture correction/review before claiming compliance with the animation law. This is a review of the first integrated candidate, not final v15 acceptance.

Reviewer: carved-face agent. I did not author the hunter state machine, audio, hat/pack, or hunter renderer. I authored the unrelated Drowned face and participated in earlier terrain work. I changed no production files during this audit. The frozen face decision remains unchanged.

## Evidence and scope

Reviewed `main.c`, `items.c`, `inhabitants.c/.h`, `audio.c/.h`, both hunter draw paths, the cairn prop suppression, `hunter_responses.c`, `hunter_audio.c`, and the actual CLI integration test. Read the reported module/audio/CLI results; I did not rerun those suites merely to duplicate them. Governing requirements are the hunter table in `claude/ROOMS.md` and `claude/DESIGN-LAW.md`.

The [native manifest](evidence/hunter/native/manifest.json) identifies executable SHA-256 `9b85169d10ecd1c3510e66168e6be3505c3443948c972add75601aea225649b7`, inhabitants source `40be13380171e22c0899786b05e62fc428a3c96ba313f592aefbbcb673a7382a`, and depth source `0de785f47dd16c617fd1622e9c5b4388e84e74f704b8530e3756e38d4bd23f5b`. These are the reviewed versions; subsequent policy fixes need their own evidence.

I inspected full-room Vault idle, return frame 50, fire frame 160, flat return frame 50, and Drowned frame 120. I also compared twelve unscaled camp crops covering idle, taking, carrying, placement, peak tending and warm idle. The [contact sheet](evidence/hunter/native-review-contact-sheet.png) only crops and arranges the original PNG pixels; no enhancement or rescaling was applied.

## Required causal response correction

The [preserved first CLI timeline](evidence/hunter/voice-v1-regression.json) exposes a fault that the successful ownership tests do not reject. Reproduce with:

```
--at 15,19 --play -:2,X:1,-:8,L:8,X:1,L:24,-:400 --trace --trace-hunter --mute
```

| Frame | Actual event/state | Voice consequence |
|---|---|---|
| 3 | Player takes stone 3; hunter stands | TAKEN requested but blocked by greeting plus quiet gap |
| 40 | Hunter has recovered that stone and is carrying it | No sharp response yet |
| 45 | Hunter is carrying the returned stone | Stale TAKEN starts |
| 84 | Hunter is placing the stone; player has left the bedroll | Queued BEDROLL starts |
| 95 | Stone joins cairn; placement completes | OFFER requested |
| 131 | Hunter has returned to watching | OFFER finally starts |

The separate carry-away case starts TAKEN at frame 45 after the hunter sat at frame 32. This weakens the wordless relationship between action, gesture and answer required by the room table and L6. It is not a PCM duration or stone-conservation error.

In the reviewed `VoiceStep`, the bounded priority set waits for both the previous phrase and its 24-tick quiet gap. Only greeting has a disappearing-context cancellation. Require prompt sharp-taken preemption of a lower-priority phrase/gap, cancellation of obsolete bedroll/idle requests, and assertions about the state/context **when a voice starts**. Preserve the original failing trace; recheck the actual CLI sequence after the fix. A generic queue must not replay earlier social situations after they have ended.

## Native gesture and craft findings

**Tending reach needs correction/review.** In [peak tending, frame 2276](evidence/hunter/native/tend/f2276.png), both narrow arms become long diagonal rods from the chest to a stone above the hat. Compare [start, frame 2228](evidence/hunter/native/tend/f2228.png) and [replacement, frame 2324](evidence/hunter/native/tend/f2324.png). The renderers calculate each elbow as a fraction of shoulder-to-hand distance, so the arm segments actually change length as the hand target moves. Body, pack and hat stay rigid, but that alone does not establish L10's “No squash-and-stretch.” The native result reads as elastic arms. Preserve actual item ownership and visible hand contact while constraining the limb reach or changing the reachable pose; inspect the corrected peak at the same camera and scale.

**Cairn craft/readability concern, separate from correctness.** Four identically tilted ellipsoids read as an almost body-height vertical totem rather than four flat stones. In [warm idle, frame 160](evidence/hunter/native/fire/f0160.png), the pile masks much of the hunter's left face/body. The right-facing eyes and hands remain discernible, so this is not complete loss of the fire response. More varied, flatter real stone silhouettes could improve the cairn and reduce competition with the actor, but must preserve each real item's pickup bounds and single draw. No new proxy pile is appropriate.

The hat and side pack clearly distinguish the hunter from the player. The actual carried stone is visible between the hands at return frame 50, and rejoins one cairn at frame 104. Warm movement brings the hands toward the fire. The character does not introduce a new bright focal beacon or fill the frame with effects. Sampled frames show no duplicate decorative cairn, disconnected hat/pack, body scaling, off-room hunter, or substrate contamination.

## Mechanical findings and limits

No additional substantive ownership/reset fault was found. Initialization appends four stable real IDs once; pickup is still the original Hold loop; pinned stones only skip gravity. Reconciliation relinquishes player-held/off-room stones, including interruptions while carrying/tending, and releases a different carried stone if another member is taken. The reviewed fixtures exercise conservation, six-stone exhaustion, competing pickup, cross-room sinking, return without respawn, full reset, persistent fire, and player-space yielding. The main loop stops hunter sound on reset and room exit, then clears the off-room pending voice/mouth state. The source-derived item homes and original RNG/audio state remain separately checked.

The PCM and detached mouth view share a pitched sample timeline; technical tests cover voice/breath/tail boundaries and silence. Both renderers use that copied view without mutating simulation. Muted native stills cannot establish audible synchronization, voice quality, interruption comfort, full-mix restraint, or how a person reads a moving gesture. Those remain pending.

The integration adds no verb, gate, reward counter, on-screen instruction, combat or camera movement. The hunter's player-sized body is explicitly required by the room table, so it is not a violation of the broader large-creature preference. These bounded findings do not establish human discovery, immersion, complete design-law acceptance, or AAA finish.
