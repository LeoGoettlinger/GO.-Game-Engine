#!/usr/bin/env python3
"""Enforce Code Agentics' forbidden-UI and path-guardrail invariants.

This scanner is intentionally source-based: it runs on every PR without a
compiled editor. Comments are removed, but string literals are kept because
forbidden UI labels would appear there.

GO.-owned test code is MIT licensed.
"""

# Copyright (c) 2026 SampleCraft / GO. Game Engine contributors
# SPDX-License-Identifier: MIT

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MODULE = ROOT / "modules/go_suite"

REQUIRED_FILES = [
    "go_git_dock.cpp",
    "go_git_dock.h",
    "go_agentics_dock.cpp",
    "go_agentics_dock.h",
    "go_kilo_client.cpp",
    "go_kilo_client.h",
    "go_agentics_tools.cpp",
    "go_agentics_tools.h",
    "go_suite_icons.gen.h",
    "go_system_git.h",
    "go_system_git.cpp",
    "go_git_controller.h",
    "go_git_controller.cpp",
    "go_remote_assistant.h",
    "go_remote_assistant.cpp",
    "go_lfs_dialog.h",
    "go_lfs_dialog.cpp",
]

SCANNED_FILES = [
    "go_git_dock.cpp",
    "go_git_dock.h",
    "go_agentics_dock.cpp",
    "go_agentics_dock.h",
    "go_kilo_client.cpp",
    "go_kilo_client.h",
    "go_agentics_tools.cpp",
    "go_agentics_tools.h",
    "go_suite_launcher.cpp",
    "go_suite_launcher.h",
    "go_git_controller.cpp",
    "go_git_controller.h",
    "go_remote_assistant.cpp",
    "go_remote_assistant.h",
    "go_lfs_dialog.cpp",
    "go_lfs_dialog.h",
]

FORBIDDEN_LABELS = [
    "api key",
    "api-key",
    "provider",
    "base url",
    "login",
    "sign in",
    "mcp",
    "workspace picker",
    "select folder",
    "folder picker",
    "terminal",
    "shell command",
    "custom provider",
    "byok",
    "billing",
]

FORBIDDEN_LINEEDIT_NAMES = [
    "api_key",
    "provider",
    "login",
    "mcp",
    "workspace",
    "folder",
    "base_url",
    "token",
    "password",
    "secret",
]

FORBIDDEN_CODE_PATTERNS = [
    r"OS::execute\s*\(",
    r"set_base_url\s*\(",
    r"ACCESS_FILESYSTEM",
]


def strip_comments(text: str) -> str:
    """Remove // and /* */ comments while preserving string literals."""
    result: list[str] = []
    i = 0
    in_string = False
    in_char = False
    in_line_comment = False
    in_block_comment = False
    while i < len(text):
        c = text[i]
        n = text[i + 1] if i + 1 < len(text) else ""
        if in_line_comment:
            if c == "\n":
                in_line_comment = False
                result.append(c)
            i += 1
            continue
        if in_block_comment:
            if c == "*" and n == "/":
                in_block_comment = False
                i += 2
            else:
                if c == "\n":
                    result.append(c)
                i += 1
            continue
        if in_string:
            result.append(c)
            if c == "\\" and n:
                result.append(n)
                i += 2
                continue
            if c == '"':
                in_string = False
            i += 1
            continue
        if in_char:
            result.append(c)
            if c == "\\" and n:
                result.append(n)
                i += 2
                continue
            if c == "'":
                in_char = False
            i += 1
            continue
        if c == '"':
            in_string = True
            result.append(c)
            i += 1
            continue
        if c == "'":
            in_char = True
            result.append(c)
            i += 1
            continue
        if c == "/" and n == "/":
            in_line_comment = True
            i += 2
            continue
        if c == "/" and n == "*":
            in_block_comment = True
            i += 2
            continue
        result.append(c)
        i += 1
    return "".join(result)


def main() -> int:
    errors: list[str] = []
    for relative in REQUIRED_FILES:
        if not (MODULE / relative).is_file():
            errors.append(f"missing GO. suite source: modules/go_suite/{relative}")

    contents: dict[str, str] = {}
    for relative in SCANNED_FILES:
        path = MODULE / relative
        if not path.is_file():
            errors.append(f"missing scanned source: modules/go_suite/{relative}")
            continue
        cleaned = strip_comments(path.read_text(encoding="utf-8"))
        contents[relative] = cleaned
        lowered = cleaned.lower()
        for label in FORBIDDEN_LABELS:
            if label in lowered:
                errors.append(f"{relative}: forbidden UI term {label!r}")
        for match in re.finditer(r"LineEdit\s*\*\s*(\w+)", cleaned):
            name = match.group(1).lower()
            for forbidden in FORBIDDEN_LINEEDIT_NAMES:
                if forbidden in name:
                    errors.append(f"{relative}: forbidden LineEdit name {match.group(1)!r}")
        for pattern in FORBIDDEN_CODE_PATTERNS:
            if re.search(pattern, cleaned):
                errors.append(f"{relative}: forbidden code pattern {pattern!r}")

    agentics = contents.get("go_agentics_dock.cpp", "")
    for required in [
        'TTR("Auto-Approve")',
        'TTR("Stop")',
        "Workspace: res:// (locked)",
        "Free models only",
    ]:
        if required not in agentics:
            errors.append(f"go_agentics_dock.cpp: missing required UI invariant {required!r}")

    client = contents.get("go_kilo_client.cpp", "")
    for required in ["DEFAULT_KILO_GATEWAY_URL", "ANONYMOUS_API_KEY", "models_endpoint"]:
        if required not in client:
            errors.append(f"go_kilo_client.cpp: missing Kilo invariant {required!r}")

    tools = contents.get("go_agentics_tools.cpp", "")
    for required in ["validate_res_path", "PathOperation::WRITE"]:
        if required not in tools:
            errors.append(f"go_agentics_tools.cpp: missing path-guard invariant {required!r}")

    icon_check = subprocess.run(
        [sys.executable, str(ROOT / "misc/scripts/go_update_dock_icons.py"), "--check"],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if icon_check.returncode != 0:
        errors.append("generated dock icon header is out of date")
    else:
        print(icon_check.stdout.strip())

    if errors:
        print("GO. UI guardrail check FAILED:", file=sys.stderr)
        for error in errors:
            print(f"  - {error}", file=sys.stderr)
        return 1
    print(f"OK: {len(SCANNED_FILES)} GO. suite UI/tool sources contain no forbidden settings or shell UI")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
