#!/usr/bin/env python3
"""Print the important lines of a failed SCons log as GitHub annotations.

GitHub's job-log CDN is not reachable from every developer sandbox. One compact
error annotation is retrievable through api.github.com, so full-build failures
remain diagnosable without a second artifact download.

GO.-owned code is MIT licensed.
"""

# Copyright (c) 2026 SampleCraft / GO. Game Engine contributors
# SPDX-License-Identifier: MIT

from __future__ import annotations

import argparse
import re
from pathlib import Path

ANSI = re.compile(r"\x1b\[[0-9;?]*[ -/]*[@-~]")
IMPORTANT = re.compile(
    r"(error:|fatal error:|undefined reference|unresolved external|scons: \*\*\*|failed building|fatal error |ninja: build stopped|LNK\d+)",
    re.IGNORECASE,
)
MAX_LINES = 120
MAX_CHARS = 60000


def escape_command(value: str) -> str:
    return value.replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")


def summarize(text: str, max_lines: int = MAX_LINES) -> tuple[str, int]:
    lines = text.replace("\r\n", "\n").replace("\r", "\n").splitlines()
    hits = [line for line in lines if IMPORTANT.search(line)]
    tail = lines[-35:]
    selected: list[str] = []
    for line in hits + tail:
        if line not in selected:
            selected.append(line)
        if len(selected) >= max_lines:
            break
    return "\n".join(selected[-max_lines:]), len(hits)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path)
    parser.add_argument("--max-lines", type=int, default=MAX_LINES)
    args = parser.parse_args()

    text = ANSI.sub("", args.log.read_text(encoding="utf-8", errors="replace"))
    summary, hit_count = summarize(text, args.max_lines)
    message = f"GO. build log {args.log.name} ({hit_count} error-like lines):\n{summary}"[:MAX_CHARS]
    print(f"::error title=GO. full build failure::{escape_command(message)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
