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

## Current implementation checkpoint (2026-10-28)

- Full CI run `40649672758` has passed **Build editor** and **Verify --version branding**
  on Linux and Windows for commit `6a5596e5`. Templates were still running when checked.
  This evidence does not cover later unbuilt changes.
- Native Kilo HTTPRequest client and chat are now implemented: explicit Send/Refresh Models,
  timeout, cancellation, anonymous headers, catalog-based free-model validation and HTTP errors.
  Live gateway requests have not been tested here; mock HTTP tests cover request/response parsing.
- ProjectDialog now exposes Git Suite/LFS choices for new projects. Setup copies available
  plugin sources without enabling unproven defaults, sets splash/settings and initializes Git.
  Runtime verification and a visible nonfatal setup-warning dialog are still pending.
- Agent tool bridge and Automatic Commit controller are **drafts**, not completed features.
  Tool execution is disabled, and Automatic Commit has no active callback/button, until
  per-action confirmation, native tool-result messages, Undo/Redo, filesystem race protection,
  worker-thread Git execution and modal lock tests are implemented.
- Existing manual Git operations still run synchronously. Stage/Unstage/Diff UI, correct
  effective LFS attribute queries, real LFS transport tests and lifecycle reminders remain open.
- Header syntax checks now disable C++ exceptions like Godot. They are not runtime tests.

### Additional validation queued

- `go_lfs_transport_test.py` requires real git-lfs in CI and round-trips an actual binary
  through a local bare remote. Local execution explicitly skipped because git-lfs is absent.
- `go_editor_smoke.py` runs the compiled editor with `--headless --import` in an isolated
  project and fails on startup errors. It is queued after the editor build, not yet proven.
- JSON parsing now rejects excessive nesting and empty/malformed price strings; catalog-based
  request validation prevents selecting models solely by an unverified `/free` suffix.
- Setup preserves existing addon directories instead of overwriting project plugin sources.

## 2026-10-28 — full Kilo reference and build failures

- Full unfiltered Kilo clone verified at the existing pin (10 audited source hashes);
  `KILO_PORT.md` maps original sources to native implementation and missing ports.
- Latest full run `40660072217` failed: Linux static link of ProjectDialog setup;
  Windows startup inherited signal connection. Adapter placement and early class
  registration corrected; new full CI required before claiming either fixed at runtime.
- Requested LFS extension remains open: automatic size-based selection versus one
  or more recursively tracked folders, persisted configuration and preview. No
  history rewriting or silent activation; text-format exclusions must be explicit.

## 2026-10-28 — LFS policy implementation

- Previous full build `40702540665` passed Windows/Linux editor, startup smoke and
  templates for `bfd78bf7`; this does not validate the new LFS code.
- Added selectable size threshold and recursive multi-folder modes, project-local
  persistence, native folder picker, rule generation before staging and effective
  Git attribute pointer checks. Legacy file-type mode remains the default.
- Fixed managed-block replacement at end-of-file; switching modes no longer leaves
  obsolete exact-path/folder rules behind.
- Added policy boundary tests and real Git attribute matcher tests; CI real-LFS
  transport now exercises generated multi-folder rules and text exclusions.
- Still open: visual UI acceptance, asynchronous Git, safer native tool protocol,
  approvals/Undo and Automatic Commit activation. Size mode runs on rule refresh /
  staging, not continuously. Local LFS config is intentionally not cloned/shared.

## 2026-10-28 — native tool conversation prerequisite

- LFS quick checks `40745850623` passed, including real generated-folder LFS
  transport; full build `40745850770` was still running at this checkpoint.
- Added typed native conversation serialization and wired the HTTP client to it.
  Tool call IDs/results must match; invalid/incomplete batches fail before HTTP.
  Catalog tool capability is checked before sending native definitions.
- Pure tests cover parallel calls, two rounds, orphan/duplicate IDs, malformed
  arguments and the history round cap. 26 native C++ syntax checks pass.
- Found missing upstream copyright banners in touched sources; added them to
  address static hook failures. Full static CI still needs confirmation.
- Execution remains disabled: per-action approvals, Undo/Redo, asynchronous Git,
  complete session lifecycle and Automatic Commit are still unfinished.

## 2026-10-30 — asynchronous dock Git and strict tool contracts

- Baseline `56ab458c`: full Windows/Linux build `40746646418`, quick checks
  `40746646940` and static checks `40746646586` all passed. LFS full build
  `40745850770` also passed.
- Implemented nonblocking Fetch/Pull/Push/Commit, including branch/upstream phases,
  timeout, bounded logs, cancellation and teardown. Native process tests added;
  CI editor builds now include tests and execute that subset. New runtime proof
  is pending; status/LFS/setup/Automatic Commit are not yet async.
- Replaced generic tool parameter schemas with per-tool arguments and strict
  bridge validation. Actions that may execute scripts now require approval
  classification. Auto-Approve cannot imply that disabled execution is available.
- Local guardrails, real Git local flow, 27 C++ translation units and both native
  test header syntax checks pass. These are not a visual/UI execution acceptance.
- Remaining: confirmations, scene/resource Undo/Redo, complete safe agent loop,
  lifecycle reminders, remaining async Git paths, Automatic Commit activation,
  screenshots and full player/export E2E acceptance. Do not mark the prompt done.
