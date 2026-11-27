#!/usr/bin/env python3
"""Offline archive-validation regressions; not native extension startup tests."""

# Copyright (c) 2026 SampleCraft / GO. Game Engine contributors
# SPDX-License-Identifier: MIT

import hashlib
import io
import tempfile
import unittest
import zipfile
from pathlib import Path

from go_bundle_extensions import unpack


class ArchiveTests(unittest.TestCase):
    def archive(self, extra=None):
        stream = io.BytesIO()
        with zipfile.ZipFile(stream, "w") as archive:
            for name, value in {
                "package/addons/example/example.gdextension": "[configuration]",
                "package/addons/example/bin/example.so": "fixture",
                "package/addons/example/LICENSE": "fixture license",
                **(extra or {}),
            }.items():
                archive.writestr(name, value)
        return stream.getvalue()

    def plugin(self, data):
        return {
            "id": "example",
            "addon_path": "addons/example",
            "license_file": "addons/example/LICENSE",
            "release_asset": {"sha256": hashlib.sha256(data).hexdigest()},
        }

    def test_valid_bundle_and_no_overwrite(self):
        data = self.archive()
        with tempfile.TemporaryDirectory() as tmp:
            output = Path(tmp)
            unpack(data, self.plugin(data), output, "linux")
            self.assertTrue((output / "example/.release-origin.json").is_file())
            with self.assertRaises(ValueError):
                unpack(data, self.plugin(data), output, "linux")

    def test_hash_mismatch(self):
        data = self.archive()
        with tempfile.TemporaryDirectory() as tmp, self.assertRaises(ValueError):
            unpack(data + b"changed", self.plugin(data), Path(tmp), "linux")

    def test_traversal(self):
        data = self.archive({"../outside": "bad"})
        with tempfile.TemporaryDirectory() as tmp, self.assertRaises(ValueError):
            unpack(data, self.plugin(data), Path(tmp), "linux")

    def test_platform_binary_required(self):
        data = self.archive()
        with tempfile.TemporaryDirectory() as tmp, self.assertRaises(ValueError):
            unpack(data, self.plugin(data), Path(tmp), "windows")

    def test_ambiguous_addon(self):
        data = self.archive({"other/example/example.gdextension": "ambiguous"})
        with tempfile.TemporaryDirectory() as tmp, self.assertRaises(ValueError):
            unpack(data, self.plugin(data), Path(tmp), "linux")


if __name__ == "__main__":
    unittest.main()
