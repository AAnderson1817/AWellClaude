# DESIGN LAW — Animal Well Redux (vertical slice)

These are not suggestions. If a decision conflicts with one of these, the law wins.
If a law is wrong for this game, stop and say so rather than quietly violating it.

**L1 — Ceiling first, then density.** The world is **5×5 = 25 rooms**. This cap never
rises. When you want more content, you make existing rooms denser.

**L2 — Mechanics before puzzles. Never design a puzzle.** Build toys. Play with them.
Log the moments where *you* are surprised by two of your own systems colliding. Then
build a room that showcases exactly one of those surprises. Maintain `SURPRISES.md` —
every entry is a real interaction discovered by running the game, dated, with the room
it became. A room with no corresponding surprise entry is a room designed backwards;
delete it and start again.

**L3 — Two verbs. Both toys. Neither a weapon.** Nothing that reads as a weapon or a
straight power-up. Real-world objects with interesting physics arrive pre-loaded with
behaviours you didn't have to invent. Each verb must pass both gates:
(a) *inherent playfulness* — a stranger picks it up and enjoys it before knowing its
purpose; (b) *second reading* — a background interaction or an ability you can stumble
into. A verb that fails either gate gets redesigned, not shipped.

**L4 — The redundancy cut.** If verb B can do something verb A already does, that
overlap is a bug. Cut or redesign.

**L5 — No text. Hard prohibition.** No dialogue, no tutorials, no tooltips, no
"press X to jump", no item descriptions, no signs, no narration. Do not build a
dialogue system, a cutscene system, or an event sequencer — *even if it would be easy*.
Teaching happens through: room composition, affordance, animal behaviour, and the
first-room-is-the-tutorial principle.

**L6 — The affordance contract.** The world must respond to plausible hypotheses *even
when they are wrong*. If a player throws a verb at something that isn't a puzzle,
something should still happen — a sound, a wobble, an animal reacting. "I want people
to always feel like their ideas are valid, or that the game is at least acknowledging
them."

**L7 — Default-open, gate reactively.** Build the 5×5 fully traversable first. Add
gates *only* where playtesting shows people wander into dead ends. One-way doors are
the soft-gate primitive; item gates are the last resort. Order of acquisition changes
*difficulty and solution space*, never *access*. Model is Mega Man, not Metroid.

**L8 — Layers end in credits; deeper content is invisible, not locked.** Layer 1 ends
with a real credits roll and must feel finished. The Layer 2 hook must be **visible in
the first two minutes, unusable then, usable by the end, and never announced.** No
greyed-out slots, no percentage counter, no "???" entries, no achievement list. If a
player can *see* that they missed something, you have built a checklist, not a well.

**L9 — Dread from indifference, not malice.** Creatures are much larger than the player
and mostly unbothered by them. "It just views you purely as a food source. Its
personality is just kinda animalistic. There's no malicious intent." The player is
disempowered. No combat.

**L10 — Refuse the juice defaults.** No screenshake. No squash-and-stretch. They supply
a reassuring cartoon vocabulary that kills the uncanny. Use procedural animation
*because* it misbehaves. At 320×180, moving an eye one pixel changes a personality.

**L11 — One room is one screen; the camera locks.** No scrolling camera. The locked
frame is a reading instruction: it invites the player to examine the whole composition
and ask where something could be hidden. Room = 40×22 tiles of 8×8 px = 320×176 inside
a 320×180 frame.

**L12 — The unifying test.** "I like to try to guide the player into thinking they
fully understand something, and then reveal that they don't." Apply this to verbs, to
the map, and to the ending.

## ANTI-PATTERNS (any one of these means the slice failed, regardless of how good it looks)

- Any on-screen text that teaches, labels, or narrates — including "Press Z"
- A collectible counter, completion percentage, checklist, or achievement list
- Screenshake or squash-and-stretch
- Designing a puzzle before the verb it uses exists
- A third verb (two — the constraint is the point)
- Raising the 25-room cap
- An ECS framework, entity base class, virtual dispatch, or `std::shared_ptr`
- Streaming assets from disk instead of embedding them
- A scrolling camera
- Combat, or any item that reads as a weapon
- Asking the user to install a toolchain, or shipping a build you have not looked at

## ACCEPTANCE TESTS

A cold player with no instructions:
1. **Discovers a use for a verb no room was built for.** → item design works
2. **Finishes and believes they finished — then spots the thing they missed.** → layer design works
3. **Never asks what a button does.** → wordless teaching works
4. **Describes the world as unsettling without being able to name a threat.** → tone works
5. **Takes a route through the 25 rooms that was not planned.** → default-open works

Tests 1 and 2 are the ones that matter.

## ORIGINALITY CONSTRAINT

This is a novel work, not a clone. It must not reuse Animal Well's actual inventory or
creatures. Off-limits as verbs: yo-yo, bubble wand, flute, slink, b.wand/remote,
UV/blacklight lantern, firecracker, spinning top, disc, animal-shaped flute. Off-limits
as premise: a well, ghosts collected in a lantern, 64 eggs, a cat god, capybaras/kangaroos
as the specific cast. Take the *method*, not the *content*.
