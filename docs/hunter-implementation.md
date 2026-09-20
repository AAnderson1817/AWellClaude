# Hunter module: authored interaction contract

2026-09-20. This is an **unhooked implementation increment**, awaiting main-loop, item-fall, renderer and sound integration. It implements the Vault hunter table in `claude/ROOMS.md`; it does not complete build 3 or establish human acceptance. Source: `src/inhabitants.c` and `src/inhabitants.h`.

The hunter offers a second reading of Hold: a stone can be entrusted, reclaimed, carried away, or taken back during its placement. The player receives no reward, gate, counter, instruction or new verb. The module never writes player state, input, tiles, original item homes, `heldItem`, or sound state. Its only world writes are four initial stones and the positions/vertical velocity/grounded state of explicitly owned cairn or carried stones. Independent deterministic randomness supplies timing and voice variation.

## Stones are objects, including the cairn

Initialization appends four `IT_STONE` entries after the existing lamp and two stones: IDs 0–2 remain unchanged; the authored world uses 7 of the existing 8 item slots. Capacity and camp anchors are checked before any append. Initialization is idempotent for the same item world and is not a room-entry operation.

The four seed stones have stable homes at the cairn, center `(124,160)` in room pixels. A stone is 5×4 pixels; 2.2-pixel tier spacing makes a compact, slightly overlapping pile, 10.6 pixels high initially. IDs are allocated top first to retain the original nearest-item loop's lowest-index tie break. The flat four-rock prop must be suppressed when this real cairn is presented; it is not an additional source of stones.

An offered stone keeps its original ID, kind, home and reset room. Ownership is an index in a bounded cairn list or one carried index; no inventory count converts or consumes it. All six stones in the authored world can be removed individually. The maximum cairn contains six stones (15 pixels high), still reachable by the unchanged ground-level Hold test. One unused item slot remains, but this increment does not create another item.

Pinned stones skip only ordinary vertical falling. They remain candidates in the complete original nearest-item pickup loop. Immediately after that loop, reconciliation relinquishes any member the player picked up or transported to another room. This applies during tending, approach, carrying and placement. The hunter never writes `heldItem` or claims a lamp. Drop cooldown is respected, followed by six stable floor ticks before an offering is accepted.

Simultaneous-action resolution: if the player takes a *different* cairn stone while the hunter has one in its hands, the startled hunter releases its held stone at its current visible position and turns to the taken one. That released object resumes original gravity and can be picked up or returned. This resolves a state-machine collision without consuming, teleporting home, or leaving an unrendered owned item. It is an implementation choice within the authored response, and needs native gesture review.

This is an intentional simulation extension, **not** exact equivalence to the old three-item world. Pressing Hold beside the former decorative cairn now picks up a real stone; it can therefore select a new nearer candidate instead of a legacy item. Cooldown and equal-distance ordering remain the original rules, including their existing nearest-candidate behavior. Do not filter those changed held-item/heavy-state results out of a historical comparison. Record the authored action and verify conservation, identity and the resulting original physics separately.

## Responses and timing

All timings are 60 Hz simulation ticks. Pausing or visiting room 1 suspends the hunter's timers and motion; item ownership persists. The visible and audio states are separate from the physics of the player.

| Trigger | Implemented response |
|---|---|
| Idle | Sits at center x=116; schedules tending starts 1,200–2,400 ticks apart when idle. Lifts the actual top stone four pixels, turns it through a detached pose value, returns it over 96 ticks. Interactions can delay a tending start. |
| Player enters five-tile radius | One greeting request, gaze follows for 120 ticks, then returns to the cairn. Leaving rearms the greeting. No pursuit. |
| Released stone within two tiles of the hunter | Accepts only a settled camp-floor stone reachable along the supported floor. Walks to it, visibly carries the actual item, and places it over 36 ticks; requests a three-syllable phrase. |
| Player takes a cairn or carried stone | Releases ownership in the same simulation step, stands and requests a sharp syllable, tracks the stone while it remains within four tiles of camp. A returned stone within that radius can be reclaimed; a distant stone stays wherever the player left it. |
| Existing fire catches | Uses the original persistent fire flag and original two-second lamp ignition. Moves to x=126, presents hands toward the fire, requests a lower three-syllable phrase. A current stone transfer finishes first. |
| Fire remains lit | Hum scheduling changes from 1,500–2,700 ticks to 720–1,320 ticks, including rescheduling when the fire first catches. |
| Player stands on bedroll | Uses the original bedroll state, requests one syllable per entry and waits. Leaving resumes activity. An already active stone transfer finishes first. |

