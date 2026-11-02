# GO. Git Suite

Status: implementation in progress; see [`ROADMAP.md`](ROADMAP.md) for exact completion.

The suite drives the system `git` with `-C <project>` only. It never stores passwords or
tokens; authentication uses the OS credential helper or SSH agent. There is no force-push,
hard-reset or amend of published commits.

## Host parsing

`GoGitRemote` normalises HTTPS, `ssh://`, SCP-style SSH and local remotes to host, owner, repo,
protocol and web URL. Known host chips: GitHub, Codeberg (Forgejo), GitLab, Custom, Local.
Codeberg LFS is treated as a first-class host; no GitHub-only code path guards LFS.

Remote assistant URL previews:

| Host | HTTPS | SSH |
| --- | --- | --- |
| Codeberg | `https://codeberg.org/<owner>/<repo>.git` | `git@codeberg.org:<owner>/<repo>.git` |
| GitHub | `https://github.com/<owner>/<repo>.git` | `git@github.com:<owner>/<repo>.git` |
| GitLab | `https://gitlab.com/<owner>/<repo>.git` | `git@gitlab.com:<owner>/<repo>.git` |

New repositories are created in the host's browser at `/repo/create`, `/new` or `/projects/new`.
No token-based repository creation and no credential entry exist in GO.

## Git LFS

`GoGitLfsManager` separates *installed*, *locally initialised* and *ready* states. Installation
always requires an explicit user action; Linux uses `pkexec` for the package-manager command and
Windows prefers `winget` with the official installer as fallback. Failure leaves the project
usable and shows a retry banner.

The managed `.gitattributes` block is:

```gitattributes
# GO. Git LFS (automatisch verwaltet)
*.png filter=lfs diff=lfs merge=lfs -text
...
```

Default patterns cover images, DCC files, 3D buffers, Godot binary resources, audio, video,
fonts and `.zip`. Text formats (`.tscn`, `.tres`, `.gltf`, `.gd`, `.import`, `.uid`, `.svg`,
`.json`, `.cfg`, `.godot`) stay normal Git files. The block is written **before** the first
stage. User `.gitattributes` lines are preserved.

Thresholds: warn above `large_file_warning_mib` (default 5 MiB), red warning above 50 MiB,
GitHub hard file limit 100 MiB without LFS. Codeberg storage/LFS guidance is a warning, never a
hard block. History migration is never automatic.

## Automatic Commit

The deterministic sequence is: preflight → UI lock → status/diff → `kilo-auto/free` message →
LFS-aware staging → commit → push (if remote) → unlock/result. Network failure falls back to a
conventional-commit message. Every terminal state unlocks the UI; this invariant is unit-tested.

## Step 5 implementation status

The following pieces are implemented as source and covered by automated checks:

- `go_system_git.*` builds every Git/LFS command as an argv vector with `git -C <project>`.
  No shell is used, remote URLs with embedded credentials are rejected, and commit messages stay
  ordinary argv values. Output parsers cover porcelain status (including branch, ahead/behind and
  conflicts), log records, `git lfs ls-files --size`, `git lfs status`, config values and LFS
  pointer prefixes.
- `go_system_git_runner.*` is the only place that calls `OS::execute`; it converts the argv vector
  to Godot's `List<String>` and captures stdout/stderr plus the exit code.
- `go_git_controller.*` provides `snapshot()`, `set_remote()`, `fetch()`, `pull()`, `push()`,
  `stage_all()`, `stage_paths()`, `commit()`, `diff_stat()`, `diff_text()` and the LFS helpers.
  `stage_all()` writes the managed `.gitattributes` block before staging and verifies that staged
  LFS-covered files contain pointer prefixes; otherwise it re-adds those paths.
- `GoRemoteAssistant` offers GitHub / Codeberg / GitLab / Custom host, HTTPS/SSH, URL preview,
  a browser create-page button and `git remote set-url`/`add`. It never asks for credentials.
- `GoLfsDialog` exposes check, system install (explicit `pkexec`/`winget` command), local init,
  managed rules, suggested-pattern tracking, download/upload and quota output.
