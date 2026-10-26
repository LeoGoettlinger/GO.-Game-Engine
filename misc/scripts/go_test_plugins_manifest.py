#!/usr/bin/env python3
"""Small standard-library unit tests for the plugin manifest."""

# Copyright (c) 2026 SampleCraft / GO. Game Engine contributors
# SPDX-License-Identifier: MIT

from __future__ import annotations

import json
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / "go_bundled_plugins/plugins.json"


class PluginManifestTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))

    def test_schema_and_nonempty(self) -> None:
        self.assertEqual(self.manifest["schema_version"], 1)
        self.assertGreater(len(self.manifest["plugins"]), 0)

    def test_all_entries_are_pinned_and_mit(self) -> None:
        for plugin in self.manifest["plugins"]:
            with self.subTest(plugin=plugin["id"]):
                self.assertEqual(len(plugin["commit"]), 40)
                self.assertNotIn("latest", plugin["tag"].lower())
                self.assertEqual(plugin["license"], "MIT")

    def test_no_unproven_default_activation(self) -> None:
        for plugin in self.manifest["plugins"]:
            with self.subTest(plugin=plugin["id"]):
                self.assertFalse(plugin["default_enabled"])

    def test_mutual_exclusion_is_symmetric(self) -> None:
        by_id = {plugin["id"]: plugin for plugin in self.manifest["plugins"]}
        for plugin in self.manifest["plugins"]:
            for other_id in plugin.get("mutually_exclusive_with", []):
                self.assertIn(other_id, by_id)
                self.assertIn(plugin["id"], by_id[other_id].get("mutually_exclusive_with", []))


if __name__ == "__main__":
    unittest.main()
