"""Capture native hunter gestures from the verified CLI input scenarios.

Run after building game.exe and test_hunter_integration.py. The calling process
sets its own resource affinity; the game retains its 60 fps presentation ceiling.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import runpy
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def source_hashes():
    paths = sorted((ROOT / "src").rglob("*.c")) + sorted((ROOT / "src").rglob("*.h"))
    return {p.relative_to(ROOT).as_posix():sha(p) for p in paths}


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--exe", type=Path, default=ROOT / "build/game.exe")
    ap.add_argument("--trace-report", type=Path, default=ROOT / "docs/evidence/hunter/headless-suite/hunter-full-game.json")
    ap.add_argument("--output", type=Path, default=ROOT / "build/hunter-review")
    args = ap.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    verified = json.loads(args.trace_report.read_text(encoding="utf-8"))
    assert verified["status"] == "passed"
    inputs = runpy.run_path(str(ROOT / "tools/tests/test_hunter_integration.py"))["CASES"]
    by_name = {c["case"]:c for c in verified["cases"]}
    tend = next(t["frame"] for t in by_name["idle-tending"]["transitions"] if t["state"] == "5")
    carry = next(t["frame"] for t in by_name["return-nearby"]["transitions"] if t["state"] == "3")
    place = next(t["frame"] for t in by_name["return-nearby"]["transitions"] if t["placed"] == "1")
    gestures = [2, 10, carry+10, place-19, place+9]
    cases = [
        ("vault", [], [120]),
        ("drowned", ["--room", "1", "--at", "22,3"], [120]),
        ("return", inputs["return-nearby"], gestures),
        ("fire", inputs["fire-conversation"], [8,126,160,260]),
        ("tend", inputs["idle-tending"], [tend,tend+24,tend+48,tend+72,tend+96]),
        ("flat-return", ["--flat", *inputs["return-nearby"]], gestures),
    ]
    startup = None
    if os.name == "nt":
        startup = subprocess.STARTUPINFO()
        startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
        startup.wShowWindow = 0
    sources = source_hashes()
    # A different native executable is expected, but the simulated C/header
    # source must match the completed input-loop evidence used to select poses.
    for path, digest in verified["source_sha256"].items():
        if path.startswith("src/"):
            assert sources[path] == digest, f"Re-run input-loop evidence after source change: {path}"
    records = []
    for name, options, shots in cases:
        folder = out / name
        folder.mkdir(exist_ok=True)
        cmd = [str(args.exe.resolve()), *options, "--scale", "6", "--still", "--mute", "--shots", ",".join(map(str,shots)), "--out", str(folder)]
        result = subprocess.run(cmd,cwd=ROOT,startupinfo=startup,capture_output=True,text=True,check=True)
        (folder/"stdout.txt").write_text(result.stdout,encoding="utf-8")
        (folder/"stderr.txt").write_text(result.stderr,encoding="utf-8")
        assert not result.stderr.strip(), result.stderr
        records.append({"case":name,"arguments":cmd[1:],"images":{f"f{frame:04}.png":sha(folder/f"f{frame:04}.png") for frame in shots}})
        print(f"Captured {name}",flush=True)
    montage = out/"hunter-voice-montage.wav"
    subprocess.run([str(args.exe.resolve()),"--hunter-wav",str(montage),"--scale","1"],cwd=ROOT,startupinfo=startup,capture_output=True,check=True)
    assert source_hashes() == sources, "Sources changed during capture; do not publish a mixed revision"
    report = {"capturedUtc":datetime.now(timezone.utc).isoformat(),"exeSHA256":sha(args.exe),"sourceHashes":sources,"fixtureSHA256":sha(Path(__file__)),
              "inputReportSHA256":sha(args.trace_report),"cases":records,"voiceMontageSHA256":sha(montage),
              "scope":"Actual native CLI input loop; selected states from complete-item trace. Normal 60fps ceiling. These stills and isolated WAV do not establish live perceptual or human discovery acceptance."}
    (out/"manifest.json").write_text(json.dumps(report,indent=2)+"\n",encoding="utf-8")


if __name__ == "__main__":
    main()
