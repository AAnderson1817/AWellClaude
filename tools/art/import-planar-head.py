"""Extract or verify the pinned CC0 Head - Planar asset in isolated Blender.

blender --background --factory-startup --disable-autoexec -t 8 \
  --python-exit-code 1 --python tools/art/import-planar-head.py -- \
  --source PATH/human_base_meshes_bundle.blend --verify-only

The upstream bundle is acquired separately. This script has no network access.
"""
import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

import bpy

ROOT = Path(__file__).resolve().parents[2]
OBJECT = "Head - Planar"
AUTHOR = "Paul Kotelevets"
SOURCE_SHA = "3c121505651140ceb4d69fd1d8923f7788ffadd81672f5be14845a5f2c75c137"
GEOMETRY_SHA = "b40e4d1a4f15dd952ad24712858c0237ec0a54f7c80fd3cc0c55826c4b6a9d7d"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read_object(path):
    with bpy.data.libraries.load(str(path), link=False) as (available, loaded):
        if OBJECT not in available.objects:
            raise ValueError(f"Missing {OBJECT!r} in {path}")
        loaded.objects = [OBJECT]
    head = loaded.objects[0]
    if not head.asset_data or (head.asset_data.author, head.asset_data.license) != (AUTHOR, "CC0"):
        raise ValueError("Unexpected author/license metadata")
    if (len(head.data.vertices), len(head.data.polygons)) != (318, 316):
        raise ValueError("Unexpected control mesh")
    return head


def evaluated_record(label, path):
    head = read_object(path)
    scene = bpy.data.scenes.new("VERIFY | " + label)
    scene.collection.objects.link(head)
    bpy.context.window.scene = scene
    modifier = next(m for m in head.modifiers if m.type == "MULTIRES")
    if modifier.total_levels != 5:
        raise ValueError("Expected all five authored multiresolution levels")
    # Change only this in-memory verification copy; never resave the vendor file.
    modifier.levels = 4
    bpy.context.view_layer.update()
    deps = bpy.context.evaluated_depsgraph_get()
    mesh = bpy.data.meshes.new_from_object(head.evaluated_get(deps), depsgraph=deps)
    mesh.calc_loop_triangles()
    data = b"".join(struct.pack("<3f", *v.co) for v in mesh.vertices)
    data += b"".join(struct.pack("<3I", *t.vertices) for t in mesh.loop_triangles)
    record = {
        "source": label,
        "vertices": len(mesh.vertices),
        "triangles": len(mesh.loop_triangles),
        "evaluated_level": 4,
        "geometry_sha256": hashlib.sha256(data).hexdigest(),
    }
    bpy.data.meshes.remove(mesh)
    if record["geometry_sha256"] != GEOMETRY_SHA:
        raise ValueError(f"Evaluated geometry differs from pinned source: {record}")
    return record


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--out", type=Path, default=ROOT / "assets/vendor/blender/head-planar-1.4.1")
    parser.add_argument("--verify-only", action="store_true")
    parser.add_argument("--report", type=Path, help="Optional verification report destination")
    args = parser.parse_args(sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else [])
    source = args.source.resolve()
    if sha(source) != SOURCE_SHA:
        raise ValueError("Upstream Blender bundle SHA256 mismatch")
    asset = args.out / "head-planar.blend"
    if not args.verify_only:
        if asset.exists():
            raise FileExistsError(f"Refusing to replace existing vendor selection: {asset}")
        args.out.mkdir(parents=True, exist_ok=True)
        head = read_object(source)
        bpy.data.libraries.write(str(asset), {head}, path_remap="RELATIVE", fake_user=True, compress=True)
        provenance = {
            "upstream_catalog": "https://www.blender.org/download/demo-files/",
            "upstream_archive_url": "https://mirror.blender.org/demo/asset-bundles/human-base-meshes/human-base-meshes-bundle-v1.4.1.zip",
            "upstream_archive_sha256": "811f43accbb31a88266d932f8f5563b2d13586fca0ba2693aad1f5fe582b3515",
            "upstream_blend_sha256": SOURCE_SHA,
            "selected_object": OBJECT,
            "author": head.asset_data.author,
            "license": head.asset_data.license,
            "selection_method": "Blender library write of the unmodified selected object and its dependencies; no geometry, modifiers or source material edits",
            "blender_version": bpy.app.version_string,
            "base_vertices": len(head.data.vertices),
            "base_polygons": len(head.data.polygons),
            "modifiers": [{"name": m.name, "type": m.type, "levels": getattr(m, "levels", None),
                           "total_levels": getattr(m, "total_levels", None)} for m in head.modifiers],
            "asset_sha256": sha(asset),
            "asset_bytes": asset.stat().st_size,
        }
        (args.out / "provenance.json").write_text(json.dumps(provenance, indent=2) + "\n")
    provenance = json.loads((args.out / "provenance.json").read_text())
    if sha(asset) != provenance["asset_sha256"]:
        raise ValueError("Selected asset differs from its provenance hash")
    report = {"status": "pass", "blender_version": bpy.app.version_string,
              "script_sha256": sha(Path(__file__)), "selected_asset_sha256": sha(asset),
              "records": [evaluated_record("upstream", source), evaluated_record("selected", asset)]}
    destination = args.report or args.out / "selection-verification.json"
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
