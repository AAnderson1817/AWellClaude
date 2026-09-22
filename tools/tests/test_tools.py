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
                    room=1, wet=0, sfx="-", hold=0, lampRoom=0, lampX=0, lampY=0,
                    stoneRoom=0, stoneX=0, stoneY=0, fade=0.0)
        base.update(k); return base
    if plan.startswith("-:90,R"):            return [row(y=261.0, x=780.0)]          # the basin -> the first tread: row 34, col 97
    if plan == "R:900":                      return [row(y=101.0, x=160.0 + 4 * i) for i in range(200)]   # the long walk: row 14, past col 108
    if plan == "R:30,-:200":                 return [row(y=269.0, x=136.0)]          # the chasm: the undercroft's floor, row 35
    if plan.endswith("H:90,-:60"):           return [row(wet=1, hold=2)] * 20 + [row(x=97.0, y=157.0)]   # reset from the deep: the start, at the door's foot
    if plan.endswith("H:90,-:40"):           return [row(hold=2, stoneX=290)] * 20 + [row(hold=0, stoneX=290)]   # the stone goes home
    if plan.endswith("H:40,-:40"):           return [row(hold=2, fade=0.44)] * 20 + [row(hold=2, fade=0.0)]      # let go early
    return [row()]

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
