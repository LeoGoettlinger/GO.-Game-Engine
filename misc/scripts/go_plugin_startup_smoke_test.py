#!/usr/bin/env python3
"""Mocked plugin-startup orchestration tests, not editor/runtime acceptance."""

# Copyright (c) 2026 SampleCraft / GO. Game Engine contributors
# SPDX-License-Identifier: MIT

import io
import json
import subprocess
import tempfile
import unittest
from contextlib import redirect_stdout
from pathlib import Path
from unittest.mock import patch

import go_plugin_startup_smoke as smoke


class StartupTests(unittest.TestCase):
    def check_result(self, output, expected, phases):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            bundle = root / "go_bundled_plugins"
            addon = bundle / "fixture/addons/fixture"
            addon.mkdir(parents=True)
            (addon / "plugin.cfg").write_text('[plugin]\nname="Fixture"\n', encoding="utf-8")
            (bundle / "plugins.json").write_text(
                json.dumps({"plugins": [{"id": "fixture", "type": "gdscript", "addon_path": "addons/fixture"}]}),
                encoding="utf-8",
            )
            args = [
                "smoke",
                "--editor",
                str(root / "editor"),
                "--extensions-dir",
                str(root),
                "--output",
                str(root / "results"),
            ]
            with (
                patch.object(smoke, "ROOT", root),
                patch("sys.argv", args),
                patch.object(
                    smoke.subprocess, "run", return_value=subprocess.CompletedProcess([], 0, output, "")
                ) as run,
            ):
                with redirect_stdout(io.StringIO()) as captured:
                    self.assertEqual(smoke.main(), expected)
                self.assertEqual(run.call_count, phases)
                for call in run.call_args_list:
                    command = call.args[0]
                    project = Path(command[command.index("--path") + 1])
                    self.assertFalse(project.is_relative_to(root))
                if expected:
                    self.assertIn("SCRIPT ERROR: broken addon", captured.getvalue())
            summary = json.loads((root / "results/summary.json").read_text(encoding="utf-8"))
            self.assertEqual(bool(summary["failures"]), bool(expected))

    def test_import_then_enabled_startup(self):
        self.check_result("Editor started", 0, 2)

    def test_script_error_fails_even_with_zero_exit(self):
        self.check_result("\x1b[31mSCRIPT ERROR: broken addon\x1b[0m\n" + "import progress\n" * 1000, 1, 1)


if __name__ == "__main__":
    unittest.main()
