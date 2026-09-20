# Hunter voice and mouth timing

2026-09-20. **Technical audio increment; perceptual acceptance pending.** The Vault hunter has six quiet, nonverbal phrase shapes. They share a reedy mid-register voice with moving vowel resonances; the fire phrase is lower and longer, taking a stone produces a short higher utterance, and the hum has a nearly closed mouth. These are synthesized sounds, not words or a dialogue system.

`src/audio.h` contains the shared pure timing specification. `src/audio.c` appends one marked hunter bank after all existing audio code. `src/inhabitants.c` uses that same specification for the detached mouth and phrase state. The original palette, city synthesis, audio random stream, `Sfx` counters and debug last-sound field remain unchanged. Hunter playback has separate counters reported by `AudioHunterPrintStats`.

| Event | Syllables | Source duration | Nominal pitch | Approximate playback duration |
| --- | --- | --- | --- | --- |
| Greeting | 1 | 0.330 s | 1.00 | 0.330 s |
| Stone placed | 3 | 0.885 s | 1.00 | 0.885 s |
| Stone taken | 1 | 0.280 s | 1.18 | 0.237 s |
| Fire catches | 3 | 1.355 s | 0.72 | 1.882 s |
| Bedroll | 1 | 0.365 s | 1.00 | 0.365 s |
| Hum | 1 | 0.590 s | 0.83 | 0.711 s |

Durations in the table are rounded. The actual count is the sum of integer 22,050 Hz source-sample spans, including the leading breath, voiced syllables, closed pauses, final breath and short diffuse tail. Existing event pitch variation is ±0.012; the duration query applies the exact event pitch. A shared finite clamp of 0.5–1.5 applies to both the timing helper and `SetSoundPitch`, so malformed inputs cannot silently separate those timelines.

`AudioHunterDurationTicks(kind, pitch)` returns the ceiling of actual sample duration divided by playback pitch at 60 Hz. `AudioHunterMouthAt` samples the same voiced envelope used in synthesis at the corresponding pitched sample position. Mouth openness is zero during pauses, breath and tail. The hum limits visible openness to 0.16. `HunterView` exposes `mouthOpen`, `voiceElapsedFrames`, `voiceTotalFrames`, and zero-based `voiceSyllable` (`-1` outside a voiced span); the existing binary `mouth` remains available. The fire phrase no longer uses the old 54-tick approximation, which ended well before its low-pitch playback.

The hunter keeps a bounded request bit set, with action responses separated from incidental speech. Taking a stone starts the sharp syllable on that simulation tick, preempting any lower speech or quiet gap. Placing a stone starts its answer on that placement tick. Both discard old greeting, bedroll and hum requests. The fire request can wait while the hunter handles a stone or watches a taken stone; its persistent lit state remains a valid context. It does not interrupt an active taken/placement phrase, but it may bypass the incidental quiet gap.

Only incidental greeting, bedroll and hum requests wait for the complete active phrase and a 24-tick quiet gap. A pending greeting expires when the nearby watch context ends or a stone/bedroll interaction supersedes it. Bedroll speech requires the player still to be standing there and the hunter actually waiting; hum requires the hunter to be sitting idle or warm. Room exit clears requests. The established hum spacing remains 25–45 seconds with a cold fire and 12–22 seconds after ignition. One sound plays at a time. Sharp preemption stops the old sound through the existing playback API; the audible transition still needs listening for an objectionable cutoff or click.

This corrects a real integration regression, preserved in [voice-v1-regression.json](evidence/hunter/voice-v1-regression.json): taking a stone on frame 3 could previously speak on frame 45, after recovery or return, and a departed bedroll request could play during placement. The old request queue was bounded in size but semantically stale. The revised context and same-tick rules address that defect; they do not prove that the complete sound mix feels comfortable.

## Integration contract

Call `AudioHunterInit()` immediately after `AudioInit()`, including muted/headless startup. It synthesizes six bounded PCM buffers once. Each phrase uses its own local synthesis seed; it never borrows the original audio pool, mutable synthesis buffers or RNG. Samples are softened below clipping and have zero-valued first/last samples. Short internal reflection taps stay within the declared tail. They are not a claim of a spatial acoustic simulation.

After every simulation tick, a successful `InhabitantsPollVoice(&event)` is followed by `AudioHunterPlay(&event)`. The latter returns the same pitch-adjusted total tick count used by the mouth state. `AudioHunterStop()` is required on leaving room 0 and on reset; `AudioHunterClose()` releases its sounds at native shutdown. Stop and close are idempotent. The bank owns one active speaker and stops an earlier sound defensively before playing a new event.

The phrase source level is multiplied by 0.60 and the event volume (normally 0.20, hum 0.12), then passes through the existing master volume and underwater mix processor. Pan uses the event's room position. The separate export API `AudioHunterExportMontage(path)` produces an isolated sequence at the authored pitch and event levels, before master volume or full-mix processing. It is a review artifact, not the in-game mix.

## Technical evidence and remaining limits

Run `tools/tests/test_hunter_audio.py` with the invoking PowerShell affinity set to 65535. The C99 UBSan fixture exercises real synthesized PCM, device-call arguments, muted behavior, allocation/stop/unload lifecycle and detached counters. It checks 26,293 pitch-scaled pose/time samples, exact tick-ceiling bounds across pitches 0.50–1.50, no clipping, bounded DC, sample endpoints and unchanged original PCM/RNG/counters. The runner also removes only the marked EOF append and compares every previous audio line to the v14 checkpoint `8b52542`, allowing only checkout line-ending normalization. Results and hashes are in [audio-tests.json](evidence/hunter/audio-tests.json).

The real item/prop/hunter harness, extended for the later fixed-arm tending correction, passes [14,116 detached voice-pose checks](evidence/hunter/integrated-module-tests.json) against the pitched timeline, alongside 28,748 conservation checks and 6,000 exact inherited-state idle comparisons. Regression assertions require every real pickup/placement to emit its answer on that action tick, test taken preemption of greeting/fire/quiet gap, and prove that departed bedroll/greeting requests expire. These also verify the shared timing through room/reset lifecycle tests; they do not measure an audio device. The separate CLI integration suite checks the same timing constraints through actual executable input plans.

Six isolated WAVs are generated under `.local/hunter-audio/hunter-1.wav` through `hunter-6.wav`, in the table's order. Their levels and waveforms are technically measured; **no listening acceptance is claimed**. They require listening in isolation and beside the room ambience, plants, drips, fire and water muffle. Device buffering can introduce output latency; the implementation aligns logical playback and animation within a simulation tick, not by querying an audio-device sample clock. Variable render rates and actual device latency need native observation. Readable gesture synchronization, distinct intent, sensory comfort, wordless interpretation and immersion remain human/perceptual checks.
