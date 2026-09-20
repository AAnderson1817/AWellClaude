#!/usr/bin/env python3
"""Compare this presentation branch against the actual inherited C simulation.

Use --build-baseline on Windows to compile the pinned Git source with the same
portable compiler and I/O stubs. --render instead compares real native rendered
frames; pass the original native executable with --baseline for that mode.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
BASELINE_REF = "99de785"
CASES = [
    ("quiet-vault", ["--play", "-:3600"]),
    ("quiet-water", ["--room", "1", "--play", "-:3600"]),
    ("run-jump-release", ["--play", "R:20,RJ:18,-:30,L:30,LJ:3,-:70"]),
    ("hold-lamp", ["--play", "L:6,X:1,-:8,R:34,X:1,-:180"]),
    ("bulb-normal", ["--at", "12,16", "--play", "-:180"]),
    ("bulb-timed", ["--at", "12,16", "--play", "-:15,J:1,-:180"]),
    ("shaft-down", ["--at", "22,19", "--play", "-:20,D:300"]),
    ("shaft-up", ["--room", "1", "--at", "22,0", "--play", "-:10,J:30,-:60"]),
    ("failed-seam-exit", ["--room", "1", "--at", "22,0", "--play", "-:6,J:6,-:80"]),
    ("water-to-island", ["--room", "1", "--at", "22,3", "--play", "-:110,L:64,LJ:12,L:6,-:60"]),
    ("stone-dive-and-reset", ["--room", "1", "--at", "5,5", "--play", "-:4,X:1,-:4,R:34,-:170,H:90,-:60"]),
    ("abort-reset", ["--at", "36,13", "--play", "-:4,X:1,-:20,H:40,-:40"]),
    ("camp-lamp-fire", ["--lamp", "0,16,19", "--at", "16,19", "--play", "-:600"]),
    ("door-glint", ["--lamp", "0,3,11", "--play", "-:300"]),
    ("animal-and-pot", ["--at", "31,13", "--play", "R:36,L:36,RJ:14,L:40,-:300"]),
]

def command(args, **kwargs):
    result = subprocess.run([str(a) for a in args], cwd=ROOT, capture_output=True, **kwargs)
    if result.returncode:
        stderr = result.stderr.decode(errors="replace") if isinstance(result.stderr, bytes) else result.stderr
        raise RuntimeError(f"Command failed ({result.returncode}): {' '.join(map(str, args))}\n{stderr}")
    return result.stdout

def original(path):
    return command(["git", "show", f"{BASELINE_REF}:{path}"])

def build_baseline(output, render=False):
    if os.name != "nt":
        raise RuntimeError("Automatic baseline compilation uses the portable Windows toolchain; otherwise pass --baseline.")
    folder = ROOT / ".local" / f"baseline-{BASELINE_REF}"
    source = folder / "src"
    source.mkdir(parents=True, exist_ok=True)
    names = command(["git", "ls-tree", "-r", "--name-only", BASELINE_REF, "src"], text=True).splitlines()
    for name in names:
        if Path(name).suffix in (".c", ".h"):
            (source / Path(name).name).write_bytes(original(name))
    zig = ROOT / ".toolchain/zig-x86_64-windows-0.14.1/zig.exe"
    include = ROOT / ".toolchain/raylib-5.5_win64_mingw-w64/include"
    env = dict(os.environ, ZIG_GLOBAL_CACHE_DIR=str(ROOT / ".toolchain/zig-cache"))
    sources = sorted(source.glob("*.c"))
    libraries = []
    if render:
        libraries = [include.parent / "lib/libraylib.a", "-lopengl32", "-lgdi32", "-lwinmm"]
    else:
        sources.append(ROOT / "tools/tests/raylib_stubs.c")
    output.parent.mkdir(parents=True, exist_ok=True)
    command([zig, "cc", "-std=c99", "-O2", *sources,
             "-I", include, *libraries, "-o", output], env=env)

def contract_checks():
    # These are the signed-off gameplay modules. A future intentional mechanics
    # change should update the contract explicitly instead of silently blessing it.
    for path in ("src/player.c", "src/items.c"):
        assert (ROOT / path).read_bytes().replace(b"\r\n", b"\n") == original(path).replace(b"\r\n", b"\n"), path
    audio = (ROOT / "src/audio.c").read_text()
    city_audio = re.search(r"    // BEGIN CITY RESPONSE SOUNDS\n(.*?)    // END CITY RESPONSE SOUNDS\n", audio, re.S)
    assert city_audio and not re.search(r"\b(?:Noise|Rnd|AudioRnd)\s*\(", city_audio.group(1)), "City synthesis must not consume the inherited random stream"
    # Permit only the marked appended synthesis block. All original synthesis,
    # runtime audio code and random-stream calls remain byte-for-byte identical.
    audio, removed = re.subn(r"    // BEGIN CITY RESPONSE SOUNDS\n.*?    // END CITY RESPONSE SOUNDS\n", "", audio, count=1, flags=re.S)
    assert removed == 1, "Expected one appended city audio block"
    assert audio == original("src/audio.c").decode().replace("\r\n", "\n"), "Original audio behavior changed"
    # Permit exactly the detached presentation accessor in fx.c, preserving every
    # original effect update/draw line and the random-stream behavior verbatim.
    effects = (ROOT / "src/fx.c").read_text()
    effects, removed = re.subn(r"\nint FxViews\(Particle \*out, int max\) \{.*?\n\}\n", "\n", effects, count=1, flags=re.S)
    assert removed == 1, "Expected one FxViews accessor"
    clean = lambda text: re.sub(r"\n{3,}", "\n\n", text.replace("\r\n", "\n")).strip()
    assert clean(effects) == clean(original("src/fx.c").decode()), "Original effects behavior changed"
    for path, getters, enum_names in (
        ("src/life.c", ("LifeBirdViews", "LifeBeastView", "LifePlantViews"), ("B_PERCH", "M_WALK", "P_IDLE")),
        ("src/props.c", ("PropsViews",), ("PR_NONE", "POT_REST")),
    ):
        before = original(path).decode()
        after = (ROOT / path).read_text()
        for name in getters:
            after, removed = re.subn(rf"\nint {name}\([^\n]*\) \{{.*?\n\}}\n", "\n", after, count=1, flags=re.S)
            assert removed == 1, f"Expected one {name} accessor"
        for name in enum_names:
            before, removed = re.subn(rf"enum \{{ {name}\b.*?\}};\r?\n", "", before, count=1, flags=re.S)
            assert removed == 1, f"Missing baseline enum {name}"
        assert clean(before) == clean(after), f"Original life/prop behavior changed: {path}"
    for path, marker in (("src/room.c", "MAPS"), ("src/props.c", "PROPS")):
        pattern = rf"static const char \*{marker}\[ROOM_COUNT\]\[RH\] = \{{(.*?)\n\}};"
        before = re.search(pattern, original(path).decode(), re.S)
        after = re.search(pattern, (ROOT / path).read_text(), re.S)
        assert before and after, f"Missing map declaration: {path}"
        # C comments/indentation can change; authored tile/prop rows cannot.
        assert re.findall(r'"([^"\n]+)"', before.group(1)) == re.findall(r'"([^"\n]+)"', after.group(1)), marker

def trace(executable, args, render, mode):
    options = [*args, "--trace", "--mute", "--scale", "1"]
    if not render: options.append("--nodraw")
    if mode: options.append(mode)
    startup = None
    if os.name == "nt":
        startup = subprocess.STARTUPINFO()
        startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
        startup.wShowWindow = 0
    output = command([executable, *options], text=True, startupinfo=startup)
    lines = [line for line in output.splitlines() if line.startswith(("f=", "LIFE ", "SFX "))]
    assert any(line.startswith("f=") for line in lines), f"No simulation trace from {executable}"
    city = re.findall(r"^CITY SFX city-murmur=(\d+) city-hum=(\d+)$", output, re.M)
    assert len(city) <= 1, "Duplicate city sound report"
    return lines, ({"city-murmur": int(city[0][0]), "city-hum": int(city[0][1])} if city else None)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline", type=Path, default=ROOT / "build/game-baseline-probe.exe")
    parser.add_argument("--candidate", type=Path, default=ROOT / "build/game-probe.exe")
    parser.add_argument("--build-baseline", action="store_true")
    parser.add_argument("--render", action="store_true")
    parser.add_argument("--case", action="append", choices=[name for name, _ in CASES],
                        help="Run only these named cases; repeat to select more than one.")
    parser.add_argument("--report", type=Path, default=ROOT / "build/preservation-report.json")
    args = parser.parse_args()
    if args.build_baseline: build_baseline(args.baseline, render=args.render)
    contract_checks()
    results = []
    for name, options in CASES:
        if args.case and name not in args.case: continue
        # Actual GPU smoke checks focus on meaningful transitions without thousands
        # of expensive idle frames. Headless checks retain the full calm-time sample.
        if args.render:
            options = [re.sub(r"-:(3600|600)", "-:180", part) for part in options]
        expected, baseline_city = trace(args.baseline.resolve(), options, args.render, None)
        assert baseline_city is None, "Historical baseline unexpectedly contains city responses"
        city_counts = {}
        for mode in ("--flat", "--depth"):
            actual, city_counts[mode[2:]] = trace(args.candidate.resolve(), options, args.render, mode)
            assert city_counts[mode[2:]] is not None, "Candidate did not report its separate city sounds"
            assert len(actual) == len(expected), f"{name}/{mode}: trace length changed"
            for index, (before, after) in enumerate(zip(expected, actual)):
                if before != after:
                    raise AssertionError(f"{name}/{mode}, trace line {index}:\nBASE {before}\nNEW  {after}")
        digest = hashlib.sha256("\n".join(expected).encode()).hexdigest()
        frames = sum(line.startswith("f=") for line in expected)
        assert city_counts["flat"] == city_counts["depth"], "City sound events depend on presentation mode"
        results.append({"case": name, "frames": frames, "sha256": digest, "modes": ["flat", "depth"], "city_sound_counts": city_counts})
        print(f"PASS {name}: {frames} original frames match both presentation modes")
    report = {"baseline_git_ref": BASELINE_REF, "rendered": args.render,
              "unchanged_contract": ["movement", "items", "fx behavior", "life behavior", "props behavior", "original audio", "room geometry", "prop placements"],
              "intentional_extension": "City responses add separately reported murmurs and hums; original dbgLastSfx retains its inherited gameplay-event meaning. No original trace fields are filtered.",
              "cases": results, "compared_frames": 2 * sum(r["frames"] for r in results)}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + "\n")
    print(f"PASS {len(results)} cases / {report['compared_frames']} compared frames; {args.report}")

if __name__ == "__main__":
    try: main()
    except (AssertionError, RuntimeError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        sys.exit(1)
