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
