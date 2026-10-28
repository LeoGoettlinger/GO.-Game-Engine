#!/usr/bin/env python3
"""Compile and run GO.'s Godot-independent C++ guardrail tests.

The tested `.cpp` files are the same ones compiled into `modules/go_suite`, but
they have no Godot includes. This gives PR CI a real C++ compile/run check
without a multi-hour engine build.

GO.-owned test code is MIT licensed.
"""

# Copyright (c) 2026 SampleCraft / GO. Game Engine contributors
# SPDX-License-Identifier: MIT

from __future__ import annotations

import argparse
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MODULE = ROOT / "modules/go_suite"
SOURCES = [
    "go_suite_core.cpp",
    "go_git_remote.cpp",
    "go_agentics_path.cpp",
    "go_git_lfs.cpp",
    "go_system_git.cpp",
    "go_kilo_models.cpp",
    "go_agentics_tools.cpp",
    "go_commit_message.cpp",
    "go_auto_commit.cpp",
]
TEST = MODULE / "tests/standalone_guardrails.cpp"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", default="g++")
    args = parser.parse_args()

    missing = [name for name in SOURCES if not (MODULE / name).is_file()]
    if missing or not TEST.is_file():
        print(f"missing GO. guardrail sources: {missing}", file=sys.stderr)
        return 1

    with tempfile.TemporaryDirectory(prefix="go-guardrails-") as tmp:
        executable = Path(tmp) / "go-guardrails"
        command = [
            args.compiler,
            "-std=c++17",
            "-fno-exceptions",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-I",
            str(MODULE),
            str(TEST),
            *[str(MODULE / name) for name in SOURCES],
            "-o",
            str(executable),
        ]
        compile_result = subprocess.run(command, cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if compile_result.returncode != 0:
            print("GO. guardrails compilation FAILED:", file=sys.stderr)
            print(compile_result.stdout, file=sys.stderr)
            return 1
        if compile_result.stdout.strip():
            print(compile_result.stdout.strip())
        run_result = subprocess.run(
            [str(executable)], cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=120
        )
        print(run_result.stdout.strip())
        if run_result.stderr:
            print(run_result.stderr.strip(), file=sys.stderr)
        if run_result.returncode != 0:
            return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
