# Animal Well — Research Brief (compiled 2026-08-29)

Full formatted version: published as the "Animal Well Teardown" artifact.

## Confidence markers used throughout
- VERIFIED — traced to Basso's own words or to reverse-engineered code
- SINGLE SOURCE — one interview, uncorroborated
- UNSOURCED — circulates widely, no traceable origin

## Claims that did NOT survive verification
1. **DirectX 11** — DX11 was the original backend; shipping PC build is **Direct3D 12** (adopted for the Xbox port).
2. **stb_vorbis / XAudio2** — Ogg Vorbis is confirmed (asset enum has `Ogg = 3`); the specific library names have no primary source and conflict with GDC coverage describing an engine built "without relying on external libraries."
3. **Console API names (NVN / GNM / AGC)** — appear in no source. Basso only says "their native API." Self-porting with no porting house IS confirmed.
4. **X-macro entity system + tilemap prepass allocation** — SINGLE SOURCE (Wookash podcast), not corroborated against the RE tooling.
5. **Bubble jump discovered accidentally** — SINGLE SOURCE (Second Wind Q&A auto-captions).
6. Tracker music theory from Steam forums is **false**.
7. There are **three** hardcoded AES-128 keys, not one. Key 0 is ASCII `GoodLUcKMyFriEnd`.

## The core thesis
Every distinctive property of Animal Well traces to a ceiling Basso chose or failed to build around, then treated as a density mandate rather than an obstacle.

| Constraint | Consequence |
|---|---|
| One byte per room ID | 16×16 = 256 rooms; density instead of expansion; anti-scope-creep for 7 years |
| No cutscene/dialogue tooling ever built | Zero text; everything forced into level design and visual language |
| No weapons | Inventory became toys → interesting physics → unplanned interactions |
| One executable, no loose files | All 676 assets linked into the binary |
| True 320×180 internal render | Most of the 33 MB story |
| No external dependencies, ever ("30-year rule") | Every secret solvable from shipped bytes; no post-launch content |

**Cost of non-linearity, quantified:** he refused to lock areas behind items, so every new mechanic forced rewrites of existing rooms. ~500+ rooms designed, 256 shipped — roughly 3× budget.

## Engineering (VERIFIED unless noted)
- C-style C++, no member functions/templates/RTTI/smart pointers. Handmade Hero influence; platform/game split with hot-reloaded game code.
- Started 2017 as a DX11 triangle tutorial. All console ports done personally. Graphics ≈70% of porting effort. Switch required HLSL→GLSL/SPIR-V transpiling via MSBuild targets.
- Thin platform wrapper (draw verts, set constant buffer, set shader, set shader resource); logic pushed out into game layer to minimize port surface.
- No CPU/GPU pipelining → same-frame input→render; claims one frame better than Unity/Unreal theoretical best.
- Three nested preallocated memory scopes: global (process) / game (per save) / **screen (wiped every room transition)**. Bug containment comes free with the room arena.

## Data formats (verified against Redcrafter/Animal-Well-editor)
- Asset table at `.data` offset 0: 676 entries × 0x30 bytes; payloads in `.rdata`.
- `struct Room { u8 x,y,bgId,waterLevel,lighting_index,_,_,_; MapTile tiles[2][22][40]; }` = **7,048 bytes exactly**.
- `MapTile` = u16 tile_id, u8 param, u8 flags (mirror/rotate) = 4 bytes.
- Room = 40×22 tiles @ 8×8px = **320×176** — one room is one screen, hence the locked camera.
- Tile flags carry the whole model in 16 bits: collides_left/right/up/down, contiguous (autotiling), blocks_light, has_normals, uv_light, obscures, dirt, hidden.
- The "256" is a *design* constraint (16×16 grid); storage is a sparse list with explicit x,y bytes, roomCount capped at 255. World wraps horizontally.
- **Sprite format = forked Aseprite with custom export target.** Fixed 0x30 header + four flat POD arrays, zero pointer fixups. Layer flags include `is_normals1/2` and `uv_light` — **normal maps authored as extra Aseprite layers in the same file.**
- AES-128 encryption on assets where `type & 192 == 64`. Three hardcoded keys. Plus 9 hacker-only decoy rabbits planted to poison memory-reading shortcuts around the endgame cipher.
- Save = 479,360 B; 3 slots × 159,760 B; **~99% of a slot is three 52,800-byte 1-bit minimap bitmaps**. Everything else is dense bitfields (eggs = 64-bit, all 64 used). Single-byte XOR checksum; corrupting it deliberately has designed in-game consequences.

