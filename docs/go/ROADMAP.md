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

## Scene-only execution checkpoint

- `5c1cc216` passed full build `41069106792`, quick checks `41069106910`, and
  static checks `41069107083`, including native process tests on both platforms.
- Added opt-in experimental native scene-tool loop with four enabled tools, default
  per-write confirmation, denial, Stop during approval, target/scene identity checks,
  strict runtime subset, correlated history and 20-individual-call budget.
- Add/rename use fixed scene undo history; creation is limited to six basic built-in
  classes and rejects scripted ancestors. New-conversation resets Auto-Approve.
- Added native fixed-history add/rename/owner restoration tests to both CI builds.
  Local guardrails and 27 source / 3 test-header syntax checks pass. New native runtime
  tests and visual modal/Undo acceptance remain pending.
- Still NOT complete: unrestricted Godot tool suite, JSON fallback execution,
  file/resource edits and their safety, persisted chat history, lifecycle reminders,
  remaining async Git paths, deterministic Automatic Commit and full export/player E2E.

## Lifecycle wiring and native-test crash correction

- `693cf422`: quick run `41324302239` and static run `41324302273` passed. Full run
  `41324302361` failed on BOTH platforms in the new SceneUndo test after building
  the editors. It crashed before assertions: EditorUndoRedoManager creates histories
  through live EditorNode log/debugger singletons absent in this test harness.
- Shared operation recording now takes core UndoRedo. Production obtains that core
  history from the live scene manager; the native test instantiates core UndoRedo
  directly. No assertion was removed. New runtime evidence is still required.
- Added asynchronous startup fetch/status and final-close status/LFS reminders,
  local one-time activation offer, persistent per-project opt-out, optional Pull,
  modal close progress, Cancel/Close Anyway and exact upstream quit continuation.
- Added owned-job exclusion and a close-pending guard for scene tools/late responses.
  Pure reminder decisions and the new upstream hook are checked locally.
- Open: visual lifecycle/approval/Undo acceptance; Automatic Commit (including its
  lifecycle button), remaining file/resource tools and async paths, persisted chat,
  whole-editor lock validation and full player/export E2E. This is not completion
  of the entire PROMPT.md.

## Automatic Commit asynchronous phases and privacy checkpoint

- `6b09bc5d`: full build `41330022968`, static checks `41330022891`, and quick
  checks `41330022882` passed. This includes the corrected native core Undo test.
- Converted the draft controller's diff reads (staged + unstaged), add/renormalize,
  commit and push to nonblocking phases. Local preflight, LFS rule preparation
  and pointer verification still need conversion; activation remains blocked.
- Fixed self-interruption caused by reparenting the running controller. The overlay
  now moves independently; owned input/job state is restored on exit paths.
- Added native synthetic-root overlay ownership/failure/teardown tests to CI. The
  new runtime results and real-editor/native-subwindow interaction remain pending.
- Hardened prompt redaction and metadata/diff bounds; real Git fixtures and pure
  tests pass locally, as do 28 source + 4 native test-header syntax checks.
- Remaining: end-to-end transaction and rejected-push/UI-unlock evidence, complete
  nonblocking preparation, safe activation and lifecycle integration of Automatic
  Commit, remaining agent file/resource tools and full export/player acceptance.

## Automatic Commit asynchronous preparation checkpoint

- Converted remaining Automatic Commit Git subprocesses: preflight, LFS install,
  file discovery, cached attribute checks and staged blob verification. No synchronous
  Git runner remains in that controller. Size-policy stat work is bounded to 16
  files per frame; local file I/O is not claimed to be nonblocking.
- Added raw NUL-delimited parsing, output/list/attribute bounds, strict complete
  pointer validation, status recheck under the input lock and separate ownership
  of the preflight job reservation. Fixed loss of the result message on dock refresh.
- Added native offline transaction fixtures for no changes, local commit,
  successful/rejected push, invalid pointers, real LFS and preflight teardown;
  both platform workflows run them. Runtime results are pending.
- Activation remains blocked on native transaction results and whole-editor/native
  window input-lock acceptance. On-close integration, remaining safe agent tools,
  chat persistence and release/export/player/screenshots remain unfinished.
- Previous `182a9e6d`: quick/static green; full Windows/Linux build still running
  at this checkpoint. Local guardrails, real-Git flow and 28 sources/5 native
  headers pass syntax checks; these are not substitutes for the runtime fixtures.

### Native test-build configuration correction

The `ac64d508` full build failed on both platforms before runtime tests: upstream
`TESTS_ENABLED` is not globally defined, so the test-only message fallback member
was missing in `tests/test_main.cpp`. GO. now sets its own consistent
`GO_SUITE_TESTS_ENABLED` define through module configuration for test builds.
Local syntax CI now checks both production and test configurations rather than
inventing an upstream macro only for test headers. Both configurations pass
locally; the corrected full-build/runtime result remains pending.

### Multi-window input ownership

Automatic Commit now snapshots input state for every live Viewport (including
native Window nodes and embedded SubViewports), subscribes to new nodes for the
lock duration, and restores surviving objects by ObjectID. New native regression
coverage includes pre-disabled, added and deleted viewports, plus disconnect on
unlock. The editor quit guard remains in place. This is stronger than disabling
only the root viewport, but visible native-window/shortcut acceptance is still
pending; Automatic Commit is not enabled solely on this change.

### Project-local chat history implemented

Added a bounded, versioned, display-only archive in `.godot/editor`, read-only
History dialog and confirmed Clear History. Old actions/requests are never replayed
or sent on load. Storage refuses symlink components, preserves malformed files,
and uses a recoverable previous backup for interrupted replacement. Pure format,
Unicode/retention/error tests pass locally; native restart/recovery/clear fixtures
are wired into both full-build jobs, with runtime results pending. This closes the
missing persistence implementation, not the remaining agent tool/visual acceptance
work. Production and test syntax now cover 29 module sources and six native headers.

