#!/usr/bin/env python3
"""Verify that GO.'s minimal upstream patches are present and documented.

A full SCons build compiles the patched translation units. This lightweight
check catches accidental loss of a patch during an upstream merge before the
expensive full build starts.

GO.-owned code is MIT licensed.
"""

# Copyright (c) 2026 SampleCraft / GO. Game Engine contributors
# SPDX-License-Identifier: MIT

from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def main() -> int:
    errors: list[str] = []

    editor_node = (ROOT / "editor/editor_node.cpp").read_text(encoding="utf-8")
    required_editor_snippets = [
        "#ifdef MODULE_GO_SUITE_ENABLED",
        '#include "modules/go_suite/go_suite_registry.h"',
        "GoSuiteRegistry::is_editor_input_locked()",
        "NOTIFICATION_WM_CLOSE_REQUEST",
        "PROJECT_QUIT_TO_PROJECT_MANAGER",
    ]
    for snippet in required_editor_snippets:
        if snippet not in editor_node:
            errors.append(f"editor/editor_node.cpp is missing GO. patch snippet {snippet!r}")
    if editor_node.count("GoSuiteRegistry::is_editor_input_locked()") < 2:
        errors.append("editor/editor_node.cpp must guard both close and quit paths")

    registry = (ROOT / "modules/go_suite/go_suite_registry.h").read_text(encoding="utf-8")
    for snippet in ["set_editor_input_locked", "is_editor_input_locked", "request_auto_commit"]:
        if snippet not in registry:
            errors.append(f"go_suite_registry.h is missing {snippet!r}")

    register = (ROOT / "docs/go/UPSTREAM_PATCHES.md").read_text(encoding="utf-8")
    if "`editor/editor_node.cpp`" not in register or "Automatic Commit" not in register:
        errors.append("UPSTREAM_PATCHES.md must document the editor_node.cpp lock patch")

    if errors:
        print("GO. upstream patch check FAILED:", file=sys.stderr)
        for error in errors:
            print(f"  - {error}", file=sys.stderr)
        return 1
    print("OK: GO. upstream patches are present and documented")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