## Rendering
- **Rim light was the breakthrough** — "Immediately, when I got that shader working, I'm like, 'This is a unique look.'" Look preceded tone.
- Objection to commercial engines is a rendering argument: "they have point lights, they have a smooth gradient, and that clashes with pixel art." Lighting is thresholded/posterised with dithering.
- CRT: render at true 320×180; bilinear on X only; clamp Y to sharp pixel centres; **scanline falloff modulated per-pixel by luminosity** so it can't be replicated with an overlay. Validated on a Sony PVM at 240p using NTSC Genesis timing values.
- Full-screen **Navier–Stokes** solver on a dedicated compositing layer, running constantly. **Sprites can be drawn into it** (inject into density/velocity fields). Presentational, not mechanical.
- SDF raymarched backgrounds with secondary bounce ray — cut on Switch; fluid sim scaled down by reducing solver iterations.

## Size breakdown
- SteamDB: **33.26 MiB** install / 29.41 MiB download.
- Asset table 32,448 B (0.09%); overworld map at cap 1,797,256 B (≤5.2%); all 5 maps ~2 MiB (~6%); tile UV table 10,252 B.
- **All level+animation metadata ≈ 6%.** Remaining ~31 MB is PNG atlas + Ogg + code. No published per-type breakdown exists — *directly answerable* by patching the Redcrafter editor (it already enumerates all 676 assets with type and length).
- Audio pipeline (the useful confirmed part): originally authored *in* Ogg Vorbis (lossy source of truth — his stated mistake), switched to **uncompressed WAV sources + a build step that encodes each to Ogg** = one global quality knob, per-platform adjustable.

## Design philosophy

### Puzzles are a byproduct
> "I first typically come up with game mechanics… And then designing puzzles is the process of playing with the things I've made for myself and figuring out **where I get surprised** — finding those edge cases between seeing two things I've created interact — and then designing rooms to showcase those surprising bits… I don't really get inspirations for the puzzles. I get inspirations for the game mechanics, and then the puzzles kind of come as a result."

Documentary confirms ordering: systems first, world crafted afterwards to justify them.

### Two-pass item rule
1. **Inherent playfulness** — anybody can pick it up and enjoy it before knowing its purpose.
2. **Reinterpretation** — background interactions, secret abilities you stumble into.
> "It's almost like I'm designing the game three times over."

**Cut criterion:** items dropped when *redundant with an existing item*.

### Why toys not weapons
> "I wanted you to feel kind of disempowered… Didn't want to give you anything that felt explicitly like a weapon… And also, making game items toys, there's a lot of potential there for fun physics interactions."

Tone decision that produces a systems benefit. Yoyo string = maintained array of tile-corner intersection points; pivots about the last corner it caught.

### Four layers (declared Feb 2022, pre-launch, in his own byline)
- **L1** — the ending. 10–15h blind. Credits roll.
- **L2** — 64 eggs, true ending. "This is where most games stop." *Second* credits roll. **Platform achievements cap here.**
- **L3** — 16 secret rabbits → 64-digit BDTP cipher, key is UV chalkboard graffiti written as C-style pseudocode. Entry gate = Bunny Mural: **each copy holds a procedurally unique 1/50th of one image**; 50 players had to pool grids. Some rabbits need a barcode scanner or a physical printer.
- **L4** — "secrets that only I know." Cross-playthrough stunts (speedrun thresholds, real-world Groundhog Day, 100 berries undamaged). Payoff is spoken audio from Basso + publishers.

