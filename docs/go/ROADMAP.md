# GO. roadmap and honest status

Legend: `[x]` implemented and locally verifiable, `[~]` implemented but not yet proven in a
compiled editor/CI, `[ ]` open. A user-facing feature comparison lives in
[`Features.md`](../../Features.md).

## Foundation

- [x] Stable Godot 4.7.2 base and documented upstream remote.
- [x] Kilo and Monocraft references pinned outside the committed source.
- [x] `docs/go/` scaffold and upstream-patch register.
- [x] Large build/release workflow and fast PR workflow authored; YAML and all local quick checks pass.
- [x] GO. Quick Checks green on GitHub Actions, including official Godot 4.7.2-stable parsing of
  all 397 vendored GDScript files; upstream static checks also pass on the GO. branch.
- [~] Editor-only `modules/go_suite` source, docks, icons and guardrails present; full Godot
  C++/SCons compile is still CI-only.
- [x] All 23 Godot-facing module C++ files pass a real-header `-fsyntax-only` check in quick CI
  (`go_syntax_check.py`); this is not a full link/build.
- [ ] Full Windows + Linux editor/template build proven green in GitHub Actions.
- [ ] Full release ZIP verified on a clean offline machine.

## Branding

- [x] `version.py`, About, Project Manager, `--version` and GO. release label.
- [x] Editor + game splash images with exact SampleCraft text in Monocraft.
- [x] `go_assets/make_splash.py` reproducibility and `misc/scripts/go_branding_check.py`.
- [ ] Screenshot of the *running* editor splash from a compiled editor (generated PNGs are reviewed locally).

## Git Suite

- [~] Two built-in docks with icons and side-dock buttons (source present, compiled-editor screenshot pending).
- [x] Host parser, LFS rules, commit-message sanitising, auto-commit state and system-Git
  command builders/parsers with standalone and Doctest unit tests.
- [~] Status, stage, diff, commit, pull, push, log and remote assistant wired to system Git
  in source; all module C++ passes a real-header syntax check. Compiled-editor UI smoke test pending.
- [~] Git LFS install/init/`.gitattributes`/large-file/pointer/quota flows wired in source;
  real `git lfs` binary and OS installer commands still need a compiled-editor/CI smoke test.
- [x] Local bare-remote end-to-end test (`go_git_flow_test.py`) with managed `.gitattributes`,
  large-file classification and a synthetic LFS pointer in the staged index.
- [ ] Open/close reminders with per-project disable switches.

## Automatic Commit

- [~] Pure stage machine and guaranteed-unlock invariant unit-tested.
- [ ] Six-stage overlay that swallows all editor input and window-close.
- [ ] Kilo `kilo-auto/free` message generator with sanitising and fallback wired to network.
- [ ] LFS-aware staging, commit, push and honest intermediate-state errors.

## Code Agentics

- [~] Anonymous free-only model filter, 429 parsing and mock-gateway test.
- [~] Native chat dock with locked workspace and no forbidden settings (source present, compiled-editor screenshot pending).
- [~] Whitelisted tool schemas and `res://` path guardrails unit-tested.
- [ ] Real HTTP client wired to Godot `HTTPRequest`.
- [ ] 20-step tool loop with native tool calling and JSON fallback.
- [ ] Player smoke test with screenshot/log in a compiled editor.

## Plugins and lifecycle

- [x] Manifest with 12 pinned, license-checked entries and offline validation.
- [x] Seven GDScript plugins source-vendored with SHA-256 tarball pins and provenance.
- [x] GDScript parse-check orchestration (real binary check runs in CI; stub-tested locally).
- [ ] GDExtension plugins bundled via CI with checksums.
- [ ] `default_enabled=true` after a compiled-editor plugin startup smoke test.
- [ ] New-project checkbox, offline copy, project settings, `git init`, boot splash.
- [ ] Empty GO. project opens without plugin warnings; export test passes.

## Known limits

- Anonymous Kilo free tier: roughly 200 requests/hour/IP; no account workaround.
- Models without tool calling are weaker and use the JSON fallback.
- Editor-log access depends on the Godot 4.7 API and may be best-effort.
- macOS signing/notarisation is out of scope.
- C#/.NET is disabled.
- `go_suite/config.py` requires the built-in `svg` module for dock icons; a no-SVG editor build
  therefore disables `go_suite` until a fallback icon path is selected.
- This sandbox does not ship a prebuilt 4.7.2 editor; UI screenshots and compiled-editor
  smoke tests must be produced by CI or on a developer machine. The GitHub release-asset CDN
  is not in the sandbox host allow-list, so the real GDScript parse check is CI-only.
