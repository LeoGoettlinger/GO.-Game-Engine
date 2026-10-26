# GO. architecture

GO. Game Engine is a full fork of Godot 4.7.2. The two product suites live in a new editor-only
engine module, not in a per-project addon.

## `modules/go_suite`

`config.py` returns `env.editor_build`, so export templates contain none of the UI. With
`TOOLS_ENABLED`, `register_types.cpp` registers two built-in `EditorPlugin` classes. Each owns
an `EditorDock` in the right dock area with its own tab button/icon:

- `GoGitDockPlugin` → **Git Suite**
- `GoAgenticsDockPlugin` → **Code Agentics**

A third plugin-like singleton, `GoProjectLifecycle`, owns project open/close reminders and
`Automatic Commit`'s full-editor overlay. It is also editor-only.

### Separation of concerns

| Layer | Responsibility |
| --- | --- |
| `go_git_remote.*` | Pure, unit-testable host/owner/repo/protocol parser and remote URL builder. |
| `go_git_lfs.*` | Pure LFS pattern rules, `.gitattributes` block merging, large-file classification and pointer detection. |
| `go_agentics_path.*` | Pure `res://` path guard, symlink and `.git`/`.godot` checks. |
| `go_kilo_models.*` | Pure free-model filter and minimal gateway JSON parsing. |
| `go_commit_message.*` | Pure commit-message sanitising and secret-aware diff truncation. |
| `go_system_git.*` | One process wrapper; all commands use `-C <project>` and run in a worker. |
| `go_kilo_client.*` | Only the anonymous Kilo Gateway client; no provider/key/login/MCP code. |
| `go_agentics_tools.*` | Whitelisted tools, schemas, validation and Godot-side execution. |
| `go_auto_commit.*` | Stage machine, UI lock invariant and deterministic message generation. |
| `go_project_setup.*` | New-project template/plugin copy, project settings, `git init`, LFS init. |
| `go_bundled_plugins.*` | Offline plugin catalogue and activation. |

Pure logic files have no dependency on Godot classes so that `misc/scripts/go_run_guardrails.py`
can compile and execute them on any CI runner. The same functions are also covered by Godot's
doctest module tests when built with `tests=yes`.

**Current status:** the table is the target architecture. Step 4 adds the two docks, the launcher,
icon generation, the pure logic layers listed above, and the standalone/doctest guardrails. The
process wrapper (`go_system_git.*`), project lifecycle singleton, `go_project_setup.*` and
`go_bundled_plugins.*` runtime glue are still planned; their UI buttons currently report that
the backend is not wired. See [`ROADMAP.md`](ROADMAP.md).

## Bundled data lookup

At runtime GO. looks for `go_bundled_plugins/`, `go_project_template/` and `go_assets/` in this
order:

1. directory next to the editor executable (release layout),
2. editor data directory,
3. source-tree root (development builds).

No network access is used for this lookup. GDScript plugins are copied into
`res://addons/<plugin>/` for new projects. GDExtension binaries are release-bundle artifacts,
never casual commits to the engine source.

## Project settings

- `go/git_suite/enabled` (bool, default in new projects with the checkbox: `true`)
- `go/git_suite/remind_on_open` (bool, default `true`)
- `go/git_suite/remind_on_close` (bool, default `true`)
- `go/git_suite/lfs/enabled` (bool, default `true` with Git Suite)
- `go/git_suite/lfs/large_file_warning_mib` (int, default `5`)

The Git host is derived from `origin` on every refresh; it is not a separate setting. Code
Agentics has no per-project activation and stores conversation history in the editor config
directory, never in the project.

## Upstream patches

Every changed upstream file carries a `// GO.` comment (or an equivalent GO. section in
Python/YAML) and is listed in [`UPSTREAM_PATCHES.md`](UPSTREAM_PATCHES.md). New code lives in
`modules/go_suite`, `go_assets`, `go_bundled_plugins`, `go_project_template`, `docs/go` and
`.github/workflows/go-*.yml`.
