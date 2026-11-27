#!/usr/bin/env python3
"""Start each bundled addon independently in a disposable compiled GO. editor project."""

# Copyright (c) 2026 SampleCraft / GO. Game Engine contributors
# SPDX-License-Identifier: MIT

from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import subprocess
import tempfile
from pathlib import Path

from go_annotate_build_log import ANSI, escape_command, summarize

ROOT = Path(__file__).resolve().parents[2]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--editor", type=Path, required=True)
    parser.add_argument("--extensions-dir", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    editor = args.editor.resolve()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    manifest = json.loads((ROOT / "go_bundled_plugins/plugins.json").read_text(encoding="utf-8"))
    failures = []
    # Keep fixtures outside the engine checkout. Otherwise Git walks to the
    # parent repository and GO. reads its status as though it were this project.
    with tempfile.TemporaryDirectory(prefix="go-plugin-startup-") as temporary:
        for plugin in manifest["plugins"]:
            project = Path(temporary) / plugin["id"]
            if project.exists():
                raise ValueError(f"Refusing existing smoke project: {project}")
            project.mkdir()
            base = args.extensions_dir.resolve() if plugin["type"] == "gdextension" else ROOT / "go_bundled_plugins"
            source = base / plugin["id"] / plugin["addon_path"]
            shutil.copytree(source, project / plugin["addon_path"])
            config = (
                'config_version=5\n[application]\nconfig/name="GO. Plugin Startup Smoke"\n'
                '[rendering]\nrenderer/rendering_method="gl_compatibility"\n'
            )
            if plugin["id"] == "dialogic":
                config += '[autoload]\nDialogic="*res://addons/dialogic/Core/DialogicGameHandler.gd"\n'
            (project / "project.godot").write_text(config, encoding="utf-8")
            env = os.environ.copy()
            for key in ("HOME", "LOCALAPPDATA", "XDG_CONFIG_HOME", "XDG_DATA_HOME", "XDG_CACHE_HOME", "APPDATA"):
                home = project / key.lower()
                home.mkdir()
                env[key] = str(home)
            for phase in ("import", "startup"):
                if phase == "startup" and (project / plugin["addon_path"] / "plugin.cfg").is_file():
                    enabled = plugin["addon_path"] + "/plugin.cfg"
                    (project / "project.godot").write_text(
                        config + f'[editor_plugins]\nenabled=PackedStringArray("res://{enabled}")\n', encoding="utf-8"
                    )
                command = [str(editor), "--headless", "--path", str(project)]
                command += ["--import"] if phase == "import" else ["--editor", "--quit-after", "120"]
                try:
                    result = subprocess.run(
                        command, env=env, capture_output=True, text=True, errors="replace", timeout=180
                    )
                    text = ANSI.sub("", result.stdout + result.stderr)
                    failed = result.returncode != 0 or bool(re.search(r"(?m)^(?:SCRIPT ERROR|ERROR):", text))
                except subprocess.TimeoutExpired:
                    text = "Editor startup timed out after 180 seconds"
                    failed = True
                (output / f"{plugin['id']}-{phase}.log").write_text(text, encoding="utf-8")
                if failed:
                    failures.append(plugin["id"] + ":" + phase)
                    summary, _ = summarize(text)
                    message = escape_command(failures[-1] + "\n" + summary[:12000])
                    print(f"::error title=Plugin startup smoke::{message}", flush=True)
                    break
            else:
                print(f"GO_PLUGIN_STARTUP_OK: {plugin['id']}", flush=True)
    (output / "summary.json").write_text(json.dumps({"failures": failures}, indent=2) + "\n", encoding="utf-8")
    return bool(failures)


if __name__ == "__main__":
    raise SystemExit(main())
