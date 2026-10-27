#!/usr/bin/env python3
"""Vendor pinned GDScript plugins from upstream tarballs (no runtime download).

The script is for maintainers/CI, not for the end user's editor. It downloads a
pinned commit from codeload.github.com, verifies/record its SHA-256, extracts
only the declared addon directory and license, and writes an `.origin.json`
provenance file. GDExtension binaries are intentionally not vendored here.

Usage:
    python3 misc/scripts/go_vendor_plugins.py --update-checksums
    python3 misc/scripts/go_vendor_plugins.py --check
"""

# Copyright (c) 2026 SampleCraft / GO. Game Engine contributors
# SPDX-License-Identifier: MIT

from __future__ import annotations

import argparse
import hashlib
import io
import json
import shutil
import sys
import tarfile
import urllib.request
from pathlib import Path, PurePosixPath

ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / "go_bundled_plugins" / "plugins.json"
BUNDLE = ROOT / "go_bundled_plugins"


def tarball_url(plugin: dict) -> str:
    upstream = plugin["upstream_url"].rstrip("/")
    prefix = "https://github.com/"
    if not upstream.startswith(prefix):
        raise ValueError(f"Unsupported upstream URL: {upstream}")
    return f"https://codeload.github.com/{upstream[len(prefix) :]}/tar.gz/{plugin['commit']}"


def download(url: str) -> bytes:
    request = urllib.request.Request(url, headers={"User-Agent": "GO. Game Engine plugin vendor/1.0"})
    with urllib.request.urlopen(request, timeout=180) as response:
        if response.status != 200:
            raise RuntimeError(f"HTTP {response.status} for {url}")
        return response.read()


def safe_relative(path: str) -> PurePosixPath:
    candidate = PurePosixPath(path)
    if candidate.is_absolute() or ".." in candidate.parts:
        raise ValueError(f"Unsafe path in tarball: {path}")
    return candidate


def extract_plugin(data: bytes, plugin: dict, destination: Path) -> None:
    source_subdir = plugin.get("source_subdir", "")
    license_source = plugin.get("license_source", "")
    license_file = plugin.get("license_file", "")
    if not source_subdir or not license_source or not license_file:
        raise ValueError(f"{plugin['id']}: source_subdir/license_source/license_file are required")
    if destination.exists():
        shutil.rmtree(destination)
    destination.mkdir(parents=True)

    addon_destination = destination / plugin["addon_path"]
    addon_destination.mkdir(parents=True)
    license_destination = destination / license_file
    license_destination.parent.mkdir(parents=True, exist_ok=True)

    found_addon = False
    found_license = False
    exclude_prefixes = tuple(plugin.get("exclude_prefixes", []))
    with tarfile.open(fileobj=io.BytesIO(data), mode="r:gz") as archive:
        members = archive.getmembers()
        roots = {safe_relative(m.name).parts[0] for m in members if m.name}
        if len(roots) != 1:
            raise ValueError(f"Unexpected tarball roots: {sorted(roots)[:5]}")
        root = next(iter(roots))
        for member in members:
            rel = safe_relative(member.name)
            if rel.parts[0] != root:
                continue
            relative = PurePosixPath(*rel.parts[1:])
            if member.issym() or member.islnk():
                raise ValueError(f"Symlink in plugin tarball is not allowed: {member.name}")
            if member.isdir():
                continue
            if not member.isfile():
                continue
            relative_text = relative.as_posix()
            if relative_text == source_subdir or relative_text.startswith(source_subdir + "/"):
                inner = PurePosixPath(relative_text).relative_to(source_subdir).as_posix()
                if inner.startswith(exclude_prefixes):
                    continue
                target = addon_destination / inner
                target.parent.mkdir(parents=True, exist_ok=True)
                with archive.extractfile(member) as src, target.open("wb") as dst:
                    shutil.copyfileobj(src, dst)
                found_addon = True
            if relative_text == license_source:
                with archive.extractfile(member) as src, license_destination.open("wb") as dst:
                    shutil.copyfileobj(src, dst)
                found_license = True

    if not found_addon:
        raise ValueError(f"{plugin['id']}: source_subdir not found: {source_subdir}")
    if not found_license:
        raise ValueError(f"{plugin['id']}: license_source not found: {license_source}")
    if not (addon_destination / "plugin.cfg").is_file():
        raise ValueError(f"{plugin['id']}: plugin.cfg missing after extraction")

    origin = {
        "name": plugin["name"],
        "id": plugin["id"],
        "upstream_url": plugin["upstream_url"],
        "tag": plugin["tag"],
        "commit": plugin["commit"],
        "license": plugin["license"],
        "license_url": plugin["license_url"],
        "tarball_sha256": plugin.get("tarball_sha256", ""),
        "source_subdir": source_subdir,
        "addon_path": plugin["addon_path"],
    }
    (destination / ".origin.json").write_text(json.dumps(origin, indent=2) + "\n", encoding="utf-8")


