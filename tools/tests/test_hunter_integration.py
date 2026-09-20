"""Exercise authored hunter interactions through the real executable's input loop.

No simulation state is injected. Every item, owner and home is recorded alongside
the inherited trace; intentional differences from the three-item baseline remain
visible. The separate UBSan fixture covers direct competing-action edge cases.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
CASES = {
    "take-cairn": ["--at", "15,19", "--play", "-:2,X:1,-:80"],
    "return-nearby": ["--at", "15,19", "--play", "-:2,X:1,-:8,L:8,X:1,L:24,-:400"],
    "carry-away": ["--at", "15,19", "--play", "-:2,X:1,-:8,L:42,X:1,-:300"],
    "reset-away": ["--at", "15,19", "--play", "-:2,X:1,-:8,L:42,X:1,-:30,H:90,-:80"],
    "idle-tending": ["--play", "-:2600"],
    "fire-conversation": ["--lamp", "0,16,19", "--at", "10,19", "--play", "-:500"],
}


def execute(exe, arguments, mode, render=False, hunter=True):
    start = None
    if __import__("os").name == "nt":
        start = subprocess.STARTUPINFO()
        start.dwFlags |= subprocess.STARTF_USESHOWWINDOW
        start.wShowWindow = 0
    command = [str(exe.resolve()), *arguments, "--trace", "--mute", "--scale", "1"]
    if mode:
        command.append(mode)
    if hunter:
        command.append("--trace-hunter")
    if not render:
        command.append("--nodraw")
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, check=True, startupinfo=start)
    assert not result.stderr.strip(), result.stderr
    return result.stdout


def fields(line):
    return dict(re.findall(r"(\w+)=([^ ]*)", line))


def inspect(output):
    frames = {}
    for line in output.splitlines():
        if line.startswith("HUNTER f="):
            row = fields(line)
            frames[int(row["f"])] = {"hunter": row, "items": {}}
        elif line.startswith("ITEM f="):
            row = fields(line)
            frames[int(row["f"])]["items"][int(row["id"])] = row
    assert frames, "No complete-item trace; integration hooks absent"
    homes = None
    taken_before = placed_before = 0
    for frame in frames.values():
        h, items = frame["hunter"], frame["items"]
        assert sorted(items) == list(range(7)), "An item was created or lost"
        current = [it["home"] for it in items.values()]
        if homes is None:
            homes = current
        assert current == homes, "An original or cairn stone home changed"
        members = [int(x) for x in h["members"].split(",") if x]
        assert len(members) == int(h["count"]) and len(set(members)) == len(members)
        held, carried = int(h["holds"]), int(h["carried"])
        if int(h["taken"]) > taken_before:
            assert h["voice"] == "3", "Sharp acknowledgment arrived after its action"
        if int(h["placed"]) > placed_before:
            assert h["voice"] == "2", "Placement phrase arrived after its gesture"
        if h["voice"] == "5":
            assert h["state"] == "8", "Stale bedroll utterance outside bedroll attention"
        if h["voice"] == "6":
            assert h["state"] in ("0", "1", "7"), "Idle hum started during an object response"
        taken_before, placed_before = int(h["taken"]), int(h["placed"])
        assert held < 0 or (held not in members and held != carried), "Two owners of the held stone"
        assert carried < 0 or carried not in members, "Carried stone still stacked"
        for i, it in items.items():
            assert all(math.isfinite(float(it[key])) for key in ("x", "y", "vy"))
            if h["visible"] == "1":
                expected_pin = int((i in members or i == carried) and i != held and it["room"] == "0")
                assert int(it["pin"]) == expected_pin, "Invisible owned or free stone"
    return frames


def check_case(name, frames):
    last = next(reversed(frames.values()))
    h = last["hunter"]
    if name in ("take-cairn", "return-nearby", "carry-away", "reset-away"):
        assert any(int(f["hunter"]["holds"]) >= 3 for f in frames.values()), "Hold never took a real cairn stone"
    if name == "take-cairn":
        assert int(h["holds"]) >= 3 and h["count"] == "3" and h["taken"] == "1"
    elif name == "return-nearby":
        assert h["holds"] == "-1" and h["count"] == "4" and h["placed"] == "1"
        assert any(int(f["hunter"]["carried"]) >= 3 for f in frames.values()), "Returned stone was not visibly carried"
    elif name == "carry-away":
        assert h["holds"] == "-1" and h["count"] == "3" and h["placed"] == "0"
        assert any(i >= 3 and it["pin"] == "0" and float(it["x"]) < 90 for i, it in last["items"].items())
    elif name == "reset-away":
        assert h["holds"] == "-1" and h["count"] == "4" and h["taken"] == "0"
        for i, it in last["items"].items():
            if i >= 3:
                room, x, y = re.split(r"[/,]", it["home"])
                assert it["room"] == room and abs(float(it["x"])-float(x)) < .001 and abs(float(it["y"])-float(y)) < .001
    elif name == "idle-tending":
        assert int(h["tended"]) >= 1 and h["count"] == "4"
        assert any(f["hunter"]["state"] == "5" and int(f["hunter"]["carried"]) >= 3 for f in frames.values())
    elif name == "fire-conversation":
        assert h["fire"] == "1" and h["state"] == "7" and h["count"] == "4"
        assert sum(f["hunter"]["voice"] == "4" for f in frames.values()) == 1


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--candidate", type=Path, default=ROOT / "build/game-probe.exe")
    ap.add_argument("--baseline", type=Path, default=ROOT / "build/game-baseline-headless.exe")
    ap.add_argument("--render", action="store_true")
    ap.add_argument("--report", type=Path, default=ROOT / "docs/evidence/hunter/full-game-integration.json")
    args = ap.parse_args()
    records = []
    for name, options in CASES.items():
        outputs = [execute(args.candidate, options, mode, args.render) for mode in ("--flat", "--depth")]
        relevant = lambda s: [x for x in s.splitlines() if x.startswith(("f=", "HUNTER ", "ITEM ", "LIFE ", "SFX ", "CITY "))]
        assert relevant(outputs[0]) == relevant(outputs[1]), f"{name}: behavior depends on presentation"
        frames = inspect(outputs[0])
        check_case(name, frames)
        transitions = []
        previous = None
        for number, frame in frames.items():
            h = frame["hunter"]
            state = tuple(h[key] for key in ("state", "carried", "count", "holds", "placed", "tended", "taken", "fire"))
            if state != previous or h["voice"] != "0":
                transitions.append({"frame":number, **h})
                previous = state
        records.append({"case": name, "frames_per_mode": len(frames), "complete_trace_sha256": hashlib.sha256("\n".join(relevant(outputs[0])).encode()).hexdigest(), "transitions":transitions, "last_hunter": next(reversed(frames.values()))["hunter"]})
        print(f"PASS {name}: {len(frames)} frames per mode, every item and owner conserved")
    baseline = execute(args.baseline, CASES["take-cairn"], None, args.render, False)
    current = execute(args.candidate, CASES["take-cairn"], "--flat", args.render)
    old = [x for x in baseline.splitlines() if x.startswith("f=")]
    new = [x for x in current.splitlines() if x.startswith("f=")]
    assert len(old) == len(new)
    first = next(((a,b) for a,b in zip(old,new) if a != b), None)
    assert first and "hold=0" in first[0] and "hold=2" in first[1], "Authored change was not the expected stone pickup"
    sources = sorted((ROOT / "src").glob("*.c")) + sorted((ROOT / "src").glob("*.h")) + [Path(__file__).resolve()]
    report = {"status":"passed", "rendered":args.render, "cases":records, "compared_frames":2*sum(r["frames_per_mode"] for r in records),
              "source_sha256":{str(p.relative_to(ROOT)).replace("\\", "/"):hashlib.sha256(p.read_bytes()).hexdigest() for p in sources},
              "candidate_sha256":hashlib.sha256(args.candidate.read_bytes()).hexdigest(),
              "intentional_baseline_difference":{"case":"take-cairn", "baseline_first_different_frame":first[0], "current_first_different_frame":first[1], "meaning":"Hold picks a real cairn stone where the inherited game had decorative stones. Heavy physics then applies normally; the difference is recorded, not filtered."},
              "scope":"Actual CLI inputs through main/Frame/Sim; conservation, reset, nearby return, distant removal, idle tending and fire. No spontaneous human discovery or audio/visual craft acceptance."}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2)+"\n", encoding="utf-8")
    print(f"PASS authored baseline difference retained; {args.report}")


if __name__ == "__main__":
    main()