- `GoGitDock` now shows real status, branch, ahead/behind, warnings and recent log; commit,
  pull, push, remote assistant and LFS buttons are wired to the controller.

Automated evidence:

- `python3 misc/scripts/go_run_guardrails.py` — pure command builders/parsers, remote safety,
  status/log/LFS parsing, pointer detection and large-file patterns.
- `python3 misc/scripts/go_git_flow_test.py` — real system `git` against a local bare remote:
  init → commit → push → clone → clean status, plus managed `.gitattributes`, large-file
  classification and a synthetic LFS pointer in the index.
- `python3 misc/scripts/go_syntax_check.py` — all 23 Godot-facing module `.cpp` files parse
  against real Godot 4.7.2 headers (no full engine link).
- `python3 misc/scripts/go_check_ui_guardrails.py` — no forbidden settings UI in the new dialogs.

Known limits: the compiled editor has not been started yet, so no UI screenshot exists. The
local sandbox has no `git lfs` binary, while GitHub runners do; the bare-repo test therefore
deliberately keeps the synthetic pointer on a non-LFS extension and never activates a real LFS
filter. Real `git lfs install/pull/push` and the Windows/Linux installer commands are
command-built and syntax-checked, but must still be smoke-tested on a developer machine or in
the tagged CI build. Automatic Commit (§8) and project lifecycle reminders (§7) remain later
steps.

## LFS selection modes (2026-10-28)

In **GO. Git LFS**, select a mode, then **Save LFS settings**:

- **Automatic: files at or above size threshold**: choose 1–1048576 MiB
  (default 5). On **Write LFS rules** and GO. staging, scan Git's tracked and
  unignored files; eligible files at or above the threshold receive exact-path
  rules, not extension-wide rules. This is not an idle/background watcher.
- **Semi-automatic: selected folders recursively**: use **Add folder…** repeatedly
  for multiple project subfolders; remove selected entries as needed. Recursive
  `.gitattributes` patterns include future files, even when staging via external
  Git. Files outside those folders receive no GO.-managed LFS rule in this mode.
- **File types (legacy default)** retains existing behavior until explicitly changed.

Selection and threshold persist locally in `.godot/go_lfs.cfg` (not shared with
collaborators). Generated `.gitattributes` rules are shared through Git. A fresh
clone does not inherit the local mode; select it before using GO.'s rule writer.
**Write LFS rules** applies the saved policy; changing controls alone does not.
GO.'s stage-all refreshes rules, stages changes and renormalizes tracked files, so
existing blobs can become pointers in the next commit without rewriting history.
This may also apply existing text normalization rules; review the staged diff.
For external Git, run `git add --renormalize .` after changing folder rules.

Godot/text formats (case-insensitive), PCK/EXE/APK/AAB exports and nested generated
`.git`, `.godot`, `export`, `exports` directories are excluded from folder LFS
rules. Ignored exports remain ignored. Root, hidden, traversal, wildcard and
symlink folder selections are rejected. Folder names with spaces are supported.
Size scanning conservatively skips unsupported paths and rejects Git-quoted
control-character filenames instead of guessing. No installation or migration
of historical commits occurs when saving settings.

Only GO.'s marked block is replaced. Foreign rules and nested `.gitattributes`
can still affect effective attributes; pointer verification consults Git's actual
cached filter attribute, not a hardcoded extension list. Changing modes does not
remove foreign rules. Removing a folder is not a history migration.

Validation: pure C++ policy/boundary tests; generated rules checked with real Git
for spaces, multiple folders, recursion, text/export exceptions and idempotent
mode changes. CI real-LFS transport additionally uses these generated folder rules
for a binary round-trip. New UI still requires a visual acceptance test; Git calls
remain synchronous and Automatic Commit remains disabled.

## Nonblocking dock commands (2026-10-30)

