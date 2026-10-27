#!/usr/bin/env python3
"""Verify GO. branding invariants without compiling the engine.

GO.-owned test code is MIT licensed. Uses only the Python standard library so it
can run on every CI runner.
"""

# Copyright (c) 2026 SampleCraft / GO. Game Engine contributors
# SPDX-License-Identifier: MIT

from __future__ import annotations

import ast
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
EXACT_SPLASH_TEXT = "A Godot Game Engine Fork by SampleCraft"
FORK_NOTICE = "GO. Game Engine is an unofficial fork of the Godot Engine"


def version_values() -> dict[str, object]:
    tree = ast.parse((ROOT / "version.py").read_text(encoding="utf-8"))
    values: dict[str, object] = {}
    for node in tree.body:
        if isinstance(node, ast.Assign) and len(node.targets) == 1 and isinstance(node.targets[0], ast.Name):
            try:
                values[node.targets[0].id] = ast.literal_eval(node.value)
            except (ValueError, TypeError):
                pass
    return values


def png_size(path: Path) -> tuple[int, int]:
    data = path.read_bytes()
    if len(data) < 24 or data[:8] != b"\x89PNG\r\n\x1a\n" or data[12:16] != b"IHDR":
        raise ValueError(f"not a PNG with IHDR: {path}")
    return struct.unpack(">II", data[16:24])


def require(condition: bool, message: str, errors: list[str]) -> None:
    if not condition:
        errors.append(message)


def main() -> int:
    errors: list[str] = []
    values = version_values()
    require(values.get("name") == "GO. Game Engine", "version.py name is not 'GO. Game Engine'", errors)
    require(values.get("short_name") == "go", "version.py short_name is not 'go'", errors)
    require(
        (values.get("major"), values.get("minor"), values.get("patch")) == (4, 7, 2),
        "version.py numerical version is not upstream-compatible 4.7.2",
        errors,
    )
    require(values.get("docs") == "4.7", "version.py docs branch must stay on upstream 4.7", errors)
    require(
        "github.com/LeoGoettlinger/GO.-Game-Engine" in str(values.get("website", "")),
        "version.py website must point to the GO. repository",
        errors,
    )

    main_cpp = (ROOT / "main" / "main.cpp").read_text(encoding="utf-8")
    require(
        "// GO. --version must include the product name" in main_cpp,
        "main/main.cpp --version GO. patch is missing",
        errors,
    )
    require(
        'String(GODOT_VERSION_NAME) + " v" + get_full_version_string()' in main_cpp,
        "--version output does not include GODOT_VERSION_NAME",
        errors,
    )

    about = (ROOT / "editor" / "gui" / "editor_about.cpp").read_text(encoding="utf-8")
    require(FORK_NOTICE in about, "About dialog is missing the exact GO. fork notice", errors)
    require("GO. 0.1.0" in about, "About dialog is missing the GO. release label", errors)

    readme = (ROOT / "README.md").read_text(encoding="utf-8")
    require(FORK_NOTICE in readme, "README is missing the exact GO. fork notice", errors)
    require((ROOT / "README.godot-upstream.md").is_file(), "upstream README was not preserved", errors)

    generator = (ROOT / "go_assets" / "make_splash.py").read_text(encoding="utf-8")
    require(
        f'SPLASH_TEXT = "{EXACT_SPLASH_TEXT}"' in generator,
        "splash generator does not contain the exact SampleCraft text",
        errors,
    )
    require((ROOT / "go_assets" / "fonts" / "Monocraft.ttf").is_file(), "Monocraft TTF is missing", errors)
    require(
        (ROOT / "go_assets" / "fonts" / "LICENSE-Monocraft.txt").is_file(), "Monocraft OFL license is missing", errors
    )
    require(
        "SIL OPEN FONT LICENSE Version 1.1"
        in (ROOT / "go_assets" / "fonts" / "LICENSE-Monocraft.txt").read_text(encoding="utf-8"),
        "Monocraft license is not OFL 1.1",
        errors,
    )

    expected_sizes = {
        "main/splash.png": (800, 600),
        "main/splash_editor.png": (1152, 648),
        "main/app_icon.png": (256, 256),
        "go_project_template/splash.png": (800, 600),
    }
    for relative, expected in expected_sizes.items():
        path = ROOT / relative
        if not path.is_file():
            errors.append(f"generated image missing: {relative}")
            continue
        try:
            actual = png_size(path)
        except ValueError as exc:
            errors.append(str(exc))
            continue
        require(actual == expected, f"{relative} has size {actual}, expected {expected}", errors)

    require((ROOT / "editor" / "icons" / "GoLogo.svg").is_file(), "GoLogo.svg is missing", errors)
    require(
        "GoLogo" in (ROOT / "editor" / "gui" / "editor_about.cpp").read_text(encoding="utf-8"),
        "About dialog does not use GoLogo",
        errors,
    )

    if errors:
        print("GO. branding check FAILED:", file=sys.stderr)
        for error in errors:
            print(f"  - {error}", file=sys.stderr)
        return 1

    expected_name = values["name"]
    expected_version = f"{values['major']}.{values['minor']}.{values['patch']}.{values['status']}"
    print(f"OK: GO. branding; --version will print '{expected_name} v{expected_version}.<build>'")
    print(f"OK: exact splash text: {EXACT_SPLASH_TEXT!r}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
