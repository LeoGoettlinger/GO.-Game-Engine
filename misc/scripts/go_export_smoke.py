#!/usr/bin/env python3
"""Exercise the compiled editor and both custom templates using an offline player fixture.

This proves import/export/player behavior, not AI scene creation or visual acceptance.
Logs and a machine-readable result are retained even when a step fails.
"""

# Copyright (c) 2026 SampleCraft / GO. Game Engine contributors
# SPDX-License-Identifier: MIT

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import tempfile
from pathlib import Path

from go_annotate_build_log import escape_command, summarize

MARKER = "GO_PLAYER_SMOKE_OK"
PLAYER = """extends CharacterBody2D

var smoke_frames: int = 0

func _physics_process(_delta: float) -> void:
    var smoke: bool = "--go-smoke-test" in OS.get_cmdline_user_args()
    velocity.x = 120.0 if smoke else Input.get_axis("ui_left", "ui_right") * 120.0
    move_and_slide()
    if smoke:
        smoke_frames += 1
        if smoke_frames == 3:
            if not ($Sprite2D is Sprite2D) or not ($CollisionShape2D is CollisionShape2D):
                push_error("Player fixture lost its child types")
                get_tree().quit(2)
                return
            if "--go-export-test" in OS.get_cmdline_user_args() and ClassDB.class_exists("EditorPlugin"):
                push_error("Editor classes leaked into the export template")
                get_tree().quit(3)
                return
            print("GO_PLAYER_SMOKE_OK")
            get_tree().quit()
"""
SCENE = """[gd_scene load_steps=5 format=3]

[ext_resource type="Script" path="res://player.gd" id="1"]

[sub_resource type="Gradient" id="Gradient"]
colors = PackedColorArray(0.1, 0.8, 0.5, 1, 0.2, 0.4, 0.9, 1)

[sub_resource type="GradientTexture2D" id="Texture"]
gradient = SubResource("Gradient")
width = 32
height = 32

[sub_resource type="RectangleShape2D" id="Shape"]
size = Vector2(32, 32)

[node name="Player" type="CharacterBody2D"]
position = Vector2(50, 50)
script = ExtResource("1")

[node name="Sprite2D" type="Sprite2D" parent="."]
texture = SubResource("Texture")

[node name="CollisionShape2D" type="CollisionShape2D" parent="."]
shape = SubResource("Shape")
"""


def prepare_project(project: Path, platform: str, debug: Path, release: Path) -> None:
    project.mkdir()
    (project / "project.godot").write_text(
        'config_version=5\n\n[application]\nconfig/name="GO. Export Smoke"\n'
        'run/main_scene="res://player.tscn"\n\n[rendering]\n'
        'renderer/rendering_method="gl_compatibility"\n',
        encoding="utf-8",
    )
    (project / "player.gd").write_text(PLAYER, encoding="utf-8")
    (project / "player.tscn").write_text(SCENE, encoding="utf-8")
    target = "Windows Desktop" if platform == "windows" else "Linux"
    (project / "export_presets.cfg").write_text(
        '[preset.0]\nname="GO Smoke"\nplatform='
        + json.dumps(target)
        + '\nrunnable=true\nexport_filter="all_resources"\ninclude_filter=""\nexclude_filter=""\n'
        "\n[preset.0.options]\ncustom_template/debug="
        + json.dumps(debug.as_posix())
        + "\ncustom_template/release="
        + json.dumps(release.as_posix())
        + '\nbinary_format/architecture="x86_64"\nbinary_format/embed_pck=true\n'
        "application/modify_resources=false\ncodesign/enable=false\n",
        encoding="utf-8",
    )


def run_step(name: str, command: list[str], cwd: Path, env: dict[str, str], logs: Path, marker: str = "") -> None:
    try:
        result = subprocess.run(
            command, cwd=cwd, env=env, capture_output=True, text=True, errors="replace", timeout=180
        )
    except subprocess.TimeoutExpired as error:
        (logs / f"{name}.log").write_text(f"Timed out after 180 seconds: {error}\n", encoding="utf-8")
        raise RuntimeError(f"{name}: timeout") from error
    output = result.stdout + result.stderr
    (logs / f"{name}.log").write_text(output, encoding="utf-8")
    print(f"{name}: exit={result.returncode}\n{output[-6000:]}")
    if result.returncode != 0 or "ERROR:" in output or (marker and marker not in output):
        summary, _ = summarize(output)
        raise RuntimeError(f"{name}: exit={result.returncode}; failed or required success marker missing\n{summary}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--editor", type=Path, required=True)
    parser.add_argument("--debug-template", type=Path, required=True)
    parser.add_argument("--release-template", type=Path, required=True)
    parser.add_argument("--platform", choices=("linux", "windows"), required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()
    logs = args.output_dir.resolve()
    logs.mkdir(parents=True, exist_ok=True)
    report: dict[str, object] = {
        "platform": args.platform,
        "passed": False,
        "scope": "offline import/export/player only",
    }
    try:
        editor, debug, release = (path.resolve() for path in (args.editor, args.debug_template, args.release_template))
        for path in (editor, debug, release):
            if not path.is_file():
                raise RuntimeError(f"Missing compiled binary: {path}")
        with tempfile.TemporaryDirectory(prefix="go-export-smoke-") as temporary:
            root = Path(temporary)
            project = root / "project"
            prepare_project(project, args.platform, debug, release)
            env = os.environ.copy()
            for key in ("HOME", "APPDATA", "LOCALAPPDATA", "XDG_DATA_HOME", "XDG_CONFIG_HOME", "XDG_CACHE_HOME"):
                directory = root / key.lower()
                directory.mkdir()
                env[key] = str(directory)
            run_step("import", [str(editor), "--headless", "--path", str(project), "--import"], project, env, logs)
            run_step(
                "editor-player",
                [str(editor), "--headless", "--path", str(project), "--", "--go-smoke-test"],
                project,
                env,
                logs,
                MARKER,
            )
            for kind in ("debug", "release"):
                export_dir = root / f"export-{kind}"
                export_dir.mkdir()
                binary = export_dir / ("player.exe" if args.platform == "windows" else "player.x86_64")
                run_step(
                    f"export-{kind}",
                    [str(editor), "--headless", "--path", str(project), f"--export-{kind}", "GO Smoke", str(binary)],
                    project,
                    env,
                    logs,
                )
                if not binary.is_file() or binary.stat().st_size == 0:
                    raise RuntimeError(f"{kind}: exported executable is missing or empty")
                # Run only the exported executable outside the source project.
                # Embedded PCK must supply the scene/script; no source fallback is possible.
                isolated = root / f"isolated-{kind}"
                isolated.mkdir()
                player = isolated / binary.name
                shutil.copy2(binary, player)
                if args.platform == "linux":
                    player.chmod(0o755)
                run_step(
                    f"player-{kind}",
                    [str(player), "--headless", "--", "--go-smoke-test", "--go-export-test"],
                    isolated,
                    env,
                    logs,
                    MARKER,
                )
        report["passed"] = True
    except (OSError, RuntimeError) as error:
        report["error"] = str(error)
        print(f"GO. export smoke FAILED: {error}")
        print(f"::error title=GO. export/player failure::{escape_command(str(error)[:50000])}")
    finally:
        (logs / "result.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
