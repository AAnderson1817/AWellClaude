# Blender Human Base Meshes: Head - Planar

**Paul Kotelevets**, distributed by Blender in **Human Base Meshes 1.4.1** under
**CC0**. The author's name and license are also embedded in the selected object's
asset metadata. See the [official catalog](https://www.blender.org/download/demo-files/)
and [bundle directory](https://download.blender.org/demo/asset-bundles/human-base-meshes/).

`head-planar.blend` contains the unmodified **Head - Planar** object and its
dependencies, extracted using Blender's library writer. It is a repackaged
selection, not a byte-identical copy of the whole upstream bundle. Its original
318-vertex / 316-polygon control mesh and all five multiresolution levels remain
editable. This is source anatomy for the carved-face study; it is not a finished
game asset and is not currently used by the production renderer.

`provenance.json` pins the original archive, original Blender file, selected file,
author, license, Blender version and selection method. `selection-verification.json`
records independent fresh loads of the original and selected files: both produce
80,898 vertices and 161,792 triangles at multiresolution level 4, with identical
ordered float32 positions and triangle indices. That check verifies the selection;
it makes no claim about artistic acceptance of an adaptation.

To reproduce, obtain the pinned archive URL in `provenance.json`, verify its SHA256,
and extract `human_base_meshes_bundle.blend`. Run in a separate Blender process:

```text
blender --background --factory-startup --disable-autoexec -t 8 --python-exit-code 1 --python tools/art/import-planar-head.py -- --source PATH/human_base_meshes_bundle.blend --verify-only
```

To make a new selection, omit `--verify-only` and pass `--out` with an empty output
directory. The script refuses to replace an existing asset. Blender file bytes can
vary on resave; evaluated geometry is pinned separately. No texture dependencies
or external service are required. Do not run the importer inside a user's live scene.