**Why nobody feels walled off:** every layer ends in real credits; deeper layers are *invisible not locked* (no greyed slots, no counter); achievements agree with the casual player.

**He lost the bet.** Marketing ARG's first stage solved in ~15 min ("hyenas tearing apart a carcass"). Secrets he expected to hold 10 years fell in ~a week. Conclusion: "in a future game, there's kind of no limit to how obscure you can make something."

**Production note:** the publisher's main creative contribution was *sanding down* the obscurity. Fanny pack was originally the reward for all eggs, as a deliberate anticlimax.

### Map: default-open, gate reactively
Objection: "Metroidvanias sometimes have an issue where there's a choke point… one key item, one door." Replacement = **knowledge as the gate**. Model is Mega Man not Metroid — order affects difficulty/solution-space, not access.

> "It almost started in the middle and then grew out to the boundaries… I would lean towards making the game more open at the start and add a lot of shortcuts, and then just watch people play… maybe I need to add another gate there, or make certain doors one-way doors… **There's no trick that I really know.**"

**Inverse of standard practice**: open everything, add gates only where playtesters actually got lost. One-way doors = the soft-gate primitive. Sequence breaks deliberately left in; L4 makes speedrunning a collectible gate.

### Wordlessness was an accident he kept
> "I don't have any way to sequence events in the editor itself. There's no dialogue. There's no cut scenes… because I didn't have a way to script those types of things, the game ended up having no dialogue and no cutscenes. **But I think it actually ended up being better for it.**"

