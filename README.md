# The Vault and the City Under It — 3D presentation and city responses

This is the existing [AAnderson1817/AWellClaude](https://github.com/AAnderson1817/AWellClaude)
C/raylib game with a developing 3D presentation. Branch `codex/celestial-foundry` starts
from `claude/animal-well-redux-slice-oimk6l` at
[`99de7856bedfa059439da1b89435f54c98bddaf7`](https://github.com/AAnderson1817/AWellClaude/commit/99de7856bedfa059439da1b89435f54c98bddaf7).

The original two rooms, movement, lamp and stones, water, bulbs, wildlife, responsive
props and reset remain the game. A locked side view stages actual 3D geometry,
atmospheric backgrounds and foreground framing around the same collision plane.
Metallurgy, astronomy and faith inform the existing vault/city setting. The original
flat renderer remains available for comparison and now presents the same city responses.

The current increment implements the authored mural, reactive windows, drowned face
and eight-fish shoal. Light conceals the mural and douses windows; a sinking stone
receives an answer from the face; fish approach a floating lamp and scatter from the
body. These extend the existing Hold interaction without changing access or geometry.
See [response implementation and limits](docs/city-response-implementation.md).

The architecture revision adds editable Blender shelf assemblies, recessed façades,
turned columns and rooted foliage, with local contact shading derived from actual
visible geometry. The [art gallery](docs/art-direction.html) includes current game
frames, Blender source previews and GLB links alongside the 60 reference images.
The [independent review](docs/artistic-review-v12.md) still requires substantial
material and hero-asset refinement before the requested AAA target is met.

**Status:** a playable presentation development branch. AAA visual quality and human
immersion have not been certified. The scope is the existing Vault Mouth and Drowned
Quarter; it is not a finished 25-room game. The city increment passes native Windows
preservation and response tests and compiles for the web. Earlier depth-v10 browser
checks are retained as historical evidence; current browser coverage is recorded
separately in [verification](docs/verification.md). The hunter, native, remaining
response-table content, full browser playthrough, perceptual audio review and Linux
validation remain open.

## Windows

The Windows setup downloads a portable compiler and raylib into the ignored
`.toolchain` directory; it does not install system tools or change PATH. Run from
PowerShell:

```powershell
./tools/setup-win.ps1
./tools/build-win.ps1
./build/game.exe
```

The executable does not require Python or Blender. Python 3 is used for development
verification and asset generation; Blender is used to author the editable assets.

```powershell
./tools/check-win.ps1
```

This rebuilds a headless game, runs sanitizer regressions, compares against the original
Git source, and searches traversal and escape routes. It writes evidence to
`build/verification`. Use `-Python 'C:/path/to/python.exe'` if Python is not discovered.
To additionally rebuild both native renderers and compare actual rendered runs:

```powershell
./tools/check-win.ps1 -Render
```

See [verification and evidence](docs/verification.md) for what these results prove and
what remains open.

### Web build from Windows

With Emscripten 6.0.8 installed in `.toolchain/emsdk-main` and the raylib 5.5 source in
`.toolchain/raylib-5.5`:

```powershell
./tools/build-web-win.ps1
```

This compiles raylib's web library, keeps the original Emscripten build flags, and
produces `build/game.js` plus self-contained `build/play.html`. It activates the SDK
locally when necessary and does not change the system PATH. `-RebuildRaylib` rebuilds
the cached library. The native setup command does not provision these separate web
dependencies.

The portable wrapper is also available directly:

```text
python tools/web/wrap.py build/game.js build/play.html "The Vault and the City Under It"
```

Its Bash entry point remains `tools/web/wrap.sh`; it no longer depends on a hardcoded
checkout path. Serve the checkout over localhost for browser review. The self-contained
page embeds the WASM and scene data, so the game does not stream asset files.

## Controls

These controls are documented here for development and review. Normal play keeps the
project's wordless interface.

| Input | Action |
|---|---|
| Left/right arrows or A/D | Move |
| Z, Space or K | Jump; hold for a higher jump |
| Down arrow or S | Drop through a one-way shelf |
| Up arrow/W or held jump, in water | Swim upward |
| X | Pick up or set down the lamp or a stone; one object at a time |
| Hold R | Close your eyes and reset; release early to cancel |
| F2 | Switch between the 3D and flat renderer |
| F4 | Toggle optional ambient drift in 3D; gameplay remains live |
| L, in flat mode | Toggle development surface labels |

Landing on a bulb bounces the body; a fresh jump press near contact strengthens the
bounce. A carried stone changes vertical movement and buoyancy, while the lamp floats.

Presentation can also be selected at launch:

```powershell
./build/game.exe --flat
./build/game.exe --depth --still
./build/game.exe --mute
```

## Existing build targets

The repository's original Bash pipeline is preserved:

```bash
tools/setup.sh
tools/build.sh linux   # also: web, win, all
tools/check.sh
```

It uses its own raylib/Emscripten paths and dependencies described in
[tools/TOOLCHAIN.md](tools/TOOLCHAIN.md). The new Windows setup is an additional native
development path; it does not replace these targets. Older toolchain notes describe
their historical environment, not validation of this branch's new 3D renderer.

## Design and production records

The environment reference collections are complete: **30 images for the Vault Mouth
and 30 for the Drowned Quarter**, with prompts, review notes, dimensions and hashes in
their manifests. Browse the [art-direction gallery](docs/art-direction.html) to compare
the 60 references with current native game captures. This completes the reference
count requirement; runtime AAA quality and human immersion remain open.

* [Existing design law](claude/DESIGN-LAW.md) and [premise](claude/PREMISE.md).
* [Principles, primary-source research and preservation audit](docs/design-principles.md).
* [Current production status](docs/production-status.md) and [city response evidence](docs/city-response-implementation.md).
* [Production brief](docs/production-brief.md), [asset contract](docs/asset-contract.md)
  and [asset acceptance](docs/asset-acceptance.md).
* Environment references live under `public/art/references`; editable assets and
  rendered review evidence live under `assets`.

Reference art establishes direction. A reference image, successful trace or exported
mesh is not, by itself, evidence that the finished scene meets the visual or immersion
goal.
