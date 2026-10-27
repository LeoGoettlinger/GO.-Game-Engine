# GO. changelog

## 4.7.2.go.0.1.0 — in development

### Added

- Stable Godot 4.7.2 source baseline and documented Kilo/Monocraft reference pins.
- GO. architecture, upstream-patch register, build instructions and honest roadmap.
- GO. branding in `version.py`, `--version`, About dialog, Project Manager, app icon and README.
- Reproducible game/editor splash PNGs with the exact SampleCraft line in Monocraft (OFL-1.1).
- `go_assets/make_splash.py`, `go_assets/go_logo.svg`, project-template basics and a branding check script.
- `go-build.yml` (Windows/Linux editors, templates, tagged releases) and `go-quick-checks.yml`
  (branding, manifest, YAML, splash reproducibility, GDScript parsing, packaging smoke test).
- `go_bundled_plugins/plugins.json` with 12 license-checked pins, seven SHA-256-verified
  GDScript plugins vendored offline, and reproducible vendor/validate/check scripts.
- `go_package_release.py` producing a release ZIP, Godot-compatible `.tpz`, license bundle and
  SHA-256 checksums.
- Editor-only `modules/go_suite` with `GoGitDock`, `GoAgenticsDock`, the suite launcher, three
  generated SVG dock icons and a checked-in icon header.
- Pure C++ guardrail layers for remote parsing, `res://` paths, LFS rules, free-model filtering,
  commit-message sanitizing, auto-commit state and the Agentics tool whitelist.
- Standalone guardrail runner, mock Kilo Gateway test, forbidden-UI source scanner and Godot
  doctest module tests; all run locally without a compiled editor.
- Byte-stable splash/app-icon source PNGs under `go_assets/rendered/` and a headless `--import`
  warm-up in the official-binary parse job, so quick checks are reproducible on GitHub runners.
- Minimal, documented ProtonScatter `_get()` return patch so all 397 vendored GDScript files
  parse under the official Godot 4.7.2-stable binary.
- GO. Quick Checks and upstream static checks green on GitHub Actions; the inherited
  full-platform build matrix is gated off on the GO. branch in favor of `go-build.yml`.
- User-facing `Features.md` comparing GO. with upstream Godot, with explicit
  implemented/partial/planned status and a short English summary.
- Editor-only Git Suite step 5: pure `go_system_git.*` command builder/parsers,
  `go_system_git_runner.*`, `go_git_controller.*`, `GoRemoteAssistant`, `GoLfsDialog`
  and a fully wired `GoGitDock` for status/commit/pull/push/log/LFS.
- `go_git_flow_test.py` local bare-repo integration test with managed `.gitattributes`,
  large-file classification and a synthetic LFS pointer in the staged index.
- `go_syntax_check.py` parses all 23 Godot-facing module C++ files against real Godot 4.7.2
  headers in quick CI; it is a syntax check, not a full engine build.