def bundled_gdscript(manifest: dict) -> list[dict]:
    return [p for p in manifest["plugins"] if p.get("bundled") and p.get("type") == "gdscript"]


def check(manifest: dict) -> list[str]:
    errors: list[str] = []
    for plugin in bundled_gdscript(manifest):
        destination = BUNDLE / plugin["id"]
        addon = destination / plugin["addon_path"]
        license_file = destination / plugin["license_file"]
        origin = destination / ".origin.json"
        checksum = plugin.get("tarball_sha256", "")
        if not (addon / "plugin.cfg").is_file():
            errors.append(f"{plugin['id']}: missing {plugin['addon_path']}/plugin.cfg")
        if not license_file.is_file():
            errors.append(f"{plugin['id']}: missing license {plugin['license_file']}")
        if not origin.is_file():
            errors.append(f"{plugin['id']}: missing .origin.json")
        else:
            data = json.loads(origin.read_text(encoding="utf-8"))
            if data.get("commit") != plugin["commit"]:
                errors.append(f"{plugin['id']}: origin commit mismatch")
            if data.get("tarball_sha256") != checksum:
                errors.append(f"{plugin['id']}: origin SHA-256 mismatch")
        if isinstance(checksum, str) and len(checksum) != 64:
            errors.append(f"{plugin['id']}: manifest SHA-256 is missing or invalid")
    return errors


def vendor(manifest: dict, update_checksums: bool) -> int:
    changed = False
    for plugin in bundled_gdscript(manifest):
        url = tarball_url(plugin)
        print(f"Downloading {plugin['name']} ({plugin['tag']}) ...")
        data = download(url)
        digest = hashlib.sha256(data).hexdigest()
        recorded = plugin.get("tarball_sha256", "")
        if recorded and recorded != digest:
            print(f"SHA-256 mismatch for {plugin['id']}: manifest {recorded}, download {digest}", file=sys.stderr)
            return 1
        if not recorded:
            if not update_checksums:
                print(f"Missing SHA-256 for {plugin['id']}; pass --update-checksums", file=sys.stderr)
                return 1
            plugin["tarball_sha256"] = digest
            changed = True
        print(f"  sha256 {digest}, {len(data)} bytes")
        extract_plugin(data, plugin, BUNDLE / plugin["id"])
    if changed:
        MANIFEST.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
        print(f"Updated checksums in {MANIFEST.relative_to(ROOT)}")
    errors = check(manifest)
    if errors:
        for error in errors:
            print(f"  - {error}", file=sys.stderr)
        return 1
    print(f"OK: {len(bundled_gdscript(manifest))} GDScript plugins vendored and verified")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--update-checksums", action="store_true", help="record missing tarball SHA-256 values")
    parser.add_argument("--check", action="store_true", help="offline validation of vendored files")
    args = parser.parse_args()
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    if args.check:
        errors = check(manifest)
        if errors:
            for error in errors:
                print(f"  - {error}", file=sys.stderr)
            return 1
        print(f"OK: {len(bundled_gdscript(manifest))} vendored GDScript plugins match the manifest")
        return 0
    return vendor(manifest, args.update_checksums)


if __name__ == "__main__":
    raise SystemExit(main())
