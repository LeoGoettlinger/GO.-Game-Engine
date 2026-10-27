#!/usr/bin/env python3
"""Exercise the GO. Git Suite command builders/parsers against a local bare repo.

This test uses the real system `git` but never contacts a network service. The
`git-lfs` binary is not required: LFS pointer semantics are simulated with a
small synthetic pointer file, and the pure C++ probe validates the same parser
logic the editor uses. Real `git lfs install/pull/push` remains a compiled-editor
/CI smoke test, documented in docs/go/GIT_SUITE.md.

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
PROBE = ROOT / "misc/scripts/go_git_probe.cpp"
SOURCES = [
    MODULE / "go_suite_core.cpp",
    MODULE / "go_git_remote.cpp",
    MODULE / "go_git_lfs.cpp",
    MODULE / "go_system_git.cpp",
    PROBE,
]

POINTER = (
    "version https://git-lfs.github.com/spec/v1\n"
    "oid sha256:0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\n"
    "size 6291456\n"
)


def git(*args: str, cwd: Path | None = None, check: bool = True) -> str:
    result = subprocess.run(
        ["git", *args],
        cwd=cwd,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if check and result.returncode != 0:
        print(f"git {' '.join(args)} failed:\n{result.stdout}", file=sys.stderr)
        raise SystemExit(1)
    return result.stdout.strip()


def compile_probe(compiler: str, executable: Path) -> None:
    command = [
        compiler,
        "-std=c++17",
        "-Wall",
        "-Wextra",
        "-Wpedantic",
        "-Werror",
        "-I",
        str(MODULE),
        *[str(path) for path in SOURCES],
        "-o",
        str(executable),
    ]
    result = subprocess.run(command, cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if result.returncode != 0:
        print("GO. Git probe compilation failed:", file=sys.stderr)
        print(result.stdout, file=sys.stderr)
        raise SystemExit(1)
    if result.stdout.strip():
        print(result.stdout.strip())


def probe(executable: Path, *args: str, stdin: str | None = None, check: bool = True) -> str:
    result = subprocess.run(
        [str(executable), *args],
        cwd=ROOT,
        text=True,
        input=stdin,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if check and result.returncode != 0:
        print(f"probe {' '.join(args)} failed:\n{result.stdout}", file=sys.stderr)
        raise SystemExit(1)
    return result.stdout


def assert_contains(text: str, needle: str, label: str) -> None:
    if needle not in text:
        print(f"missing {needle!r} in {label}:\n{text}", file=sys.stderr)
        raise SystemExit(1)


def parse_key_values(text: str) -> dict[str, str]:
    result: dict[str, str] = {}
    for line in text.splitlines():
        if not line or " " not in line:
            continue
        key, value = line.split(" ", 1)
        result[key] = value
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", default="g++")
    args = parser.parse_args()

    with tempfile.TemporaryDirectory(prefix="go-git-flow-") as tmp:
        root = Path(tmp)
        executable = root / "go_git_probe"
        compile_probe(args.compiler, executable)

        # 1. Pure command builders and remote safety.
        status_cmd = probe(executable, "cmd", str(root), "status")
        assert_contains(status_cmd, "ARG 0 -C", "status command")
        assert_contains(status_cmd, f"ARG 1 {root}", "status command")
        assert_contains(status_cmd, "ARG 2 status", "status command")

        codeberg_cmd = probe(executable, "cmd", str(root), "remote-add", "https://codeberg.org/owner/repo.git")
        assert_contains(codeberg_cmd, "EXEC git", "Codeberg remote command")
        assert_contains(codeberg_cmd, "https://codeberg.org/owner/repo.git", "Codeberg remote command")

        bad_remote = probe(
            executable,
            "cmd",
            str(root),
            "remote-add",
            "https://user:secret@example.com/repo.git",
            check=False,
        )
        assert_contains(bad_remote, "INVALID", "credential remote rejection")

        install_linux = probe(executable, "cmd", str(root), "install-linux")
        assert_contains(install_linux, "EXEC pkexec", "Linux LFS install command")
        install_windows = probe(executable, "cmd", str(root), "install-windows")
        assert_contains(install_windows, "EXEC winget", "Windows LFS install command")

        # 2. Pure parsers.
        parsed_remote = parse_key_values(probe(executable, "parse-remote", "https://codeberg.org/owner/repo.git"))
        assert parsed_remote.get("VALID") == "1", parsed_remote
        assert parsed_remote.get("HOST") == "codeberg", parsed_remote
        assert parsed_remote.get("OWNER") == "owner", parsed_remote
        assert parsed_remote.get("REPO") == "repo", parsed_remote

        parsed_status = parse_key_values(
            probe(
                executable,
                "parse-status",
                stdin="## main...origin/main [ahead 1, behind 2]\nM  file.gd\nUU conflict.txt\n",
            )
        )
        assert parsed_status.get("BRANCH") == "main", parsed_status
        assert parsed_status.get("AHEAD") == "1", parsed_status
        assert parsed_status.get("BEHIND") == "2", parsed_status
        assert parsed_status.get("CONFLICT") == "1", parsed_status

        parsed_lfs = parse_key_values(
            probe(
                executable,
                "parse-lfs-ls",
                stdin="oid123 * hero.png (2.0 MB)\noid456 - music.ogg (512 KB)\n",
            )
        )
        assert parsed_lfs.get("FILES") == "2", parsed_lfs
        assert int(parsed_lfs.get("TOTAL", "0")) >= 2 * 1024 * 1024, parsed_lfs
        assert parsed_lfs.get("FIRST") == "hero.png", parsed_lfs

        aheadbehind = parse_key_values(probe(executable, "aheadbehind", stdin="0\t0"))
        assert aheadbehind.get("OK") == "1" and aheadbehind.get("AHEAD") == "0" and aheadbehind.get("BEHIND") == "0", aheadbehind

        pointer_result = probe(executable, "pointer", stdin=POINTER)
        assert_contains(pointer_result, "POINTER 1", "LFS pointer parser")
        assert_contains(probe(executable, "pointer", stdin="not a pointer"), "POINTER 0", "LFS pointer parser")

        # 3. Real local bare-repo flow.
        remote = root / "remote.git"
        work = root / "work"
        clone = root / "clone"
        git("init", "--bare", "--initial-branch=main", str(remote))
        git("init", "-b", "main", str(work))
        git("config", "user.name", "GO. Test", cwd=work)
        git("config", "user.email", "go-test@example.invalid", cwd=work)

        attributes = probe(executable, "attributes", stdin="")
        assert_contains(attributes, "# GO. Git LFS", "managed .gitattributes")
        assert_contains(attributes, "*.png filter=lfs diff=lfs merge=lfs -text", "managed .gitattributes")

        (work / "assets").mkdir(parents=True)
        big = work / "assets" / "big.unknown"
        big.write_bytes(b"G" * (6 * 1024 * 1024))
        large = parse_key_values(probe(executable, "large", "res://assets/big.unknown", str(big.stat().st_size), "5", "codeberg"))
        assert large.get("ACTION") == "track-lfs", large
        assert large.get("PATTERN") == "*.unknown", large

        # Simulate `git lfs track *.unknown`: verify the pure merge keeps a user rule.
        # Do not actually write that rule here: CI runners have a real git-lfs binary,
        # and enabling the filter would require uploading a real LFS object to the bare repo.
        tracked_attributes = probe(
            executable,
            "attributes",
            stdin="*.unknown filter=lfs diff=lfs merge=lfs -text\n",
        )
        assert_contains(tracked_attributes, "*.unknown filter=lfs diff=lfs merge=lfs -text", "tracked LFS pattern")
        assert_contains(tracked_attributes, "*.png filter=lfs diff=lfs merge=lfs -text", "managed LFS block")
        (work / ".gitattributes").write_text(attributes, encoding="utf-8")

        # Keep the classification evidence, but do not stage a 6 MiB binary through a filter.
        big.unlink()

        (work / "README.md").write_text("# GO. local bare-repo test\n", encoding="utf-8")
        (work / "project.godot").write_text('config_version=5\n\n[application]\nconfig/name="GO. Test"\n', encoding="utf-8")
        pointer_file = work / "assets" / "hero.pointer"
        pointer_file.write_text(POINTER, encoding="utf-8")

        staged_pointer = probe(executable, "pointer", stdin=pointer_file.read_text(encoding="utf-8"))
        assert_contains(staged_pointer, "POINTER 1", "simulated LFS pointer file")

        git("add", "-A", cwd=work)
        staged_blob = git("show", ":assets/hero.pointer", cwd=work)
        assert_contains(probe(executable, "pointer", stdin=staged_blob + "\n"), "POINTER 1", "staged pointer")

        git("commit", "-m", "chore: initial GO. Git Suite test", cwd=work)
        git("remote", "add", "origin", str(remote), cwd=work)
        git("push", "-u", "origin", "main", cwd=work)

        git("clone", str(remote), str(clone))
        assert (clone / "README.md").is_file(), "cloned README missing"
        assert (clone / "assets" / "hero.pointer").is_file(), "cloned pointer file missing"

        clean_status = git("status", "--porcelain=v1", "--branch", "--untracked-files=all", cwd=clone)
        parsed_clean = parse_key_values(probe(executable, "parse-status", stdin=clean_status + "\n"))
        assert parsed_clean.get("CHANGES") == "0", parsed_clean
        assert parsed_clean.get("BRANCH") == "main", parsed_clean

        ahead_behind_text = git("rev-list", "--left-right", "--count", "HEAD...origin/main", cwd=clone)
        parsed_ab = parse_key_values(probe(executable, "aheadbehind", stdin=ahead_behind_text))
        assert parsed_ab.get("AHEAD") == "0" and parsed_ab.get("BEHIND") == "0", parsed_ab

        remote_url = git("remote", "get-url", "origin", cwd=clone)
        parsed_local_remote = parse_key_values(probe(executable, "parse-remote", remote_url))
        assert parsed_local_remote.get("VALID") == "1", parsed_local_remote
        assert parsed_local_remote.get("HOST") == "local", parsed_local_remote

        log_subject = git("log", "-1", "--pretty=%s", cwd=clone)
        assert log_subject == "chore: initial GO. Git Suite test", log_subject

    print("OK: GO. Git Suite local bare-repo flow passed (init → commit → push → clone → status)")
    print("NOTE: real git-lfs install/pull/push remains a compiled-editor/CI smoke test; no live network used")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