Replacement model = Half-Life 2 attention direction (the distant Combine soldier firing harmlessly to turn the player's head).

Three teaching mechanisms:
- **Puzzles are the tutorial** — "the game never explicitly teaches you itself."
- **Locked camera is a reading instruction** — "encourages you to examine each screen as a whole and think about where something could be hidden." Plus "any black tile could be hiding something."
- **The affordance contract** — "I want people to always feel like their ideas are valid, or that the game is at least acknowledging them." *This is the justification for item redundancy: the world must respond to plausible hypotheses even when wrong, or players stop forming them.*

### Tone: dread from indifference
Horror or cozy? — "Yes."
> "It is dark. It is lonely. You don't belong in this world. It's not that it's a hostile world… it's just… not yours."
> "It just views you purely as a food source. Its personality is just kinda animalistic. **There's no malicious intent.**"

Dread from **scale mismatch and disinterest**, not aggression.

**Refuses the juice defaults:** "I've been actively avoiding using squash and stretch, and screen shake." Leans on procedural animation *because* it misbehaves — "having code drive animations often makes things look uncanny." And: "Moving a character's eye up or down one pixel can completely change their personality."

**The unifying sentence:** "I like to try to guide the player into thinking they fully understand something, and then reveal that they don't."

## What transfers to a vertical slice

1. **Choose an arbitrary hard ceiling first, treat it as a density mandate.** Room budget, tile cap, palette limit, byte size. When you hit it the only move left is density.
2. **Build 2–3 toys before designing any puzzle.** Test: fun to hold with no goal attached? Real-world objects with interesting physics arrive pre-loaded with behaviours.
3. **Play with your own toys; log where *you* get surprised.** Don't brainstorm puzzles — brainstorm mechanics, find collisions, build a room showcasing exactly one.
4. **Two-pass item test, then the redundancy cut.**
5. **Do not build a cutscene or dialogue system.** Deliberately. Cheapest way to buy the character.
6. **Open the whole map, add gates only where testers get lost.** One-way doors as soft gate; item gates last resort. Budget ~3× the rooms you keep.
7. **Every completion tier ends in real credits.** Visible empty slots = a checklist, not a well.
8. **Author normal maps as extra layers in the same art file.** What makes rim lighting affordable.

**Technical posture:** preallocate in nested scopes (global/session/room, wipe room arena on transition); collision + autotiling + lighting participation in tile flags; lossless source assets + build-step compression (one global quality knob); author at true internal resolution; skip pipelining if latency > throughput.

**Slice scope (≈1/10 of AW's proportions):**
- 5×5 = 25 rooms, one biome, connected to unbuilt edges so the world implies more
- 2 verbs, each with a second reading and ≥1 interaction with the other
- Layer 1 complete with an ending + one Layer 2 hook visible-but-unreachable

The half-layer is the part most likely to be under-built: something seen in the first ten minutes, unusable then, usable by the end, never announced.

**Acceptance test:** hand it to someone cold, no instructions. If they find a tool use you didn't build a room for → item design works. If they finish believing they finished, then spot what they missed → layer design works.

## Best remaining lead
GDC 2025 talk **"Developing at 5mb per Year: The Making of 'Animal Well'"** — Vault-gated, no public video. Abstract promises the visual/audio presentation techniques. Would likely close the audio-stack question and the exact size breakdown. Highest-value next purchase.

## Sources
Primary: [PS.Blog four-layer thesis](https://blog.playstation.com/2022/02/10/the-secrets-of-animal-well-coming-to-ps5/) · [SE Daily #1694](https://softwareengineeringdaily.com/2024/05/15/animal-well-with-billy-basso/) · [Wookash Podcast](https://www.youtube.com/watch?v=YngwUu4bXR4) · [Dev blog: 240p](https://animalwell.blogspot.com/2020/06/running-at-240p.html) · [Game Developer: one-byte room ID](https://www.gamedeveloper.com/programming/the-scratch-coding-and-discipline-at-the-heart-of-animal-well) · [Game Developer: engine](https://www.gamedeveloper.com/design/why-animal-well-s-home-brewed-engine-was-key-to-its-success) · [Game Developer: procedural animation](https://www.gamedeveloper.com/art/creature-feature-the-surreal-pixel-art-and-animation-of-animal-well) · [Thinky Games](https://thinkygames.com/features/interview-how-animal-well-is-using-secrets-and-mysteries-to-be-a-different-kind-of-metroidvania/) · [Time Extension](https://www.timeextension.com/features/best-of-2024-the-making-of-animal-well-2024s-most-unique-metroidvania) · [Second Wind documentary](https://www.youtube.com/watch?v=tffo3U4owwE) · [GDC Vault](https://gdcvault.com/play/1035082/Independent-Games-Summit-Developing-at)

Code/format: [Redcrafter/Animal-Well-editor](https://github.com/Redcrafter/Animal-Well-editor) · [Dregu/maxwell](https://github.com/Dregu/maxwell) · [awsgtools format wiki](https://github.com/Kein/awsgtools/wiki/Format-Description) · [animalwellsave](https://github.com/apocalyptech/animalwellsave) · [SteamDB](https://steamdb.info/app/813230/depots/) · [PCGamingWiki](https://www.pcgamingwiki.com/wiki/Animal_Well)

Community: [Wiki: BDTP](https://animalwell.wiki.gg/wiki/BDTP) · [Wiki: secret bunnies](https://animalwell.wiki.gg/wiki/Secret_bunnies) · [Kotaku: layers explained](https://kotaku.com/animal-well-guide-tips-layers-explained-1851532515) · [annabunches.net](https://annabunches.net/posts/2024-10-15-animal-well/) · [Steam traveler's guide](https://steamcommunity.com/sharedfiles/filedetails/?id=3244246482) · [Push Square: 50-player puzzle](https://www.pushsquare.com/news/2024/05/single-player-animal-well-has-a-secret-puzzle-that-requires-50-players-to-solve)
