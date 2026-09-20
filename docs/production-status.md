# Production status

This branch extends the existing two-room C/raylib game. It is a working 3D
presentation candidate, with original mechanics retained and an immediately playable
native and web build. The overall goal remains active.

## Delivered

- A locked perspective camera calibrated to the original gameplay plane, 3D terrain
  following authored collision silhouettes, layered city mattes and foreground framing.
- Embedded modeled door, astronomical instrument, terrain, pot, lantern and creature
  parts, with editable Blender sources and clean GLB reimport verification.
- Existing responsive props, wildlife, bulbs, lamp/stone behavior, water and reset
  presented in 3D; original renderer available with F2.
- Independent visual review, response captures, native baseline comparisons, route and
  escape searches, sanitizer checks, and live browser rendering checks.
- Two separately documented reference collections and a local art review gallery.

## Acceptance still open

The [independent review](artistic-review.md) does **not** accept the current scene as
AAA quality. Its most consequential findings are the detail/material gap between
playable surfaces and the distant matte, limited evidence of cultural practice, and
the Drowned Quarter's missing defining inhabitants and response-table content.

The inherited slice has no completed ending or full deeper discovery structure. Its
unimplemented face, native, fish, hunter and mural must receive their intended
responses before they can count as completed game content. Adding a decorative image
is insufficient. This branch does not invent new gates or verbs to conceal that gap.

Further production should refine the playable materials and supports from the
reference collection, implement the existing response tables through actual observed
toy interactions, and run blind playtests with sound. Record what players discover
and misunderstand. AAA craft, immersion, sensory comfort, wordless teaching and the
full design law cannot be marked complete from automated preservation tests.

Current technical evidence is in [verification.md](verification.md); source-derived
principles and a concrete blind-play protocol are in
[design-principles.md](design-principles.md).
