#!/usr/bin/env python3
"""Syntax-check the Godot-facing `modules/go_suite` C++ sources.

This is not a full engine build. It generates the two small headers Godot's
SCons build normally creates (`gdextension_interface.gen.h` and
`gdvirtual.gen.h`) into a temporary directory, then asks the system C++ compiler
to parse every module `.cpp` against the real Godot headers. It catches missing
includes, wrong method signatures and type errors in minutes instead of hours.

GO.-owned test code is MIT licensed.
"""

# Copyright (c) 2026 SampleCraft / GO. Game Engine contributors
# SPDX-License-Identifier: MIT

from __future__ import annotations

import argparse
import importlib.util
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MODULE = ROOT / "modules/go_suite"


class Target:
    def __init__(self, path: Path) -> None:
        self.path = path

    def __str__(self) -> str:
        return str(self.path)


def load_builder(path: Path, name: str):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load builder: {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def generate_headers(stub_root: Path) -> None:
    core_dir = stub_root / "core"
    extension_dir = core_dir / "extension"
    object_dir = core_dir / "object"
    extension_dir.mkdir(parents=True, exist_ok=True)
    object_dir.mkdir(parents=True, exist_ok=True)

    # No classes are disabled by this syntax-only check; SCons normally generates this header.
    (core_dir / "disabled_classes.gen.h").write_text(
        "#pragma once\n// Generated empty stub for GO. header syntax checks.\n",
        encoding="utf-8",
    )

    interface = load_builder(ROOT / "core/extension/make_interface_header.py", "go_make_interface_header")
    interface.run(
        [Target(extension_dir / "gdextension_interface.gen.h")],
        [Target(ROOT / "core/extension/gdextension_interface.json")],
        None,
    )

    virtuals = load_builder(ROOT / "core/object/make_virtuals.py", "go_make_virtuals")
    virtuals.run([Target(object_dir / "gdvirtual.gen.h")], [], None)

    wrappers = load_builder(ROOT / "core/extension/make_wrappers.py", "go_make_wrappers")
    wrappers.run([Target(extension_dir / "ext_wrappers.gen.h")], [], None)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", default="g++")
    args = parser.parse_args()

    sys.path.insert(0, str(ROOT))
    sources = sorted(MODULE.glob("*.cpp"))
    if not sources:
        print("no GO. suite C++ sources found", file=sys.stderr)
        return 1

    with tempfile.TemporaryDirectory(prefix="go-syntax-") as tmp:
        stub_root = Path(tmp)
        try:
            generate_headers(stub_root)
        except Exception as exc:  # pragma: no cover - build-environment failure
            print(f"could not generate Godot stub headers: {exc}", file=sys.stderr)
            return 1

        includes = [
            stub_root,
            ROOT,
            ROOT / "platform/linuxbsd",
            ROOT / "thirdparty/zlib",
            MODULE,
        ]
        includes.append(ROOT / "thirdparty/doctest")
        test_headers = sorted((MODULE / "tests").glob("test_*.h"))
        failures: list[tuple[Path, str]] = []
        for source in sources + test_headers:
            command = [args.compiler, "-std=c++17", "-fsyntax-only", "-fno-exceptions", "-DTOOLS_ENABLED"]
            if source.suffix == ".h":
                command += ["-DTESTS_ENABLED", "-x", "c++"]
            for include in includes:
                command += ["-I", str(include)]
            command.append(str(source))
            result = subprocess.run(
                command,
                cwd=ROOT,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
            )
            if result.returncode != 0:
                failures.append((source, result.stdout))

        if failures:
            print("GO. Godot-facing syntax check FAILED:", file=sys.stderr)
            for source, output in failures:
                print(f"\n--- {source.relative_to(ROOT)} ---\n{output[-4000:]}", file=sys.stderr)
            return 1

    print(f"OK: {len(sources)} GO. suite C++ files and {len(test_headers)} native test headers pass syntax checks")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