Fetch, Pull, Push and Commit in the Git dock now use `GoGitProcess` with Godot's
nonblocking `execute_with_pipe` API. Commands use argv, never a shell. Branch and
upstream resolution are asynchronous phases too. During a job the dock rejects
another operation and disables its Git actions. **Cancel Git** terminates the
owned Git process; leaving the editor tree also closes it. The command timeout is
120 seconds, retained stdout/stderr are capped at 1 MiB each, and per-frame pipe
draining is bounded. Use **Refresh** after a job to inspect the resulting state.

Cancellation is not rollback: remote updates may already have happened. Godot's
kill API terminates the owned process, not a guaranteed whole descendant process
tree; external credential helpers, hooks and SSH children can outlive that process.
Inspect state before retrying. No global environment or credentials are changed.
Local status refresh, the LFS dialog, project setup and the disabled Automatic
Commit controller still use synchronous paths. This is not yet a complete async
Git conversion or a whole-editor transaction lock.

Native tests in `tests/test_go_git_process.h` cover stdout, nonzero exit, concurrent
start rejection, cancellation, timeout, reuse and invalid commands. CI editor
builds now enable `tests=yes` and run `[GO.][GitProcess]*` on Windows and Linux.
These tests use system Git only, with no remote, shell command or project writes.
Runtime results for this change remain pending; local syntax tests include both
native test headers as well as all module translation units.

## Project lifecycle reminders (implementation checkpoint)

For enabled projects, a new lifecycle node waits until the editor is ready before
an asynchronous origin lookup, fetch (when present), and local status read. A clean,
up-to-date repository produces no popup. Startup Git errors remain quiet; fetch
failure is logged and local status is still checked. Behind/ahead and dirty states
produce one reminder with **Open Git Suite**, **Later**, and **Pull** when behind.
Pull delegates to the existing nonblocking, fast-forward-only dock action.

Existing Git projects without the suite receive one activation offer per local
project metadata cache. Accepting enables the suite; no init/commit occurs.
Clearing `.godot` can make this local offer reappear. The project-level
`go/git_suite/remind_on_open` and `remind_on_close` settings can be disabled with
the reminder checkbox. Failed settings saves are reported and rolled back in memory.

The `EditorNode::_discard_changes` hook runs after Godot's own save/discard decisions
for Quit, Project Manager and Reload. A modal, cancellable progress dialog checks
local status and, best-effort, LFS pending objects without fetching. Clean results
continue the exact upstream quit path once. Otherwise the user may open the Git
Suite, cancel closing, or explicitly **Close Anyway**. Missing/failed status or an
already running Git operation also permits this explicit choice; it never waits
indefinitely. Each lifecycle subprocess has a 30-second timeout.

Known limitations: counts use the last known upstream; missing upstream/LFS state
is not evidence that data is backed up. The Automatic Commit action is not yet
provided here because its controller remains disabled. The dialog can open the
Git Suite instead. Closing terminates the owned Git process but does not guarantee
termination of external helpers or undo any remote changes. Local refresh/LFS
operations remain synchronous outside these reminder paths.

The suite's registry excludes concurrent owned Git jobs. While a close reminder is
pending, new dock Git operations and agent sends are rejected; pending scene tools
stop, late model responses do not execute actions. The modal blocks manual editing
while the prior save/discard decision is pending. Arbitrary third-party editor
plugins are not paused by this mechanism. Headless imports and tests skip reminders.

Local pure tests cover clean/dirty/ahead/behind/LFS/failed-status reminder decisions.
Source checks protect the upstream continuation hook. Visual reminder, save/discard,
reload, Cancel and concurrent-operation acceptance still needs a real editor run.

## Automatic Commit controller hardening (not enabled yet)

The controller's working-tree/index diff reads, `git add`, renormalization, commit,
branch lookup and push now use the nonblocking process runner. Both staged and
unstaged changes contribute to the prompt. Timeout/nonzero exit/start failure
terminate the transaction; a failed push explicitly reports that the commit remains
local. No force-push, history rewrite, automatic pull or retry is introduced.
Preflight status/identity/origin/LFS queries, repository-local LFS installation,
file discovery, cached attribute queries and staged pointer reads now also use
asynchronous Git processes. Size-policy filesystem metadata is processed in batches
of 16 files per frame; local configuration/attribute file I/O remains synchronous.
The button and agent callback remain disabled pending native runtime and visual
whole-editor-lock acceptance.

