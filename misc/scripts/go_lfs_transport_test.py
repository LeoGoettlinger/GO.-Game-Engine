#!/usr/bin/env python3
"""Test real Git LFS clean/smudge and transport using only a local bare remote.

No project or global Git configuration is changed. All repositories and LFS
objects are temporary. CI requires git-lfs; developers may explicitly skip it.
"""

# Copyright (c) 2026 SampleCraft / GO. Game Engine contributors
# SPDX-License-Identifier: MIT

from __future__ import annotations

import argparse
import hashlib
import os
import shutil
import subprocess
import tempfile
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--allow-missing", action="store_true")
    args = parser.parse_args()
    if shutil.which("git-lfs") is None:
        print("SKIP: git-lfs is not installed" if args.allow_missing else "ERROR: git-lfs is required")
        return 0 if args.allow_missing else 1

    with tempfile.TemporaryDirectory(prefix="go-real-lfs-") as temporary:
        root = Path(temporary)
        env = {key: value for key, value in os.environ.items() if not key.startswith("GIT_")}
        env.update({
            "GIT_CONFIG_NOSYSTEM": "1",
            "GIT_CONFIG_GLOBAL": str(root / "empty-config"),
            "GIT_TERMINAL_PROMPT": "0",
            "HOME": str(root),
            "XDG_CONFIG_HOME": str(root / "config"),
        })

        def git(cwd: Path, *arguments: str) -> bytes:
            result = subprocess.run(
                ["git", "-C", str(cwd), *arguments],
                env=env,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                timeout=90,
                check=False,
            )
            if result.returncode:
                raise RuntimeError(f"git {arguments!r}: {result.stderr.decode(errors='replace')}")
            return result.stdout

        remote, work, clone = root / "remote.git", root / "work", root / "clone"
        git(root, "init", "--bare", "--initial-branch=main", str(remote))
        git(root, "init", "-b", "main", str(work))
        git(work, "config", "user.name", "GO. LFS Test")
        git(work, "config", "user.email", "go-lfs@example.invalid")
        git(work, "lfs", "install", "--local")
        git(work, "lfs", "track", "*.bin")
        payload = bytes(range(256)) * 24577
        digest = hashlib.sha256(payload).hexdigest()
        (work / "asset.bin").write_bytes(payload)
        git(work, "add", "--", ".gitattributes", "asset.bin")
        pointer = git(work, "show", ":asset.bin").decode()
        assert pointer.startswith("version https://git-lfs.github.com/spec/v1\n")
        assert f"oid sha256:{digest}\n" in pointer
        assert f"size {len(payload)}\n" in pointer
        git(work, "commit", "-m", "test: store real LFS asset")
        git(work, "remote", "add", "origin", str(remote))
        git(work, "push", "-u", "origin", "main")
        git(root, "clone", str(remote), str(clone))
        git(clone, "lfs", "install", "--local")
        git(clone, "lfs", "pull", "origin")
        assert (clone / "asset.bin").read_bytes() == payload
        assert git(clone, "status", "--porcelain") == b""
        # The remote stores a pointer, never the original binary in its Git tree.
        assert git(remote, "show", "main:asset.bin").decode() == pointer
        # The original worktree remains clean after transport.
        assert git(work, "status", "--porcelain") == b""
    print("OK: real Git LFS pointer, local push, clone/pull, binary round-trip and clean status")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
