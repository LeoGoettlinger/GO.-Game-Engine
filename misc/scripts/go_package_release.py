#!/usr/bin/env python3
"""Assemble a GO. Game Engine platform release ZIP and export-template TPZ.

The source tree is the working directory. Build artifacts are read from `bin/`.
Godot 4.7.2 hardcodes the `godot` binary stem and platform template names, so
this script renames the editor for users while keeping template filenames
Godot-compatible. Standard library only.

Usage:
    python3 misc/scripts/go_package_release.py --platform linux --bin-dir bin --output-dir dist
"""

# Copyright (c) 2026 SampleCraft / GO. Game Engine contributors
# SPDX-License-Identifier: MIT

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import shutil
import sys
import tempfile
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_GO_VERSION = "4.7.2.go.0.1.0"


def load_version_values() -> dict[str, object]:
    spec = importlib.util.spec_from_file_location("go_version_py", ROOT / "version.py")
    if spec is None or spec.loader is None:
        raise RuntimeError("cannot load version.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return {
        "name": getattr(module, "name"),
        "short_name": getattr(module, "short_name"),
        "major": getattr(module, "major"),
        "minor": getattr(module, "minor"),
        "patch": getattr(module, "patch"),
        "status": getattr(module, "status"),
        "go_version": getattr(module, "go_version", "0.1.0"),
        "go_release": getattr(module, "go_release", DEFAULT_GO_VERSION),
    }


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def zip_directory(source: Path, destination: Path) -> None:
    with zipfile.ZipFile(destination, "w", zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
        for path in sorted(source.rglob("*")):
            if path.is_file():
                archive.write(path, path.relative_to(source).as_posix())


def append_license(out: list[str], title: str, path: Path) -> None:
    out.append(f"\n\n===== {title} =====\n")
    if path.is_file():
        out.append(path.read_text(encoding="utf-8", errors="replace"))
    else:
        out.append(f"WARNING: license file missing: {path}\n")


def build_license_bundle(staging: Path) -> None:
    lines = [
        "GO. Game Engine — third-party license bundle",
        "This file is informational. The canonical notices and upstream license files",
        "are included in this release. See docs/go/THIRD_PARTY_NOTICES.md.",
    ]
    append_license(lines, "Godot Engine — MIT", ROOT / "LICENSE.txt")
    append_license(lines, "Kilo Code / opencode — MIT", ROOT / "docs/go/licenses/KILO-MIT.txt")
    append_license(lines, "Monocraft — SIL OFL 1.1", ROOT / "go_assets/fonts/LICENSE-Monocraft.txt")
    manifest = json.loads((ROOT / "go_bundled_plugins/plugins.json").read_text(encoding="utf-8"))
    for plugin in manifest["plugins"]:
        source = ROOT / "go_bundled_plugins" / plugin["id"] / plugin["license_file"]
        append_license(lines, f"{plugin['name']} {plugin['tag']} — {plugin['license']}", source)
    (staging / "THIRD_PARTY_LICENSES.txt").write_text("".join(lines), encoding="utf-8")


def plugin_status(staging: Path) -> None:
    manifest = json.loads((ROOT / "go_bundled_plugins/plugins.json").read_text(encoding="utf-8"))
    lines = ["GO. bundled plugin status", ""]
    for plugin in manifest["plugins"]:
        state = "source-vendored" if plugin.get("bundled") else "release-bundle pending"
        default = "default-on" if plugin.get("default_enabled") else "opt-in"
        lines.append(f"- {plugin['name']} ({plugin['tag']}): {state}, {default}, {plugin['license']}")
    lines += [
        "",
        "GDExtension binaries are built or checksum-verified by the release workflow.",
        "No runtime download from the Asset Library is required for source-vendored addons.",
    ]
    (staging / "BUNDLED_PLUGINS_STATUS.txt").write_text("\n".join(lines) + "\n", encoding="utf-8")


def platform_layout(platform: str) -> tuple[str, dict[str, str], list[str]]:
    if platform == "linux":
        return (
            "godot.linuxbsd.editor.x86_64",
            {
                "linux_release.x86_64": "godot.linuxbsd.template_release.x86_64",
                "linux_debug.x86_64": "godot.linuxbsd.template_debug.x86_64",
            },
            ["go.linuxbsd.editor.x86_64"],
        )
    if platform == "windows":
        return (
            "godot.windows.editor.x86_64.exe",
            {
                "windows_release_x86_64.exe": "godot.windows.template_release.x86_64.exe",
                "windows_debug_x86_64.exe": "godot.windows.template_debug.x86_64.exe",
            },
            ["GO. Game Engine.exe"],
        )
    raise ValueError("platform must be 'linux' or 'windows'")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--platform", choices=("linux", "windows"), required=True)
    parser.add_argument("--bin-dir", default="bin", help="SCons output directory")
    parser.add_argument("--output-dir", default="dist", help="release artifact directory")
    parser.add_argument("--go-version", default=None, help="GO. release label, default from version.py")
    args = parser.parse_args()

    values = load_version_values()
    version = args.go_version or str(values["go_release"])
    engine_config_version = f"{values['major']}.{values['minor']}.{values['patch']}.{values['status']}"
    bin_dir = Path(args.bin_dir)
    if not bin_dir.is_absolute():
        bin_dir = (ROOT / bin_dir).resolve()
    output_dir = Path(args.output_dir)
    if not output_dir.is_absolute():
        output_dir = (ROOT / output_dir).resolve()
    output_dir.mkdir(parents=True, exist_ok=True)

    editor_name, templates, editor_dest_names = platform_layout(args.platform)
    editor_source = bin_dir / editor_name
    if not editor_source.is_file():
        print(f"missing editor binary: {editor_source}", file=sys.stderr)
        return 1
    missing_templates = [name for name in templates.values() if not (bin_dir / name).is_file()]
    if missing_templates:
        print(f"missing export templates: {', '.join(missing_templates)}", file=sys.stderr)
        return 1

    with tempfile.TemporaryDirectory(prefix="go-release-") as temp:
        staging = Path(temp) / "GO. Game Engine"
        staging.mkdir(parents=True)

        editor_target = staging / editor_dest_names[0]
        shutil.copy2(editor_source, editor_target)
        editor_target.chmod(0o755)
        if args.platform == "windows":
            console_source = bin_dir / "godot.windows.editor.x86_64.console.exe"
            if console_source.is_file():
                shutil.copy2(console_source, staging / "GO. Game Engine.console.exe")

        for relative in (
            "README.md", "README.godot-upstream.md", "LICENSE.txt", "COPYRIGHT.txt",
            "AUTHORS.md", "DONORS.md", "CHANGELOG.md", "PROMPT.md",
        ):
            source = ROOT / relative
            if source.is_file():
                shutil.copy2(source, staging / relative)
        for directory in ("docs/go", "go_assets", "go_bundled_plugins", "go_project_template"):
            source = ROOT / directory
            if source.is_dir():
                shutil.copytree(source, staging / directory, dirs_exist_ok=True)

        build_license_bundle(staging)
        plugin_status(staging)

        template_dir = staging / "templates" / engine_config_version
        template_dir.mkdir(parents=True)
        (template_dir / "version.txt").write_text(engine_config_version + "\n", encoding="utf-8")
        tpz_dir = Path(temp) / "tpz"
        tpz_template_dir = tpz_dir / "templates"
        tpz_template_dir.mkdir(parents=True)
        (tpz_template_dir / "version.txt").write_text(engine_config_version + "\n", encoding="utf-8")
        for target_name, source_name in templates.items():
            source = bin_dir / source_name
            shutil.copy2(source, template_dir / target_name)
            (template_dir / target_name).chmod(0o755)
            shutil.copy2(source, tpz_template_dir / target_name)

        (staging / "INSTALL.md").write_text(
            "# GO. Game Engine\n\n"
            f"Release: {version} (Godot base {engine_config_version})\n\n"
            "1. Extract this ZIP.\n"
            "2. Start the renamed editor binary.\n"
            "3. Install `export_templates.tpz` from Editor → Manage Export Templates → Install from file,\n"
            f"   or copy `templates/{engine_config_version}/` into the GO. export templates directory.\n"
            "4. `go_bundled_plugins/`, `go_project_template/` and `go_assets/` are found offline next to\n"
            "   the executable. Do not download plugins from the Asset Library.\n",
            encoding="utf-8",
        )

        base = f"GO-Game-Engine-v{version}-{args.platform}-x86_64"
        zip_path = output_dir / f"{base}.zip"
        tpz_path = output_dir / f"{base}-export-templates.tpz"
        zip_directory(staging, zip_path)
        zip_directory(tpz_dir, tpz_path)
        shutil.copy2(tpz_path, staging / "export_templates.tpz")
        # Rebuild ZIP now that it contains the TPZ as well.
        zip_directory(staging, zip_path)

        sums = output_dir / f"{base}-SHA256SUMS.txt"
        sums.write_text(
            "".join(f"{sha256(path)}  {path.name}\n" for path in (zip_path, tpz_path)),
            encoding="utf-8",
        )
        print(zip_path)
        print(tpz_path)
        print(sums)
        print(f"OK: {version}, Godot template directory {engine_config_version}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
