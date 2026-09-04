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


class ToolTests(unittest.TestCase):
    def route_status(self, answer):
        with patch.object(probe, "reach", return_value=answer):
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