### Automatic Commit close integration implemented, activation still gated

Wired the close reminder to an explicit commit-before-close path that retains the
quit reservation, defers continuation until unlock, and keeps the editor open on
failure. Existing Git jobs finish before close status is rechecked; canceling
close does not terminate another job. Native real-Git close-continuation fixtures
are added for success/failure/cancel/explicit close and duplicate callbacks.
The shared availability gate remains off until runtime and visual acceptance;
this checkpoint removes the missing integration implementation, not its acceptance
blocker. Local production/test syntax covers 29 sources and seven native headers.

### Scene undo/approval expansion

Expanded the opt-in native scene loop from four to eight tools with undoable
remove, move and restricted scalar property operations. Removed the destructive
live-node deletion path for remove. Added recursive owner restoration, destination
and cycle checks, strict integer index schemas, property type/range guards, and
scene-version/destination identity validation after approval. Native shared-operation
regressions added; local standalone/schema and production/test syntax checks pass.
Runtime/visual acceptance is pending. This does not complete the file/resource/run
and full agent-tool acceptance requirements.

### Export/player CI acceptance gate added

Added a no-network player fixture with CharacterBody2D, Sprite2D and CollisionShape2D;
full CI imports/runs it, exports using the actual custom debug and release templates,
and runs the resulting embedded-PCK executables in source-free directories. Exit
codes, engine errors, completion markers and editor-class exclusion are checked.
Logs/results upload even on failure, and failed exports prevent release packaging.
Local orchestration tests (three cases) and Python lint/type checks pass. Actual
exports are pending the full builds. This is not evidence of AI scene creation,
visual screenshots or packaged-release/project-manager E2E.

### Runtime findings and test harness correction

The older `c742faa2`, `fdb84369` and `8716427f` Linux editor/template jobs passed.
Windows stopped at AutoFlow without the failure annotation, so its cause is not
yet established: the shell success-count check could fail without preserving the
underlying output in an API-readable annotation. All native suites now run through
a bounded Python subprocess runner with separate user/cache directories and a
required complete doctest XML report. Zero exit, missing/empty output, zero tests
and partial reports cannot pass. XML and console diagnostics are uploaded and
annotated on every failure, including startup/cleanup failures. Local harness tests
are not a substitute for Windows execution.

The `2799dd13` Linux native SceneUndo test exposed a fixture error: a detached
subtree retains owners until it exits a live SceneTree. Remove/reparent regression
now attaches its fixture to the mock editor tree, exercising actual editor ownership
invalidation. Add-node undo also explicitly clears ownership before detaching, so
its isolated-history redo cannot reinsert a node with a stale owner. Full corrected
runtime results remain pending; Automatic Commit remains gated.

### Read-only project tools implemented

Enabled bounded list/read/search alongside the eight opt-in scene tools (11 total).
Added bounded raw reads/UTF-8 checks, before-range whole-file secret screening,
input/output/traversal/search limits, and explicit non-exhaustive search reporting.
Fixed cross-platform protected-path bypasses through `.GIT` case, trailing dot/space,
Windows devices and short-name/hidden Unicode variants. No script evaluation or
file writing is enabled by these tools. Native read/search/privacy/outside-symlink
fixtures added to full CI; local pure/schema/privacy/Git tests and syntax checks
pass. File mutation, resource/run tools and live-agent/visual E2E remain open.

### Runtime follow-up: missing attributes and actionable XML failures

`5cb50fbb` passed its Linux native suites, including live scene ownership, automatic
commit/close, local history and read tools, then failed the new export/player gate.
Windows still failed AutoFlow; its diagnostic reports a size query on a missing
`.gitattributes`. Rule creation now checks existence before querying size, avoiding
Windows' missing-file engine error. This removes a concrete error, not yet proof of
the complete Windows transaction passing. XML diagnostics now retain executed test
cases/contexts instead of a tail dominated by skipped tests. Export failures now
emit the actual failing step/log excerpt as an API-readable annotation. Both fixes
await full runtime confirmation; the shared activation gate remains off.

### Dateiänderungen: abgesichertes Backend, noch nicht freigeschaltet

- Vorbereitungs-/Ausführungs-API bindet eine Änderung an Projektpfad, Ziel, Operation,
  ursprüngliche Bytes und neuen Inhalt. Eine veraltete Freigabe wird verworfen.
- Bestehende Dateien erhalten eine bytegetreue Sicherung in
  `.godot/go_agent_backups` gesichert (maximal 200 Einträge); Ersatzdateien werden
  zuerst getrennt geschrieben. Geladene Ressourcen/offene Skripte, Addons,
  versteckte und sensible Pfade sowie Symlinks werden abgelehnt.
- Zunächst nur kleine UTF-8-Dateien (`gd`, `txt`, `md`, `json`, `csv`);
  Skripte mit Tool-/Static-/Preload-Inhalten werden konservativ abgelehnt.
- Zwei zusätzliche native Regressionen prüfen veraltete Freigaben, Sicherungen,
  überlappende Suchtreffer und geschützte Ziele. Windows-/Linux-Lauf steht aus.
- **Nicht freigeschaltet:** Der vollständige Datei-Vorschau-/Freigabedialog fehlt.
  Die Modell-Whitelist bleibt bei elf Werkzeugen. Sicherungen sind kein
  Editor-Undo/Redo. Kein Schutzversprechen gegen gleichzeitig manipulierende
  externe Prozesse, keine Skript-Sandbox und keine Crash-Durability-Garantie.
