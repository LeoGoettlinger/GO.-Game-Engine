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
`res://addons/<plugin>/` at project creation. `GO. → Bundled Plugins` can perform the same
copy/activate/deactivate action offline for an already-open project. GDExtension binaries are
release-bundle artifacts; they are not compiled into the engine core or committed as large
blobs here.

The release package includes a generated `BUNDLED_PLUGINS_STATUS.txt` so a user can see which
GDExtension binaries are present and which are still pending.

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
