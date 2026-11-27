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

### Follow-up: file approval UI and runtime failure fixes

The experimental project-tool whitelist now has 16 tools. File changes always
show a complete before/after dialog, even with scene auto-approval enabled.
Reject/Stop clear approval state; execution revalidates the fingerprint. This
supersedes the preceding note that the file dialog was missing.

Run 42221639673 crashed in the new ReadTools fixture: EditorInterface exists in
native tests without an EditorNode. The open-scene check now requires both.
Run 42220860133 identified missing include_filter/exclude_filter fields in the
export fixture; those required preset fields are now supplied and asserted.
Windows AutoFlow still failed without its diagnostic annotation. The report
printer now escapes Unicode for legacy Windows console encodings, with a
regression using an ASCII-only stream; the underlying Windows failure is not
yet diagnosed or claimed fixed. Automatic Commit remains disabled.

Added native approval-dialog, project-switch and loaded-resource regressions.
Full native runtime, export and visual acceptance still await the next CI run.

### Runtime acceptance at 13b24d8f and Windows empty-argument fix

Full run 42273906776 passed the entire Linux job, including native file approval,
read/write guards, Automatic Commit transactions and both exported player builds.
Windows failed only at AutoFlow before reaching the later suites/export gates.
The retained XML shows the invalid-LFS negative fixture unexpectedly committed;
normal commit/push/rejected-push and the real LFS filter test passed.

The base Windows process adapter drops empty arguments. Consequently, the test's
`git config filter.lfs.process ""` became a query of the inherited filter, returned
success, and never disabled the filter. Both GO. Git launch paths now preserve
empty argv entries on Windows. A dedicated native test verifies empty config
writes through synchronous and asynchronous launching; the negative LFS fixture
also verifies its local config values before running the transaction. Windows
runtime confirmation remains pending. Automatic Commit stays gated until full
runtime and visual acceptance; Linux success alone is not cross-platform proof.

### Current additions: script syntax and project-setting reads

`validate_script` now parses saved GDScript text without loading or initializing
Script resources. Success explicitly excludes semantic/dependency/runtime checks.
`get_project_setting` exposes four string-only non-secret settings with bounds.
The experimental whitelist is now 18 tools; native tests are added but await CI.
The full Windows/Linux run for the preceding empty-argument fix is still running.

Outstanding tool families remain scene creation/open/save/instantiation, script
attachment, resource creation, project-setting writes/main-scene selection,
filesystem scan/selection, run controls, editor logs and agent Git integration.
These are not claimed implemented by adding the two inspection tools.

### New scene/resource creation through the file-approval boundary

`create_scene` and `create_resource` now create new .tscn/.tres files from bounded
built-in type allowlists. They never overwrite or auto-open a scene; both use the
existing full file preview, explicit approval and stale-state guard. The old
create-scene draft that saved/opened a hardcoded Node2D was removed. Native tests
load generated resources, instantiate the scene with initialized physics servers,
and verify denied types/names/overwrites. The whitelist is now 20 tools. Full
runtime and visual acceptance of these additions is still pending.

### Cross-platform native checkpoint and physics node gap

At c9960d2f, run 43481803336 passed editor compilation/startup and all native GO.
suites on both Windows and Linux, including the previously failing AutoFlow test
and new script-validation/scene/resource tests. Both jobs were building debug
templates when inspected; final exports and visual acceptance are not yet proven.

Added CharacterBody2D/3D and CollisionShape2D/3D to the bounded add-node factory,
with a native SceneTree undo/redo regression. This removes the missing physics-node
restriction for the requested player structure, not the remaining script-attachment,
resource assignment, scene open/save/main-scene and execution-tool gaps. Runtime
confirmation of the new node test remains pending. Automatic Commit stays gated.

### Scene opening and run/stop controls

The experimental whitelist now includes open_scene, run_project,
run_current_scene and stop_project (24 tools total). They have a separate explicit
execution approval that cannot be bypassed through scene auto-approval or the
normal bridge entry point. Approval snapshots bind scene/project/run state and
are revalidated before dispatch. Unsaved scenes and script buffers block opening
and running; project execution is explicitly not a sandbox. Successful dispatch
is reported as requested, not completed. Native denial/bypass tests are added;
real-editor execution/visual acceptance of this addition remains pending.

The previous 963090f2 Linux full job is successful. Its Windows job had passed
native suites and was still building the debug template at inspection time.
Remaining major tool gaps include scene save/save-as/instantiation, script/resource
assignment, project-setting writes/main-scene selection, and agent Git/log access.
Automatic Commit activation and full visual/end-to-end acceptance remain open.

### Automatic Commit activation after cross-platform runtime success

Full run 43511344381 for b21eef10 passed Windows and Linux editor, native suites,
templates and export/player gates. Automatic Commit is now registered while its
controller is in the scene tree and unregistered on exit/destruction. The dock
button is enabled for repositories after Refresh and disabled during Git work.
A separate confirmation explains staging/commit/push and anonymous Kilo diff
transmission with possible prompt logging and imperfect secret filtering. The
close reminder includes the same warning. Registry availability and close-flow
fixtures now exercise production activation instead of injecting a callback.
These activation/UI changes still require their own runtime/visual acceptance;
prior green results do not prove the new confirmation dialog visually correct.

