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
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.unlink(missing_ok=True)
    log_path = args.output.with_suffix(".log")
    with log_path.open("w", encoding="utf-8") as log:
        process = subprocess.Popen(
            [
                str(args.editor.resolve()),
                "--editor",
                "--path",
                str(args.project.resolve()),
                "--display-driver",
                "x11",
                "--rendering-method",
                "gl_compatibility",
                "res://player.tscn",
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
            # Allow the first imported scene and a few rendered frames to settle.
            time.sleep(5)
            if process.poll() is not None:
                raise RuntimeError("Editor exited before screenshot capture")
            subprocess.run(["scrot", str(args.output.resolve())], timeout=30, check=True)
            if not args.output.is_file() or args.output.stat().st_size < 1024:
                raise RuntimeError("Missing or empty editor screenshot")
            print("GO_EDITOR_UI_CAPTURED: actual X11 editor window; manual visual review remains required")
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
