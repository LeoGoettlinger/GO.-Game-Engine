#!/usr/bin/env python3
"""Verify the complete external Kilo checkout against GO.'s audited source pin."""

# Copyright (c) 2026 SampleCraft / GO. Game Engine contributors
# SPDX-License-Identifier: MIT

import argparse
import hashlib
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("checkout", type=Path)
    args = parser.parse_args()
    manifest = json.loads((ROOT / "docs/go/kilo-reference.json").read_text(encoding="utf-8"))

    def git(*arguments: str) -> str:
        return subprocess.check_output(["git", "-C", str(args.checkout), *arguments], text=True).strip()

    errors = []
    if git("rev-parse", "HEAD") != manifest["commit"]:
        errors.append("checkout does not match the audited commit")
    if git("rev-parse", "--is-shallow-repository") != "false":
        errors.append("shallow checkout: full history required")
    config = git("config", "--list").lower().splitlines()
    if "core.sparsecheckout=true" in config or any(".partialclonefilter=" in line for line in config):
        errors.append("sparse/filtered clone: full checkout required")
    for name, expected in manifest["files"].items():
        path = args.checkout / name
        if not path.is_file() or hashlib.sha256(path.read_bytes()).hexdigest() != expected:
            errors.append(f"source mismatch: {name}")
    if errors:
        for error in errors:
            print(f"FAIL: {error}")
        return 1
    print(f"OK: full Kilo checkout at {manifest['commit']}; {len(manifest['files'])} audited source hashes match")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
