"""Build real ItemsStep with the proposed single Fall guard, without editing it.

This isolated adapter exercises the exact Hold loop awaiting root integration.
It is not the full-game preservation suite or a substitute for native play review.
"""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
WORK = ROOT / ".local" / "hunter-tests"
REPORT = ROOT / "docs" / "evidence" / "hunter" / "module-tests.json"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    WORK.mkdir(parents=True, exist_ok=True)
    source = (ROOT / "src/items.c").read_text()
    needle = "        Fall(it);"
    if source.count(needle) != 1:
        raise RuntimeError("ItemsStep changed: inspect proposed guard instead of silently adapting it")
    adapted = '#include "inhabitants.h"\n' + source.replace(needle, "        if (!InhabitantsPinsItem(i)) Fall(it);")
    items_copy = WORK / "items.c"
    items_copy.write_text(adapted)
    zig = ROOT / ".toolchain/zig-x86_64-windows-0.14.1/zig.exe"
    include = ROOT / ".toolchain/raylib-5.5_win64_mingw-w64/include"
    env = dict(os.environ, ZIG_GLOBAL_CACHE_DIR=str(ROOT / ".toolchain/zig-cache"))
    exe = WORK / "hunter-responses.exe"
    names = ["player", "room", "fx", "render", "audio", "life", "props", "city", "inhabitants"]
    command = [str(zig), "cc", "-std=c99", "-O1", "-g", "-DAWELL_HEADLESS", "-fsanitize=undefined", "-fno-sanitize-recover=all",
               "-I", str(include), "-I", str(ROOT / "src"), str(ROOT / "tools/tests/hunter_responses.c"),
               str(ROOT / "tools/tests/raylib_stubs.c"), str(items_copy), *[str(ROOT / f"src/{name}.c") for name in names], "-o", str(exe)]
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
    report = {"status": "passed", "scope": "additive module and proposed guard; not yet integrated native game",
              "sanitizer": "undefined, no recovery", "original_idle_frames_exact": 6000,
              "original_idle_trace_sha256": hashlib.sha256(traces[0]).hexdigest(),
              "intentional_state_extension": "four appended real stones; itemCount 3 -> 7; authored offers/pickups may change original nearest-item selection",
              "adapter": "isolated items.c copy: include inhabitants.h; guard only Fall(it); original Hold loop unchanged",
              "output": result.stdout.splitlines(),
              "sources": {str(path.relative_to(ROOT)).replace("\\", "/"): sha(path) for path in
                          [ROOT / "src/inhabitants.c", ROOT / "src/inhabitants.h", ROOT / "src/items.c", ROOT / "src/props.c",
                           ROOT / "tools/tests/hunter_responses.c", Path(__file__).resolve()]},
              "pending": ["main/build hooks", "full-game baseline and authored-interaction differential", "native presentation", "actual synthesized voices/listening", "human discovery/immersion review"]}
    REPORT.write_text(json.dumps(report, indent=2) + "\n")


if __name__ == "__main__":
    main()
