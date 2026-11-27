# Bundled plugins

`go_bundled_plugins/plugins.json` is the offline catalog. Each entry pins an upstream tag
**and** 40-character commit, license, Godot minimum, addon path, type, default activation and
category. `go_validate_plugins.py` checks the manifest, `plugin.cfg`, license files,
`.origin.json` provenance, checksums and symmetric mutual-exclusion rules. The quick PR
workflow parses vendored `.gd` files with the official Godot 4.7.2-stable headless binary.

## Current status

All plugins are **opt-in** (`default_enabled=false`) because plugin startup has not yet been
smoke-tested in a compiled GO. 4.7.2 editor. This is deliberate: a missing plugin test must
not produce a broken new-project default.

| Plugin | Pin | Type | Status |
| --- | --- | --- | --- |
| Godot Git Plugin | `v3.3.0` / `542d9e0…` | GDExtension | release-bundle pending |
| Dialogic 2 | `2.0-alpha-20` / `e301e1b…` | GDScript | not vendored (alpha; needs editor smoke test) |
| Dialogue Manager | `v4.1.0` / `a719088…` | GDScript | **vendored**, opt-in |
| Phantom Camera | `v0.11.0.3` / `cb6e096…` | GDScript | **vendored**, opt-in |
| LimboAI | `v1.8.1` / `e45e60e…` | GDExtension | release-bundle pending |
| Beehave | `v2.9.3` / `773a5f6…` | GDScript | **vendored**, opt-in |
| Godot State Charts | `v0.22.5` / `76d226a…` | GDScript | **vendored**, opt-in |
| Terrain3D | `v1.0.2-stable` / `0077405…` | GDExtension | release-bundle pending |
| ProtonScatter | `4.0` / `ca62441…` | GDScript | **vendored**, opt-in (demos excluded) |
| GUT | `v9.6.1` / `c80954f…` | GDScript | **vendored**, opt-in |
| Aseprite Wizard | `v9.8.0-4` / `dd7d4e2…` | GDScript | **vendored**, opt-in |
| Debug Draw 3D | `1.7.3` / `c6d9112…` | GDExtension | optional release-bundle pending |

All twelve upstream repositories were checked via the GitHub API. GitHub reports MIT for ten
of them. GUT's license was manually verified from `addons/gut/LICENSE.md`, and Debug Draw 3D's
from its root `LICENSE`; both are MIT. The vendored tarballs are SHA-256 pinned in the manifest,
and `misc/scripts/go_vendor_plugins.py` can reproduce the source-vendored directories from
those exact commits without adding tarballs or `node_modules` to Git.

Dialogic 2 and Dialogue Manager are mutually exclusive. No two conflicting dialogue systems
are activated together. Aseprite Wizard requires an external Aseprite installation; GO. does
not ship Aseprite.

## Runtime lookup and copying

The editor looks for `go_bundled_plugins/` next to the executable, then in the editor data
directory, then in the source tree. Pure GDScript addons are copied to
`res://addons/<plugin>/` at project creation. The `Bundled Plugins` button is currently disabled; its handler is a placeholder.
Offline copy/activate/deactivate management for an already-open project is not
implemented yet. Use Godot’s existing plugin controls for manually installed addons. GDExtension binaries still need a pinned build/download-and-checksum release
pipeline; they are not currently included, compiled into the engine core, or
committed as large blobs here.

The release package includes a generated `BUNDLED_PLUGINS_STATUS.txt` which labels the four
GDExtension entries as pending. That status file is not proof of bundled binaries.

## Maintainer commands

```sh
python3 misc/scripts/go_validate_plugins.py
python3 misc/scripts/go_vendor_plugins.py --check
python3 misc/scripts/go_vendor_plugins.py --update-checksums   # only when intentionally bumping a pin
python3 misc/scripts/go_check_gdscript.py --godot /path/to/Godot_v4.7.2-stable_linux.x86_64
```

The local sandbox could not download the official release binary because its CDN host is not
allow-listed, so `go_check_gdscript.py` was exercised locally with a stub executable that
returns success. The real parse check runs in GitHub Actions, where the release binary and
`SHA512-SUMS.txt` are available and checksum-verified. It parses all 397 vendored `.gd` files
after a headless `--import` warm-up. The CI run uncovered one upstream ProtonScatter 4.0
function that Godot 4.7.2 rejects; the minimal compatibility fix is recorded in
[`go_bundled_plugins/PATCHES.md`](../go_bundled_plugins/PATCHES.md).

### Offline plugin management implementation checkpoint

The earlier disabled-plugin-manager blocker is now replaced by an offline catalog
in the Git Suite. Install copies to a staging folder, refuses existing addon
folders and linked paths, and never downloads, overwrites or deletes user files.
Enable/disable requires explicit confirmation; enabling refuses a currently
enabled conflicting dialogue addon. Disabling retains files. Failures retain
staging data for inspection. The manager refuses operations during Git/editor
locks, project execution or filesystem scanning. Import and plugin activation
execute third-party code and are not sandboxed or scene-undoable.

Dialogic 2 alpha-20 is now source-vendored at the existing pinned commit with
SHA-256 provenance and unchanged MIT license; eight GDScript addons are bundled.
New-project copying includes Dialogic, but activation remains opt-in pending
actual startup/compatibility checks. Native path-safety regression coverage is
added. The dialog itself and Dialogic startup still require runtime/visual
acceptance. Four GDExtension binary bundles remain outstanding.

### Checksum-pinned extension packaging gate

All four GDExtension release assets now have fixed upstream URLs and SHA-256
values recorded from GitHub release metadata. A maintainer-only CI bundler
verifies the digest before safe extraction, rejects traversal/links/ambiguous
addon roots and requires a target-platform library. Upstream licenses fetched
from the pinned source commits are included. Packaging merges only bundles with
matching provenance and reports them separately from source-vendored addons.
A short dedicated workflow validates Windows/Linux archive layout without an
engine rebuild; the full release job runs the same bundler before packaging.

Five offline archive tests cover checksum rejection, traversal, ambiguous roots,
missing target binaries and no-overwrite behavior. Local real downloads are
blocked by the sandbox's GitHub release-CDN restriction, so actual archive
layout/download and native startup acceptance are pending CI. LimboAI uses its
published GDExtension 4.6 artifact, not a replacement upstream editor; compatibility
with GO. 4.7.2 must still be checked. GDExtensions without plugin.cfg are not
currently installable through the GDScript plugin dialog. No binary is committed.

Dialogic's upstream automatic updater was also disabled: its check/download
methods return unavailable/failure without HTTP requests. This additive source
patch is recorded in PATCHES.md and its provenance; pinned bundles must not
self-update when opened offline.

### Native-only addon installation

The offline manager now also recognizes root `.gdextension` descriptors without
`plugin.cfg`, allowing verified release addons such as Godot Git Plugin to be
copied. Consent explicitly warns that import loads native machine code. Native
extensions without an EditorPlugin switch cannot be disabled/unloaded from this
dialog; no unsafe file deletion or unload is attempted. For addons with both,
disabling the editor plugin does not unload the native library. Native startup
acceptance and the actual UI interaction remain pending.