### Guarded main-scene selection

`set_main_scene` is now the 25th experimental tool. Its required `path` must point
to a bounded readable res:// .tscn file. It serializes current ProjectSettings
with that main-scene override into a temporary local preview, without changing
project.godot or the in-memory main scene during preparation. The preview shows
all configuration differences, including other pending settings, and explicitly
warns that approving it saves those differences too.

The existing mandatory file confirmation, byte backup and replacement path apply.
The fingerprint also binds the target scene bytes and previous in-memory main
scene. A changed target, project configuration or pending settings invalidates
approval. The in-memory main scene is updated only after successful replacement.
It does not load/instantiate the scene or claim its dependencies are valid. Native
regressions cover no pre-approval mutation, stale target rejection, successful
configuration update and missing targets; runtime CI is pending.

### Guarded scene save requests

`save_current_scene` and `save_scene_as` are now enabled through mandatory editor
execution approval (27 tools total). Save As requires a new .tscn target unless
it is the current scene's own path; foreign existing scenes are never replaced.
Current-scene edits may be saved, but other unsaved scenes and script/text buffers
block the action. Project/scene/history/target bytes are fingerprinted, existing
bytes are backed up through the shared bounded backup helper, and state is
rechecked after backup before calling the editor.

The dialog warns about normal save callbacks and external-resource effects.
Godot's save_scene_as API returns no final save error, so the result deliberately
says save requested and directs the user to inspect editor errors. No success
claim is inferred from file existence. Native bypass/missing-editor tests now
cover both save actions; actual save UI/round-trip acceptance remains pending.

### Script attachment through explicit execution approval

`attach_script` is enabled (28 tools), with required `script_path` (.gd) and
optional scene-relative `node_path` (default root). It accepts only local nodes in
non-inherited scenes that do not already have a script, so existing script state
is not discarded. Unsaved script buffers, foreign targets and stale approvals
are refused. Approval explicitly warns that resource loading can execute static
initialization/tool scripts; there is no sandbox or rollback of those effects.

After approved loading, the bridge revalidates and re-resolves the target, checks
script validity and native base compatibility, and records assignment through the
editor undo history. The old direct set_script draft was removed. A native test
checks resource retention across assignment/undo/redo; denial tests cover missing
editor/approval. Full actual-editor attachment acceptance is still pending.

### Resource assignment for player components

`assign_resource` adds a 29th tool: required res:// .tres `path`, required
`property`, optional scene-relative `node_path`. Supported targets are
Sprite2D.texture (Texture2D) and CollisionShape2D/3D.shape (Shape2D/3D). Local
non-scripted targets only; existing resource assignments are retained for undo.
The approved target, resource bytes, old resource identity and history are checked
again after loading. Loading can execute dependency scripts and therefore always
requires explicit execution approval, not scene auto-approval. Only assignment,
not arbitrary initialization effects, is undoable. GradientTexture2D is now also
allowed by create_resource. Native shape-reference undo/redo and bypass tests are
added; real editor assignment and visual validation remain pending.

### Scene instantiation with approved initialization

`instantiate_scene` adds a 30th tool: required .tscn `path`, required unique ASCII
`name`, optional local parent `node_path`. It loads/instantiates only after an
explicit execution warning and then rechecks parent/history/file state before
inserting through UndoRedo. The parent must be locally owned and non-scripted;
inherited scenes and name collisions are refused. Instances over 1000 nodes are
rejected after creation; this is not a pre-allocation sandbox. Loading and script
initialization effects are not undone. Nested scene ownership is preserved by the
shared insertion path and covered by a new native undo/redo fixture. Real-editor
instantiation acceptance still needs the new full CI run.

### Actual-editor tool-chain and graphical evidence gate

The full build now creates its export fixture through production tool-bridge
calls inside a running editor, rather than writing a prebuilt player scene.
The test creates/opens a CharacterBody2D scene, adds sprite/collision nodes,
checks real editor Undo/Redo, creates/assigns resources, writes/validates/attaches
movement code, saves the scene and sets it as main. It waits for import/save
completion and checks persisted output before emitting GO_AGENT_SCENE_OK.
Both debug and release exports then run independently of the source project and
must emit GO_PLAYER_SMOKE_OK without editor classes.

The deterministic test hook is compiled only with GO_SUITE_TESTS_ENABLED and
requires --go-agent-acceptance, the exact fixture project name and a marker file.
Existing player/shape/texture files abort the test; no live gateway request is
made. Test approval deliberately invokes the prepared bridge API: it is NOT a
human-clicked dialog test and NOT proof of live AI tool planning.

On Linux, a subsequent Xvfb run opens the actual editor/player scene and captures
editor-ui.png plus its log. Generated project files and logs are retained in the
GO-validation artifact. Capture requires a visible X11 window but is NOT manual
visual review. Windows graphical review and live-AI acceptance remain open.
All these new acceptance gates are implemented but await their first full CI
execution; mocked orchestration tests are not a substitute for that result.

