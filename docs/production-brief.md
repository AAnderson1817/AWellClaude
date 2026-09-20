# The Vault and the City Under It — 3D production brief

Original request: a AAA 3D reinterpretation of Animal Well that preserves its 2D perspective, with Disney animation inspired layering, at least 30 reference images per environment, rigorously evaluated design principles, and immersion grounded in metallurgy, astronomy and faith.

The existing project is AAnderson1817/AWellClaude. This branch, `codex/celestial-foundry`, starts at commit `99de7856bedfa059439da1b89435f54c98bddaf7` from `claude/animal-well-redux-slice-oimk6l`. Preserve the existing C/raylib simulation, authored rooms, interaction design, fixed camera, and established quietness tuning.

## Environment contract

The existing environments are the Vault Mouth and the Drowned Quarter, described in `claude/PREMISE.md` and `claude/ROOMS.md`. Metallurgy, astronomy and faith become layers in their existing vault/city setting. Each environment requires its own 30-image reference collection before acceptance.

Metal holds memory. The positions of distant stars are recorded as intervals struck into bronze. Repeated ritual actions are also observations and experiments. The work leaves open whether the sanctuary's vanished inhabitants understood the sky or were trying to persuade it.

Blue basalt supports bronze mechanisms. Copper oxidation follows moisture. Amber light belongs exclusively to the hunters; green-white light belongs to the city. Pale blue atmospheric reflection is not a new emissive source. The world uses curved masses, narrow luminous accents, broad quiet surfaces, and silhouettes with negative space. Layering follows classic multiplane animation: foreground framing, readable interactive stage, middle architecture, distant silhouettes, and atmospheric background. Gameplay uses a fixed side view and collision in one plane; scenery combines actual 3D geometry and distant painted mattes.

## Acceptance targets

- At least 30 distinct generated reference images, not crops counted as separate concepts, archived with prompts and captions.
- Preserve the existing traversal and discovery loop with reusable non-combat objects, readable cause and effect, reversible experimentation, and wordless affordances. Do not design new puzzle gates ahead of discovered toy interactions.
- Character and landing-surface readability at 1280 × 720 and 1920 × 1080. Background art must not impersonate colliders.
- Quiet visual regions between accents and an optional still-atmosphere mode. No on-screen teaching text, counters, camera shake or body squash. The original audio palette and event rates are preserved; sparse city murmurs and a sinking-stone hum extend that soundscape. Their full-mix listening acceptance remains open.
- Editable Blender hero assets with supported GLB materials, verified exported scale, documented budgets and independent visual review.
- Automated deterministic simulation tests and actual rendered native playthrough evidence; verify the existing web target separately when available. Automated checks do not establish immersion or artistic quality; these require human observation and playtests.

Working target: the existing C/raylib game, native desktop and its existing Emscripten web target, conventional real-time geometry. The camera remains locked per room, no tutorial text or counters are added, and movement/body dimensions remain unchanged. The city retains green-white light; the hunters retain amber flame. AAA is an artistic and production target, not a claim inferred from an engine, polygon count, or successful tests. Overall acceptance remains open until the evidence supports it.
