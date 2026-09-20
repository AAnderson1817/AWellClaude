# The city's response to Hold

This increment implements four authored build-2 response families from
`claude/ROOMS.md`. It adds no rooms, access conditions, geometry changes or player
verbs. The inherited movement, held-item physics, props and creatures remain the
owners of their original state. The city reads that state on the fixed simulation
step, keeps its own random stream, and gives both renderers detached snapshots.

The mural on the Vault Mouth's lower back wall is visible without the lamp. Light
within six tiles conceals its phosphor procession; removing that light restores it
over two seconds. The existing campfire also conceals it. Because that fire already
persists, lighting it changes what can be seen until the world is reset. There is no
reward, counter or gate attached to this response.

The Vault window and the three Drowned windows douse as the lamp comes within five
tiles. They remain dark while it stays and wait five to ten seconds after departure
before lighting again. A return visit renews that wait. Figures sometimes pass behind
the grilles. Standing within three tiles permits an occasional low two-syllable
murmur, with a twelve-to-twenty-second cooldown. The sounds carry no words.

The drowned face breathes through its green-white eyes on a seven-second cycle. A
released stone sinking in water within three tiles of the carved bounds receives one
low note and two seconds of brighter eyes. A resting stone, a carried stone, and an
already acknowledged approach do not repeatedly trigger it. The face remains still.
Decision D8 in `claude/PREMISE.md` records the response table's conflicting eye
coordinates and the chosen placement within the brow; the floor remains untouched.

Eight pale fish drift toward the capital lantern. A lamp floating on the actual
surface in their connected basin draws them beneath it. A carried lamp does not
create that floating beacon. The island walls separate the outer water pockets;
a lamp there cannot attract this shoal through stone, so it stays at the capital.
They scatter when the body comes within three tiles, and their movement tests the
actual water tile mask, including the column and islands. The native's response is
not implemented in this increment because that inhabitant does not yet exist.

## Evidence and limits

[The response tests](../tools/tests/city_responses.c) link the real response, item, prop and room code,
using I/O stubs only. It checks all four windows, delayed and interrupted returns,
crossings, proximity voice frequency, the mural's timed recovery and actual campfire
ignition, a stone descending under real item physics, exact eye-cycle and acknowledgment
durations, actual floating-lamp gathering, disconnected-basin exclusion, body scatter, 6,000 additional shoal bounds
steps, snapshot detachment and unchanged body/item/tile ownership. They run with
undefined-behavior sanitization and no recovery.

[The recorded headless output](evidence/city-responses/headless.txt) includes the
actual event times: the fixture's sinking stone is acknowledged on step 32 at
0.7 pixels per step; the floating lamp moves the shoal's mean y from 80.800 to
66.880 room pixels. The four tested window waits are 530, 428, 428 and 428 steps.

The original-baseline checker removes only the marked appended city audio synthesis
block before comparing all original audio code. It rejects random-stream calls in
that block. Every inherited gameplay trace field remains compared exactly; none is
filtered. The inherited `dbgLastSfx` remains the original gameplay event, while a
separate `CITY SFX` line reports the added murmurs and hums. Both presentation modes
must produce identical city sound counts. This is preservation of original behavior
with an intentional audio extension, not a claim that the whole soundtrack is unchanged.

The native review fixture (`tools/capture-city-win.ps1`) runs the real frame loop and
3D renderer. It changes only initial placement, like the existing debug spawn flags.
Its extra sinking stone is confined to the review executable. It exports contrasting
mural, window and shoal states plus the face before, during and after acknowledgment,
with state logs beside the captures. These fixtures verify authored responses under
controlled conditions; they do not establish wordless discovery or human immersion.

The face's eye response, mural discovery and fish/lamp relationship still require
cold-player observation and listening in the complete mix. The hunter, native,
mouth bubbles, drowned facade openings, remaining build-2 dressing, final sound
pass, second verb and deeper-layer resolution
remain outside this increment. AAA art acceptance is assessed separately against
the actual native captures, not inferred from these passing behavior tests.
