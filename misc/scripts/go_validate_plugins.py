#!/usr/bin/env python3
"""Offline validation of the GO. bundled plugin manifest and vendored files.

GO.-owned test code is MIT licensed. Standard library only.
"""

# Copyright (c) 2026 SampleCraft / GO. Game Engine contributors
# SPDX-License-Identifier: MIT

from __future__ import annotations

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / "go_bundled_plugins" / "plugins.json"
BUNDLE = ROOT / "go_bundled_plugins"
REQUIRED = {
    "name", "id", "upstream_url", "license", "license_url", "tag", "commit",
    "godot_min", "addon_path", "license_file", "type", "default_enabled",
    "bundled", "category", "notes", "source_subdir", "license_source",
    "tarball_sha256",
}


def main() -> int:
    errors: list[str] = []
    if not MANIFEST.is_file():
        print(f"missing manifest: {MANIFEST}", file=sys.stderr)
        return 1
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    if manifest.get("schema_version") != 1:
        errors.append("schema_version must be 1")
    plugins = manifest.get("plugins")
    if not isinstance(plugins, list) or not plugins:
        errors.append("plugins must be a non-empty array")
        plugins = []

    ids: set[str] = set()
    for index, plugin in enumerate(plugins):
        label = plugin.get("id", f"plugin[{index}]")
        missing = REQUIRED - set(plugin)
        if missing:
            errors.append(f"{label}: missing fields {sorted(missing)}")
            continue
        if label in ids:
            errors.append(f"duplicate plugin id: {label}")
        ids.add(label)
        if plugin["type"] not in ("gdscript", "gdextension"):
            errors.append(f"{label}: unknown type {plugin['type']!r}")
        if plugin["license"] != "MIT":
            errors.append(f"{label}: license {plugin['license']!r} needs manual compatibility review")
        if not str(plugin["godot_min"]).startswith("4."):
            errors.append(f"{label}: godot_min should be a Godot 4.x version")
        if len(plugin["commit"]) != 40 or any(c not in "0123456789abcdef" for c in plugin["commit"]):
            errors.append(f"{label}: commit is not a pinned 40-character SHA")
        if "latest" in plugin["tag"].lower() or "latest" in plugin["upstream_url"].lower():
            errors.append(f"{label}: 'latest' is not an allowed runtime pin")
        if not plugin["addon_path"].startswith("addons/"):
            errors.append(f"{label}: addon_path must stay under addons/")
        if plugin["bundled"] and plugin["default_enabled"]:
            # Until a compiled 4.7.2 editor smoke test exists, all vendored plugins stay opt-in.
            errors.append(f"{label}: default_enabled=true is not yet proven in a compiled editor")

        for other_id in plugin.get("mutually_exclusive_with", []):
            other = next((p for p in plugins if p.get("id") == other_id), None)
            if other is None:
                errors.append(f"{label}: unknown mutually-exclusive id {other_id!r}")
            elif label not in other.get("mutually_exclusive_with", []):
                errors.append(f"{label}: mutually_exclusive_with is not symmetric with {other_id}")

        if not plugin["bundled"]:
            # GDExtension/optional entries are release-bundle stubs, not source vendoring.
            if not plugin["notes"]:
                errors.append(f"{label}: unbundled plugin needs a reason in notes")
            continue

        if plugin["type"] != "gdscript":
            errors.append(f"{label}: only GDScript plugins may be source-vendored")
        checksum = plugin["tarball_sha256"]
        if len(checksum) != 64 or any(c not in "0123456789abcdef" for c in checksum):
            errors.append(f"{label}: missing/invalid tarball_sha256")
        if not plugin["source_subdir"] or not plugin["license_source"]:
            errors.append(f"{label}: bundled plugin needs source_subdir and license_source")

        destination = BUNDLE / label
        addon = destination / plugin["addon_path"]
        license_file = destination / plugin["license_file"]
        origin = destination / ".origin.json"
        if not (addon / "plugin.cfg").is_file():
            errors.append(f"{label}: missing {plugin['addon_path']}/plugin.cfg")
        if not license_file.is_file():
            errors.append(f"{label}: missing license {plugin['license_file']}")
        if not origin.is_file():
            errors.append(f"{label}: missing .origin.json")
        else:
            data = json.loads(origin.read_text(encoding="utf-8"))
            if data.get("commit") != plugin["commit"]:
                errors.append(f"{label}: vendored origin commit differs from manifest")
            if data.get("tarball_sha256") != checksum:
                errors.append(f"{label}: vendored origin checksum differs from manifest")
        for path in destination.rglob("*"):
            if path.is_symlink():
                errors.append(f"{label}: symlink in bundle is not allowed: {path.relative_to(ROOT)}")

    if errors:
        print("GO. plugin manifest validation FAILED:", file=sys.stderr)
        for error in errors:
            print(f"  - {error}", file=sys.stderr)
        return 1
    bundled = sum(1 for plugin in plugins if plugin.get("bundled"))
    print(f"OK: {len(plugins)} plugin entries, {bundled} source-vendored, all pinned and license-checked")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
