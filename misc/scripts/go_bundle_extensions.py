#!/usr/bin/env python3
"""Maintainer-only, checksum-pinned release addon bundling; never called by the editor."""

# Copyright (c) 2026 SampleCraft / GO. Game Engine contributors
# SPDX-License-Identifier: MIT

from __future__ import annotations

import argparse
import hashlib
import io
import json
import stat
import urllib.request
import zipfile
from pathlib import Path, PurePosixPath
from typing import Any

ROOT = Path(__file__).resolve().parents[2]


def unpack(data: bytes, plugin: dict[str, Any], output: Path, platform: str) -> None:
    if hashlib.sha256(data).hexdigest() != plugin["release_asset"]["sha256"]:
        raise ValueError(f"{plugin['id']}: release SHA-256 mismatch")
    addon = PurePosixPath(plugin["addon_path"])
    with zipfile.ZipFile(io.BytesIO(data)) as archive:
        entries = archive.infolist()
        if sum(entry.file_size for entry in entries) > 1024 * 1024 * 1024 or len(entries) > 20000:
            raise ValueError("Oversized release archive")
        for entry in entries:
            path = PurePosixPath(entry.filename)
            if path.is_absolute() or ".." in path.parts or "\\" in entry.filename or ":" in entry.filename:
                raise ValueError("Unsafe release archive path")
            if stat.S_ISLNK(entry.external_attr >> 16):
                raise ValueError("Linked archive entry")
        # Locate the actual addon using its pinned directory name, not a demo project.
        roots = set()
        for entry in entries:
            parts = PurePosixPath(entry.filename).parts
            for i in range(len(parts) - 1):
                if parts[i] == addon.name and entry.filename.endswith(".gdextension"):
                    roots.add(PurePosixPath(*parts[: i + 1]))
        if len(roots) != 1:
            raise ValueError(f"{plugin['id']}: ambiguous/missing addon root: {roots}")
        source = roots.pop()
        destination = output / plugin["id"] / plugin["addon_path"]
        if destination.exists():
            raise ValueError(f"Refusing existing destination: {destination}")
        selected = []
        for entry in entries:
            path = PurePosixPath(entry.filename)
            if path.is_relative_to(source) and not entry.is_dir():
                selected.append((entry, path.relative_to(source)))
        suffix = ".dll" if platform == "windows" else ".so"
        if not any(str(path).endswith(suffix) for _, path in selected):
            raise ValueError(f"{plugin['id']}: no {platform} binary in addon")
        # Complete all validation before writing; no external extraction utility.
        for entry, relative in selected:
            target = destination / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(archive.read(entry))
        license_path = output / plugin["id"] / plugin["license_file"]
        if not license_path.is_file():
            pinned_license = ROOT / "go_bundled_plugins" / plugin["id"] / plugin["license_file"]
            if not pinned_license.is_file():
                raise ValueError(f"{plugin['id']}: missing pinned upstream license")
            license_path.parent.mkdir(parents=True, exist_ok=True)
            license_path.write_bytes(pinned_license.read_bytes())
        (output / plugin["id"] / ".release-origin.json").write_text(
            json.dumps(plugin["release_asset"], indent=2) + "\n", encoding="utf-8"
        )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--platform", choices=("linux", "windows"), required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    manifest = json.loads((ROOT / "go_bundled_plugins/plugins.json").read_text(encoding="utf-8"))
    for plugin in manifest["plugins"]:
        if plugin["type"] != "gdextension":
            continue
        url = plugin["release_asset"]["url"]
        if not url.startswith(plugin["upstream_url"] + "/releases/download/" + plugin["tag"] + "/"):
            raise ValueError("Release URL does not match pinned upstream/tag")
        print(f"Downloading pinned {plugin['name']}", flush=True)
        with urllib.request.urlopen(url, timeout=180) as response:
            data = response.read(512 * 1024 * 1024 + 1)
        if len(data) > 512 * 1024 * 1024:
            raise ValueError("Release download too large")
        unpack(data, plugin, args.output, args.platform)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        message = str(error).replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")
        print(f"::error title=Extension bundle verification::{message}", flush=True)
        raise