### Remaining project controls and asynchronous agent Git status

36 tools are now enabled. `scan_filesystem`, `select_node`, `auto_commit` and
`get_scene_tree` use mandatory editor-action approval. Selection is bounded to
the open scene and rechecks after clearing the previous selection. Scene-tree
inspection loads a saved .tscn resource without instantiating it, returns at most
1000 declarations/48000 bytes, and does not expand inherited/instanced subscenes.
Import/resource loading may execute project callbacks and is explicitly not a
sandbox. Agent Automatic Commit uses the same registered controller and lock as
the dock and close reminder, with an explicit gateway/privacy/stage/push warning.

`set_project_setting` accepts only name=application/run/main_scene and a scene
string value; it delegates to the backed-up full settings-preview flow. Other
setting writes are refused. Preview serialization now restores Godot's implicit
feature-list normalization until approval, and applies that list only after the
configuration replacement succeeds.

`git_status` runs nonblocking with a 30-second timeout and owns its reservation.
Stop/exit cancels only its own job. The response contains a local changed-path
count, not filenames/content, remote-fetch state or LFS-transfer claims. A native
dock test covers actual local Git, completion, privacy and cancellation ownership.
New editor-action denial tests and setting allowlist checks are added. Runtime
acceptance remains pending. The UI capture script executable-bit style failure
from dc8e0d90 is corrected in this checkpoint.

### Explicitly reviewed editor diagnostics

`get_editor_log` completes the current 37-tool whitelist. A small registered
upstream C++ adapter exposes the entire retained editor-output buffer only when
it fits 48000 UTF-8 bytes. Larger buffers are refused, not partially tailed. The
bridge screens the snapshot for possible credentials and then requires explicit
approval showing exactly that text before sharing it with the selected model.
The snapshot hash is rechecked at execution. Automatic screening is imperfect;
private paths/data may remain, so review is mandatory. Evicted history, separate
debugger state and repetition counts are not reconstructed.

The actual-editor acceptance fixture now exercises the log tool, then verifies
whole-buffer size rejection and a known secret-marker rejection. The upstream
adapter is documented in UPSTREAM_PATCHES.md and included in real-header syntax
checks for production/test layouts. Runtime and visual confirmation are pending.

### Remaining release blockers found during completion audit

The current 37-tool checkpoint is not final project acceptance. In particular:

- The Bundled Plugins button still has a disabled placeholder handler. Contrary
  to the earlier PLUGINS.md description, existing-project offline management is
  not implemented; that description is corrected.
- Seven GDScript addons are vendored. Dialogic remains absent and four pinned
  GDExtension entries still lack release binaries. No tested default plugin set
  has been enabled. Manifest/license validation is not startup acceptance.
- Latest full Windows/Linux editor/template builds are still in progress. The
  new actual-editor tool chain, log rejection and graphical screenshot checks
  have not returned runtime results yet.
- Live free-gateway planning, user-clicked approval flow, Automatic Commit UI,
  lifecycle reminders and plugin startup still need end-to-end/visual acceptance.

Local checkpoint checks: production/test syntax for 31 module/adapter C++ files
and eight test headers, standalone guardrails, upstream patch registry, UI
source guardrails, five mocked export-harness cases, plugin manifest and style
checks. None substitutes for the outstanding runtime/release checks above.

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

### Native scene test runtime correction

The full build for 821c9efd compiled both platforms but failed script-attachment
Undo/Redo testing: native doctest startup had not initialized GDScript language
globals ("Native class Node not found"). The fixture now scopes language
init/finish exactly as the upstream GDScript runner does. Production attachment
code is unchanged; rerun confirmation is pending. The extension archive job's
first run also failed; structured error annotations are added for diagnosis in
environments that cannot retrieve Actions log-CDN files.

### Native-only addon installation

The offline manager now also recognizes root `.gdextension` descriptors without
`plugin.cfg`, allowing verified release addons such as Godot Git Plugin to be
copied. Consent explicitly warns that import loads native machine code. Native
extensions without an EditorPlugin switch cannot be disabled/unloaded from this
dialog; no unsafe file deletion or unload is attempted. For addons with both,
disabling the editor plugin does not unload the native library. Native startup
acceptance and the actual UI interaction remain pending.

### Archive CI result and plugin startup gate

The corrected pinned-extension workflow for 76dd5a12 has passed Linux and Windows
archive verification and artifact upload (run 46042242506). The Git addon uses
upstream's `addons/godot-git-plugin` directory, not the earlier underscored guess.
This proves checksum/layout/library presence, not ABI or editor compatibility.

The full build now starts every bundled addon separately in a disposable project
using the compiled GO. editor, first importing, then enabling its EditorPlugin
where present. Native-only extensions load during import. Errors, timeout and
nonzero exits block release packaging; per-plugin logs are preserved. This new
startup gate has not completed yet. Defaults remain disabled until startup and
visual/interaction checks pass. An editor starting successfully does not by
itself prove every plugin feature or exported-project behavior.
