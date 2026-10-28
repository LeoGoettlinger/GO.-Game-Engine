#!/usr/bin/env python3
"""Launch a compiled GO. editor headlessly against an isolated empty project.

This checks native module startup and resource import, not visual UI behavior.
It never opens the developer's project or enables third-party plugins.
"""

# Copyright (c) 2026 SampleCraft / GO. Game Engine contributors
# SPDX-License-Identifier: MIT

from __future__ import annotations

import argparse
import os
import subprocess
import tempfile
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--editor", required=True, type=Path)
    args = parser.parse_args()
    editor = args.editor.resolve()
    with tempfile.TemporaryDirectory(prefix="go-editor-smoke-") as temporary:
        root = Path(temporary)
        project = root / "project"
        project.mkdir()
        (project / "project.godot").write_text(
            'config_version=5\n\n[application]\nconfig/name="GO. Native Smoke"\n'
            '\n[rendering]\nrenderer/rendering_method="gl_compatibility"\n',
            encoding="utf-8",
        )
        env = os.environ.copy()
        for key in ("HOME", "APPDATA", "LOCALAPPDATA", "XDG_DATA_HOME", "XDG_CONFIG_HOME", "XDG_CACHE_HOME"):
            directory = root / key.lower()
            directory.mkdir()
            env[key] = str(directory)
        try:
            result = subprocess.run(
                [str(editor), "--headless", "--path", str(project), "--import"],
                env=env,
                capture_output=True,
                text=True,
                errors="replace",
                timeout=180,
                check=False,
            )
        except subprocess.TimeoutExpired:
            print("::error::GO. editor startup/import exceeded 180 seconds")
            return 1
        output = result.stdout + result.stderr
        print(output)
        if result.returncode or "ERROR:" in output:
            message = output[-10000:].replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")
            print(f"::error title=GO. editor startup failed::{message}")
            return 1
        if not (project / ".godot").is_dir():
            print("::error::GO. editor did not create an import directory")
            return 1
    print("OK: compiled GO. editor started and imported an isolated empty project")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
