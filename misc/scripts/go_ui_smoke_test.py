#!/usr/bin/env python3
"""Mocked UI capture sequencing tests; not visual editor acceptance."""

# Copyright (c) 2026 SampleCraft / GO. Game Engine contributors
# SPDX-License-Identifier: MIT

import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest.mock import Mock, patch

import go_ui_smoke as smoke


class CaptureTests(unittest.TestCase):
    def exercise(self, ready):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / ".go_agent_acceptance").write_text("fixture", encoding="utf-8")
            output = root / "capture.png"
            process = Mock(pid=123, poll=Mock(return_value=None if ready else 1))
            launches = []
            commands = []

            def launch(command, **kwargs):
                launches.append(command)
                if ready:
                    kwargs["stdout"].write("GO_EDITOR_UI_READY: fixture\n")
                    kwargs["stdout"].flush()
                return process

            def run(command, **kwargs):
                commands.append(command)
                if command[0] == "scrot":
                    Path(command[1]).write_bytes(b"mock capture" * 200)
                return subprocess.CompletedProcess(command, 0, "99\n", "")

            with (
                patch(
                    "sys.argv",
                    ["capture", "--editor", str(root / "editor"), "--project", str(root), "--output", str(output)],
                ),
                patch.object(smoke.subprocess, "Popen", side_effect=launch),
                patch.object(smoke.subprocess, "run", side_effect=run),
                patch.object(smoke.time, "sleep"),
            ):
                if ready:
                    self.assertEqual(smoke.main(), 0)
                else:
                    with self.assertRaises(RuntimeError):
                        smoke.main()
            self.assertIn("--go-ui-acceptance", launches[0])
            self.assertIn("1560x960", launches[0])
            self.assertEqual(any(c[0] == "scrot" for c in commands), ready)
            self.assertEqual(output.exists(), ready)

    def test_ready_scene_is_resized_before_capture(self):
        self.exercise(True)

    def test_missing_scene_readiness_refuses_capture(self):
        self.exercise(False)


if __name__ == "__main__":
    unittest.main()
