"""Validate real hunter PCM and shared playback timing; never a listening pass."""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
WORK = ROOT / ".local/hunter-audio"
REPORT = ROOT / "docs/evidence/hunter/audio-tests.json"


def main():
    WORK.mkdir(parents=True, exist_ok=True)
    source = (ROOT / "src/audio.c").read_text()
    previous_bytes = subprocess.check_output(["git", "show", "8b52542:src/audio.c"], cwd=ROOT)
    # Match the repository's existing preservation convention: Git may check
    # this file out as CRLF on Windows. Only that encoding difference is allowed.
    assert (ROOT / "src/audio.c").read_bytes().replace(b"\r\n", b"\n").startswith(previous_bytes.replace(b"\r\n", b"\n")), "Pre-existing audio bytes must be an unchanged prefix apart from line endings"
    previous = previous_bytes.decode().replace("\r\n", "\n")
    clean, count = re.subn(r"\n// BEGIN HUNTER RESPONSE AUDIO\n.*?// END HUNTER RESPONSE AUDIO\n", "", source, flags=re.S)
    assert count == 1 and clean == previous, "Original audio/city code changed outside the explicit append"
    block = source[len(previous):]
    assert not re.search(r"\b(?:Rnd|Noise|AudioRnd|Sfx|Commit|Register|Reverb)\s*\(", block), "Hunter must not borrow original mutable synthesis/RNG"
    env = dict(os.environ, ZIG_GLOBAL_CACHE_DIR=str(ROOT / ".toolchain/zig-cache"))
    exe = WORK / "hunter-audio.exe"
    command = [str(ROOT / ".toolchain/zig-x86_64-windows-0.14.1/zig.exe"), "cc", "-std=c99", "-O1", "-g",
               "-fsanitize=undefined", "-fno-sanitize-recover=all", "-I", str(ROOT / ".toolchain/raylib-5.5_win64_mingw-w64/include"),
               str(ROOT / "tools/tests/hunter_audio.c"), str(ROOT / "tools/tests/raylib_stubs.c"), "-o", str(exe)]
    subprocess.run(command, cwd=ROOT, env=env, check=True)
    result = subprocess.run([str(exe), str(WORK)], cwd=ROOT, check=True, capture_output=True, text=True)
    print(result.stdout, end="")
    files = ["src/audio.c", "src/audio.h", "src/inhabitants.c", "src/inhabitants.h", "tools/tests/hunter_audio.c", "tools/tests/test_hunter_audio.py", "tools/tests/raylib_stubs.c"]
    report = {"status": "pass", "scope": "Technical synthesis, pitch-aware timing and inherited-state isolation; listening and full mix unverified",
              "sanitizer": "undefined, no recovery", "prior_audio_ref": "8b52542", "prior_audio_exact_after_marked_append_removal": True,
              "output": result.stdout.splitlines(), "sources": {f: hashlib.sha256((ROOT/f).read_bytes()).hexdigest() for f in files},
              "listening_candidates": [{"kind": n, "path": f".local/hunter-audio/hunter-{n}.wav", "sha256": hashlib.sha256((WORK/f"hunter-{n}.wav").read_bytes()).hexdigest()} for n in range(1, 7)],
              "pending": ["native device synchronization and latency", "actual listening and full-mix comfort", "integrated hunter rendering and state evidence", "human discovery acceptance"]}
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(report, indent=2) + "\n")


if __name__ == "__main__":
    main()
