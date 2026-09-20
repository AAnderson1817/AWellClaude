"""Exercise the integrated ItemsStep/Hunter module with UBSan and exact idle traces.

This uses the delivered source directly; it is not a substitute for native play.
"""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
WORK = ROOT / ".local" / "hunter-tests"
REPORT = ROOT / "docs" / "evidence" / "hunter" / "integrated-module-tests.json"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    WORK.mkdir(parents=True, exist_ok=True)
    source = (ROOT / "src/items.c").read_text()
    needle = "        if (!InhabitantsPinsItem(i)) Fall(it);"
    if source.count(needle) != 1:
        raise RuntimeError("ItemsStep changed: inspect delivered ownership guard")
    zig = ROOT / ".toolchain/zig-x86_64-windows-0.14.1/zig.exe"
    include = ROOT / ".toolchain/raylib-5.5_win64_mingw-w64/include"
    env = dict(os.environ, ZIG_GLOBAL_CACHE_DIR=str(ROOT / ".toolchain/zig-cache"))
    exe = WORK / "hunter-responses.exe"
    names = ["player", "room", "fx", "render", "audio", "life", "props", "city", "inhabitants"]
    command = [str(zig), "cc", "-std=c99", "-O1", "-g", "-DAWELL_HEADLESS", "-fsanitize=undefined", "-fno-sanitize-recover=all",
               "-I", str(include), "-I", str(ROOT / "src"), str(ROOT / "tools/tests/hunter_responses.c"),
               str(ROOT / "tools/tests/raylib_stubs.c"), str(ROOT / "src/items.c"), *[str(ROOT / f"src/{name}.c") for name in names], "-o", str(exe)]
    subprocess.run(command, cwd=ROOT, env=env, check=True)
    result = subprocess.run([str(exe)], cwd=ROOT, check=True, text=True, capture_output=True)
    print(result.stdout, end="")
    traces = [subprocess.run([str(exe), option], cwd=ROOT, check=True, capture_output=True).stdout
              for option in ("--disabled", "--enabled")]
    if traces[0] != traces[1]:
        for name, trace in zip(("disabled", "enabled"), traces):
            (WORK / f"original-{name}.txt").write_bytes(trace)
        raise AssertionError("Idle original-state comparison differs; exact traces retained in .local/hunter-tests")
    print("PASS 6,000 exact original-state frame hashes: original three items, player, prop state/timers, map and original Sfx counters")
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    report = {"status": "passed", "scope": "delivered hunter and ItemsStep modules; native and full-game checks recorded separately",
              "sanitizer": "undefined, no recovery", "original_idle_frames_exact": 6000,
              "original_idle_trace_sha256": hashlib.sha256(traces[0]).hexdigest(),
              "intentional_state_extension": "four appended real stones; itemCount 3 -> 7; authored offers/pickups may change original nearest-item selection",
              "adapter": "none: directly compiled src/items.c with the one ownership guard",
              "output": result.stdout.splitlines(),
              "sources": {str(path.relative_to(ROOT)).replace("\\", "/"): sha(path) for path in
                          [ROOT / "src/inhabitants.c", ROOT / "src/inhabitants.h", ROOT / "src/hunter_pose.h", ROOT / "src/items.c", ROOT / "src/props.c",
                           ROOT / "tools/tests/hunter_responses.c", Path(__file__).resolve()]},
              "pending": ["see separate full-game baseline and authored-interaction differential", "native presentation", "perceptual listening", "human discovery/immersion review"]}
    REPORT.write_text(json.dumps(report, indent=2) + "\n")


if __name__ == "__main__":
    main()
