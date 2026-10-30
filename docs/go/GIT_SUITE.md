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
