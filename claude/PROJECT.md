# Salt Works — project state

## Where this is
**Reset to a navigation slice**, after the two-verb build was played and found muddled.
Four rooms, movement + water + weight, nothing else. The prior state (two verbs, 25
rooms) is parked on the branch `claude/archive-two-verb-slice` and the tag
`phase4-two-verbs`; `parked/` holds `gaff.c` and the 25 authored rooms.

## Why the reset
The user played the two-verb build and reported: the water is the best thing in it, the
gaff is incomprehensible ("I don't know what biting corners would do"), and some
platforms had no collision.

All three were true and they share a root cause. I was both builder and playtester, and
I substituted instrumentation for play:

- **The platform bug had been in since Phase 1.** `LedgeBlocks` used the AABB occupancy
  convention `(newBottom - 1)` for what is a *crossing* test, so a one-pixel fall checked
  the row the feet were leaving and never the row they were entering. Every one-way
  platform in the game was passable from above. I never caught it because I never tested
  the one thing it governs — I verified crust fatigue, buoyancy, corner wear, room arenas
  and edge transitions, and never once checked that a body can stand on a platform. Worse,
  `traverse.py` modelled one-way tiles as landable, so tooling and game disagreed for four
  phases while both reported success.
- **The gaff failed L3's first gate** — a stranger enjoys it before knowing its purpose.
  I certified that gate on an emergence count and 4,000 frames of traces. A trace shows
  that hooking a submerged ledge stores buoyant energy. It cannot show that nobody would
  discover they can hook at all.
- **The decision that let this through** was folding Phase 1's gate into Phase 2. That
  gate was "the user plays it and judges the movement", and I argued myself out of it
  because the load dial *is* the movement. The argument was reasonable and the conclusion
  was wrong: that gate existed precisely to catch this.

The thing the user praised is the tell. Fractional buoyancy came from *playing* — the
first version was a binary in-water test that oscillated at the surface, and only running
it showed that. Everything muddled came from reasoning about what would be interesting.

## The rule going forward
Legibility is the first gate for any new verb: a stranger uses it correctly inside thirty
seconds, unprompted, before emergence gets a vote. And a playable build goes in the user's
hands before anything is built on top of it.

## What is in the build now
- `src/aw.h` — 320x180, 8px tiles, 40x22 rooms, **2x2 world** (the 5x5 ceiling returns
  when the real world is built; this is scaffolding for feel).
- Movement: run 1.45 px/f, turnaround bite, apex hangtime, jump cutoff, 6-frame coyote,
  7-frame input buffer.
- Water: buoyancy blends gravity and a load-signed sink term **by submerged fraction**,
  never a binary in-water test. A body floats where the two cancel, which differs per load
  and is therefore readable. This is the part that works — do not "simplify" it.
- Weight: `load` 0..4 indexes jump, gravity, terminal velocity, jump cutoff and sink.
  **Changed anywhere in the water** (hold X, or X+Down to pour) — the rim tile is gone.
  That removed a whole concept, made the loop self-teaching, and eliminated the deep-water
  softlock class outright, since you can always pour out and float up.
- One platform type only. Two that behaved identically was needless muddle.
- Crust: a floor that fatigues under a heavy body and gives way. Kept because it is the
  clearest statement of "your weight decides which floors exist".

## Tools
`tools/genrooms.py` compiles `rooms/world.txt` into the binary and validates edges.
`tools/traverse.py` static reachability at all five loads. `tools/compose.py` frame usage
and silhouette repeats. `tools/sweep.py` + `--wander` seeded bots. `--contact FILE`
renders every room to one sheet. See `tools/TOOLCHAIN.md` for the build gotchas.

**Every one of these tools has been wrong at least once**, and in Phase 4 they were wrong
more often than the rooms were — see `parked/rooms-25/BREAKAGE.md` for the list. Reproduce
a failure in a running build before believing any of them.
