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