The actor body is 6×11 pixels, the player's dimensions, within four tiles of camp. It walks at 0.24 pixels/tick, or 0.19 while carrying. Paths must have uninterrupted solid/one-way floor with free body clearance. It stops before the player's body and has no collision participation: it cannot push, trap, damage or block traversal. If the player walks through its stationary pose, it does not add a new physical obstacle.

Voice requests use a bounded priority bit set (taken, fire, offer, bedroll, greeting, hum), coalescing repeated requests while a phrase is active. Current phrase envelopes last 18 ticks per syllable, then a 24-tick gap. Pending greeting is dropped when the player leaves; off-room pending voices are discarded. `InhabitantsPollVoice` consumes a single event and must be called once after every simulation tick. These are wordless synthesis parameters, not dialogue strings or a general event sequencer. Actual synthesis and audibility remain unverified until integration.

The current mouth envelope is provisional: `voiceFrames = syllables × 18` does not account for playback length changing with pitch (especially the low fire phrase). Integration must use the actual synthesized sample duration divided by playback pitch, or generate a fixed-duration pitched phrase, before accepting mouth/gesture synchronization. This timing mismatch is a known pending integration requirement, not an audio pass.

## Required integration hooks

Only the new module/tests/docs are delivered here. Root owns the following changes:

1. Add `inhabitants.c` to native, headless and web source enumeration. Include `inhabitants.h` where needed.
2. Once after `RoomLoad()` in startup, while room 0's props are loaded and before a debug starting-room transition, call `InhabitantsInit()` and handle an unexpected failure explicitly. Do not call initialization from `RoomEnter`.
3. In `ItemsStep` replace the one `Fall(it);` call with `if (!InhabitantsPinsItem(i)) Fall(it);`. Keep the complete original Hold candidate loop and held-item handling unchanged.
4. In `Sim`, after `ItemsStep` and `PropsStep`, call `InhabitantsStep()`, then consume `InhabitantsPollVoice`. `in.actPressed` may already be cleared: this module observes the result of Hold, never a second input interpretation.
5. In `BeginAgain`, after `ItemsHome()`, `PropsReset()` and room-0 entry, call `InhabitantsReset()`. This restores membership of the four seed IDs; it does not append, erase or alter legacy homes. Room entry alone must never reset the hunter.
6. Both presentation modes consume copied `HunterView` and bounded `InhabitantsCairnItems`; use the real item positions when rendering stones. Hide the decorative `PR_CAIRN` pile after successful initialization. A pack/hat silhouette and coherent hands/stone contact still need modeled and native review.
7. Give voices their own sound entries/counts and independent sound randomness, preserving original audio counters and RNG. A technical event count is not a listening pass.

For preservation tooling, permit only the explicitly marked include and one Fall guard in the original `items.c` source comparison. Retain all other exact source and old trace assertions. Add source checks for the main hooks, and a separate complete item/membership/event trace. Historical scenarios without an authored item interaction should retain original entity state; scenarios which actually take or offer a cairn stone require an explicit differential test, not ignored output fields. Run full route/escape and rendered comparisons after integration.

## Verification and limits

Run `tools/tests/test_hunter.py` using the bundled Python, with the invoking PowerShell process restricted to the low 16 processors (`[System.Diagnostics.Process]::GetCurrentProcess().ProcessorAffinity=[IntPtr]65535`). It compiles C99 with undefined-behavior sanitizer and no recovery. It adapts a temporary copy of the actual `items.c` using only the proposed include/Fall guard, refusing an unexpected source shape; existing source and build scripts remain untouched.

The source-hashed report is `docs/evidence/hunter/module-tests.json`. Its run records per-step conservation checks, the actual original pickup/drop loop, offered legacy stones, all six stones removed, direct and competing interruptions, cross-room sinking, reset, fire/bedroll/greeting edges, real-item tending, player-space yielding and detached snapshot bounds/purity. A separate 6,000-frame enabled/disabled idle comparison exactly matches hashes of the three original items, player, map, prop state/timers and Sfx counters while the new hunter tends its stones. That comparison is bounded to this scenario and these fields; it is not the full route/life/render suite.

These checks establish mechanical behavior and conservation for the delivered module. They do not establish a convincing person, readable gestures, sound quality, sensory restraint, blind discovery, immersion, AAA finish, or completion of the broader Animal Well design laws. No human acceptance, third verb, ending, native creature or additional room is supplied by this increment.
