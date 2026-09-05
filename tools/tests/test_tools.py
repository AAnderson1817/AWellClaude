import contextlib
import io
from pathlib import Path
import runpy
import subprocess
import sys
import unittest
from unittest.mock import patch

TOOLS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS))
import probe


def fake_run(plan, at=None, frames=None, room=None):
    """A trace that satisfies every direct check in route.py, so the tool tests never
    spawn the game (they used to, and failed whenever the display was busy)."""
    def row(**k):
        base = dict(f=1, x=170.0, y=-3.0, vx=0, vy=0, ground=1, air=0, coy=0, buf=0,
                    room=1, wet=0, sfx="-", lamp=0, lampRoom=0, lampX=0, lampY=0)
        base.update(k); return base
    if plan == "-:20,D:300":                 return [row(room=1, wet=1)]              # shaft down
    if plan == "-:10,J:30,-:60":             return [row(room=0, y=149.0, x=176.0)]  # shaft up: row 20
    if plan.startswith("-:110,L"):           return [row(y=37.0, x=30.0)]            # water -> left island: row 6, col 3
    if plan.startswith("-:110,R"):           return [row(y=37.0, x=250.0)]           # water -> right island: row 6, col 31
    if plan == "-:6,J:6,-:6,D:120":          return [row(wet=1)]                     # through A1 with Down
    return [row()]                                                                    # on A1: row 1, col 21

class ToolTests(unittest.TestCase):
    def route_status(self, answer):
        with patch.object(probe, "reach", return_value=answer), patch.object(probe, "run", fake_run):
            with contextlib.redirect_stdout(io.StringIO()):
                with self.assertRaises(SystemExit) as result:
                    runpy.run_path(str(TOOLS / "route.py"), run_name="__main__")
        return result.exception.code

    def test_failed_hops_fail_command(self):
        self.assertEqual(self.route_status(None), 1)

    def test_passing_hops_succeed(self):
        self.assertEqual(self.route_status("reachable"), 0)

    def test_crashed_probe_is_not_a_reachability_result(self):
        result = subprocess.CompletedProcess([], 2, stdout="", stderr="startup failed")
        with patch.object(probe.subprocess, "run", return_value=result):
            with self.assertRaisesRegex(RuntimeError, "startup failed"):
                probe.run("-:1")

    def test_empty_probe_is_not_a_reachability_result(self):
        result = subprocess.CompletedProcess([], 0, stdout="", stderr="")
        with patch.object(probe.subprocess, "run", return_value=result):
            with self.assertRaisesRegex(RuntimeError, "no trace"):
                probe.run("-:1")


if __name__ == "__main__":
    unittest.main()
