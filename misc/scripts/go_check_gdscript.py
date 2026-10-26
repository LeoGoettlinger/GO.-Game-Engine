#!/usr/bin/env python3
"""Parse-check vendored GDScript plugins with a pinned official Godot binary.

This is a CI/tooling script. It never enables plugins or executes project code;
Godot's `--check-only --script` only parses each script. A fake executable that
returns 0 can be used to test the orchestration offline.

Usage:
    python3 misc/scripts/go_check_gdscript.py --godot /path/to/Godot_v4.7.2-stable_linux.x86_64
"""

# Copyright (c) 2026 SampleCraft / GO. Game Engine contributors
# SPDX-License-Identifier: MIT

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / "go_bundled_plugins" / "plugins.json"
BUNDLE = ROOT / "go_bundled_plugins"


def prepare_project(destination: Path) -> list[str]:
    """Copy bundled addons into a clean temporary project; return res:// script paths."""
    project = destination / "project"
    (project / "addons").mkdir(parents=True)
    (project / "project.godot").write_text(
        'config_version=5\n\n[application]\n\nconfig/name="GO. Plugin Parse Check"\n'
        'config/features=PackedStringArray("4.7", "GL Compatibility")\n\n'
        '[editor_plugins]\n\nenabled=PackedStringArray()\n',
        encoding="utf-8",
    )
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    copied = 0
    for plugin in manifest["plugins"]:
        if not plugin.get("bundled") or plugin.get("type") != "gdscript":
            continue
        source = BUNDLE / plugin["id"] / plugin["addon_path"]
        target = project / plugin["addon_path"]
        if not source.is_dir():
            raise RuntimeError(f"missing vendored addon: {source}")
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copytree(source, target, dirs_exist_ok=True)
        copied += 1
    if copied == 0:
        raise RuntimeError("no bundled GDScript plugins found")
    scripts = sorted(
        path.relative_to(project).as_posix()
        for path in project.rglob("*.gd")
        if path.is_file()
    )
    return scripts


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--godot", required=True, help="Path to an official Godot 4.7.2-stable executable")
    parser.add_argument("--max-scripts", type=int, default=0, help="Optional limit for faster local runs")
    args = parser.parse_args()
    godot = Path(args.godot).resolve()
    if not godot.is_file():
        print(f"Godot binary not found: {godot}", file=sys.stderr)
        return 1
    if not os.access(godot, os.X_OK):
        print(f"Godot binary is not executable: {godot}", file=sys.stderr)
        return 1

    with tempfile.TemporaryDirectory(prefix="go-gdscript-check-") as tmp:
        temp_root = Path(tmp)
        project = temp_root / "project"
        scripts = prepare_project(temp_root)
        if args.max_scripts > 0:
            scripts = scripts[: args.max_scripts]

        env = os.environ.copy()
        for key in ("XDG_DATA_HOME", "XDG_CONFIG_HOME", "XDG_CACHE_HOME"):
            (temp_root / key.lower()).mkdir(exist_ok=True)
            env[key] = str(temp_root / key.lower())

        failures: list[tuple[str, str]] = []
        for index, script in enumerate(scripts, 1):
            result = subprocess.run(
                [
                    str(godot), "--headless", "--path", str(project),
                    "--check-only", "--script", "res://" + script,
                ],
                cwd=project,
                env=env,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                timeout=90,
            )
            if result.returncode != 0:
                failures.append((script, result.stdout.strip()[-2000:]))
            if index % 100 == 0:
                print(f"  checked {index}/{len(scripts)} ...")

        if failures:
            print(f"GDScript parse check FAILED: {len(failures)}/{len(scripts)} scripts", file=sys.stderr)
            for script, output in failures[:20]:
                print(f"\n--- res://{script} ---\n{output}", file=sys.stderr)
            return 1
        print(f"OK: {len(scripts)} vendored GDScript files parsed with {godot.name}")
        return 0


if __name__ == "__main__":
    raise SystemExit(main())
