#!/usr/bin/env python3
"""Capture a real Linux editor window under Xvfb; not a manual visual acceptance."""

# Copyright (c) 2026 SampleCraft / GO. Game Engine contributors
# SPDX-License-Identifier: MIT

from __future__ import annotations

import argparse
import subprocess
import time
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--editor", type=Path, required=True)
    parser.add_argument("--project", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--player", action="store_true", help="Capture the running project, not the editor")
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.unlink(missing_ok=True)
    log_path = args.output.with_suffix(".log")
    wait_for_scene = not args.player and (args.project / ".go_agent_acceptance").is_file()
    with log_path.open("w", encoding="utf-8") as log:
        process = subprocess.Popen(
            [
                str(args.editor.resolve()),
                *([] if args.player else ["--editor"]),
                "--path",
                str(args.project.resolve()),
                "--display-driver",
                "x11",
                "--rendering-method",
                "gl_compatibility",
                *([] if args.player else ["--resolution", "1560x960", "--position", "20,20", "res://player.tscn"]),
                *(["--", "--go-ui-acceptance"] if wait_for_scene else []),
            ],
            stdout=log,
            stderr=subprocess.STDOUT,
        )
        try:
            window = subprocess.run(
                ["xdotool", "search", "--sync", "--onlyvisible", "--pid", str(process.pid)],
                capture_output=True,
                text=True,
                timeout=60,
                check=True,
            )
            if not window.stdout.strip():
                raise RuntimeError("No visible editor window")
            if wait_for_scene:
                deadline = time.monotonic() + 60
                while "GO_EDITOR_UI_READY:" not in log_path.read_text(encoding="utf-8", errors="replace"):
                    if process.poll() is not None or time.monotonic() >= deadline:
                        raise RuntimeError("Editor did not confirm the loaded 2D player scene")
                    time.sleep(0.1)
            if not args.player:
                window_id = window.stdout.splitlines()[0]
                subprocess.run(["xdotool", "windowsize", "--sync", window_id, "1560", "960"], timeout=15, check=True)
                subprocess.run(["xdotool", "windowmove", "--sync", window_id, "20", "20"], timeout=15, check=True)
            # Render the final geometry, not the initial small splash window.
            time.sleep(3)
            if process.poll() is not None:
                raise RuntimeError("Editor exited before screenshot capture")
            subprocess.run(["scrot", str(args.output.resolve())], timeout=30, check=True)
            if not args.output.is_file() or args.output.stat().st_size < 1024:
                raise RuntimeError("Missing or empty editor screenshot")
            print(
                ("GO_PLAYER_UI_CAPTURED" if args.player else "GO_EDITOR_UI_CAPTURED")
                + ": actual X11 window; manual visual review remains required"
            )
        except (OSError, RuntimeError, subprocess.SubprocessError):
            log.flush()
            diagnostic = log_path.read_text(encoding="utf-8", errors="replace")[-12000:]
            print(diagnostic.encode("ascii", errors="backslashreplace").decode("ascii"))
            raise
        finally:
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=15)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait(timeout=10)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