The overlay is now a separate Control: only that Control moves to the editor root.
Previously moving the running controller itself emitted EXIT_TREE, interrupting
its own operation. Input/job locks are released only by their owner, preserve the
previous root input state, and are released on success, failure and teardown.
The detached overlay is cleaned up by object ID. Scene tools are stopped while
this lock is held. Native `[GO.][AutoOverlay][Editor]` tests have been added for
relocation, simulated failure/success and destruction. They check the synthetic
root-window input state, not all native subwindows, shortcuts or full transactions.
Runtime CI evidence and visible whole-editor-lock acceptance remain pending.

### Commit-message privacy

Diff commands disable external diff programs and textconv, use explicit `a/` and
`b/` prefixes and suppress submodule content. Prompt filtering evaluates both old
and new paths, including renames. Sensitive names (including `key`, `token`, `.env`)
are represented without contents. Quoted/escaped/ambiguous path syntax is omitted
rather than guessed. Binary and lockfile payloads are omitted. A detected sensitive
value marker drops the whole file block, including multiline continuations.
Context lines cannot masquerade as a new diff header. The 12,000-byte diff cap
includes redaction markers; status and stat sections have separate 2,000/3,000-byte
caps. This is conservative heuristic filtering, not a guarantee of detecting an
arbitrary secret hidden in an innocuous file. Review sensitive projects before
sending any content to a model. Local Git staging is not a secret-file exclusion.

Pure regressions and local real-Git fixtures cover sensitive/renamed/quoted paths,
context-header spoofing, multiline key material, lock/binary payloads and hard caps.
No live gateway request was used to test this change.

### Asynchronous preparation and offline transaction coverage

Status and file lists use NUL-delimited raw process bytes, preserving spaces,
newlines and renamed paths without C-string truncation. Truncated subprocess
output is a hard error rather than evidence that only part of an index was checked.
A job reservation covers preflight before the input lock is acquired; status is
rechecked after acquiring the lock. External processes can still change a repository;
this is not a filesystem transaction or rollback mechanism.

LFS rules are prepared before add/renormalize. Verification reads **cached**
attributes and each staged blob, not its working-tree counterpart. LFS blobs must
be at most 1024 bytes and contain a complete canonical SHA-256 pointer with a
valid decimal size. Unsupported pointer extensions fail closed. File lists above
10,000 entries, oversized attributes (1 MiB), malformed output and unsupported
size-policy filenames abort without committing. Failure may leave files staged
or managed attributes changed; no automatic reset discards user work.

`[GO.][AutoFlow][Editor]` tests use real local Git processes and a test-build-only
message fallback: no AI gateway requests. They cover no-change preflight, local
success, bare-remote success, rejected push with local commit retained, invalid
staged LFS content, real git-lfs conversion and cancellation during preflight.
They are wired into Windows/Linux full CI; adding these tests is not a claim that
the new runtime suite has passed. Visible native-subwindow/shortcut locking and
Automatic Commit on close remain outstanding.

### Commit-before-close continuation

The lifecycle controller now has an Automatic Commit action, gated by the same
availability registration as the Git Suite. It reserves the pending quit through
preflight and the whole transaction. Normal commit requests cannot bypass that
reservation; only the explicit close continuation can start the transaction.
Completion is deferred until the controller has unlocked input and released its
Git job. Success resumes the original upstream save/discard continuation once;
failure retains the editor and offers inspection/cancel/explicit close instead.
Controller teardown reports failure to a surviving close controller. An already
running manual Git job is allowed to finish before rechecking close status, rather
than being killed to close the editor; cancelling the close does not cancel that job.

Native `[GO.][GitClose][Editor]` fixtures run the real offline commit pipeline and
substitute only the final quit callback. They cover success after unlock, failed
preflight, cancel, explicit close and stale duplicate completion. These are not a
replacement for unsaved-scene and native-window visual acceptance. The action is
implemented but remains disabled alongside Automatic Commit pending that evidence.
