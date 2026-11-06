#!/usr/bin/env python3
"""Offline orchestration tests, not evidence of a real engine export."""

# Copyright (c) 2026 SampleCraft / GO. Game Engine contributors
# SPDX-License-Identifier: MIT

from __future__ import annotations

import json
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

import go_export_smoke as smoke


class ExportSmokeTests(unittest.TestCase):
    def test_fixture_has_custom_templates_and_player_nodes(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            for platform, export_name in (("linux", "Linux"), ("windows", "Windows Desktop")):
                project = root / platform
                smoke.prepare_project(project, platform, root / "debug binary", root / "release binary")
                preset = (project / "export_presets.cfg").read_text(encoding="utf-8")
                self.assertIn(f'platform="{export_name}"', preset)
                self.assertIn("custom_template/debug=", preset)
                self.assertIn("custom_template/release=", preset)
                self.assertIn("binary_format/embed_pck=true", preset)
                self.assertIn('include_filter=""', preset)
                self.assertIn('exclude_filter=""', preset)
                scene = (project / "player.tscn").read_text(encoding="utf-8")
                for node_type in ("CharacterBody2D", "Sprite2D", "CollisionShape2D"):
                    self.assertIn(f'type="{node_type}"', scene)
                self.assertIn("ClassDB.class_exists", (project / "player.gd").read_text(encoding="utf-8"))

    def test_exit_zero_is_not_a_player_success(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            for code, output in ((0, ""), (1, smoke.MARKER), (0, "ERROR: bad scene\n" + smoke.MARKER)):
                with patch.object(
                    smoke.subprocess, "run", return_value=subprocess.CompletedProcess([], code, output, "")
                ):
                    with self.assertRaises(RuntimeError):
                        smoke.run_step("player", ["fake"], root, {}, root, smoke.MARKER)
            with patch.object(
                smoke.subprocess, "run", return_value=subprocess.CompletedProcess([], 0, smoke.MARKER, "")
            ):
                smoke.run_step("player", ["fake"], root, {}, root, smoke.MARKER)
            self.assertIn(smoke.MARKER, (root / "player.log").read_text(encoding="utf-8"))

    def test_timeout_and_missing_binary_keep_failure_evidence(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            with patch.object(smoke.subprocess, "run", side_effect=subprocess.TimeoutExpired("fake", 180)):
                with self.assertRaises(RuntimeError):
                    smoke.run_step("timeout", ["fake"], root, {}, root)
            self.assertTrue((root / "timeout.log").is_file())
            arguments = ["go_export_smoke.py", "--platform", "linux", "--output-dir", str(root)]
            for argument in ("--editor", "--debug-template", "--release-template"):
                arguments += [argument, str(root / "missing")]
            with patch("sys.argv", arguments):
                self.assertEqual(smoke.main(), 1)
            self.assertFalse(json.loads((root / "result.json").read_text(encoding="utf-8"))["passed"])


if __name__ == "__main__":
    unittest.main()
