#!/usr/bin/env python3
"""Offline tests of the live probe; all HTTP is mocked, real C++ parser is compiled."""

# Copyright (c) 2026 SampleCraft / GO. Game Engine contributors
# SPDX-License-Identifier: MIT

import json
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

import go_live_kilo_smoke as smoke


class LiveProbeTests(unittest.TestCase):
    def exercise(self, free, tool, expected, requests):
        catalog = {"data": [{"id": "kilo-auto/free" if free else "paid/model", "isFree": free}]}
        response = {"choices": [{"message": {"content": json.dumps({"tool": tool, "arguments": {"path": "res://"}})}}]}
        with tempfile.TemporaryDirectory() as tmp:
            output = Path(tmp) / "report.json"
            args = ["probe", "--allow-live-synthetic-request", "--output", str(output)]
            with (
                patch("sys.argv", args),
                patch.object(
                    smoke, "request", side_effect=[json.dumps(catalog).encode(), json.dumps(response).encode()]
                ) as request,
            ):
                self.assertEqual(smoke.main(), expected)
                self.assertEqual(request.call_count, requests)
            self.assertEqual(json.loads(output.read_text(encoding="utf-8"))["success"], expected == 0)

    def test_free_synthetic_plan(self):
        self.exercise(True, "list_files", 0, 2)

    def test_paid_catalog_never_sends_chat(self):
        self.exercise(False, "list_files", 1, 1)

    def test_unexpected_tool_refused(self):
        self.exercise(True, "delete_file", 1, 2)


if __name__ == "__main__":
    unittest.main()
