# Upstream patches

All intentional changes to files inherited from `godotengine/godot`. This list must be updated
with every new upstream-file patch so a later rebase/merge can be audited.

| File | GO. change | Merge hint |
| --- | --- | --- |
| `version.py` | `name = "GO. Game Engine"`, `short_name = "go"`, GO. repository website; version stays `4.7.2`. | Keep the Godot version fields in sync with the selected stable tag; keep branding fields. |
| `main/splash.png` | Replace game boot splash (800×600) with generated GO. art. | Generated asset; regenerate with `go_assets/make_splash.py`, do not hand-edit. |
| `main/splash_editor.png` | **Added** editor splash (1152×648). | New file, absent upstream; build with `no_editor_splash=no`. |
| `main/app_icon.png` | Generated GO. app icon (256×256). | Regenerate with `go_assets/make_splash.py`. |
| `main/main.cpp` | `--version` prints `GODOT_VERSION_NAME` in addition to the numeric build string. | Keep the GO. line; upstream currently prints only `get_full_version_string()`. |
| `methods.py` | `is_engine()` recognises both Godot's and GO.'s `short_name` for custom module discovery. | Keep both checks when upstream changes engine detection. |
| `editor/gui/editor_about.cpp` | GO. logo, GO. release label and exact fork notice; all Godot credits preserved. | Reapply the three small GO. blocks; do not remove upstream credit generation. |
| `editor/project_manager/project_manager.cpp` | GO. title-bar logo and About tooltip. | Two one-line GO. patches around `title_bar_logo`. |
| `editor/icons/GoLogo.svg` | **Added** original GO. mark used by About/Project Manager. | New file; no upstream counterpart. |
| `README.md` / `README.godot-upstream.md` | New German GO. README; upstream README preserved under its own name. | Preserve both files during merges. |
| `editor/project_manager/project_dialog.h/.cpp` | **Planned:** GO. Git Suite and Git LFS checkboxes; call `GoProjectSetup` after upstream VCS metadata creation. Not patched yet. | Reapply the small `// GO.` blocks after upstream refactors of `ProjectDialog`. |
| `editor/editor_node.cpp` | **Planned:** optional close-reminder hook in existing `SCENE_QUIT` flow. Not patched yet. | Do not duplicate save-confirmation logic; keep hook before final quit. |
| `editor/gui/editor_about.cpp` | Add fork notice and GO. release label while preserving Godot credits. | Keep all upstream credits untouched. |
| `editor/project_manager/project_manager.cpp` | Product title/footer adjustments. | `GODOT_VERSION_NAME` already supplies the product name in most places. |
| `.github/workflows/go-build.yml` | New GO.-specific build/release workflow. | Independent new file; compare with upstream workflow changes at merge time. |
| `.github/workflows/go-quick-checks.yml` | New fast PR workflow with no engine compile. | Independent new file; no upstream conflicts expected. |
| `.github/workflows/runner.yml` | Six GO.-marked branch conditions skip the inherited Android/iOS/macOS/Web/Mono build matrix on the GO. branch; static checks still run. | Preserve the conditions when upstream changes the reusable-workflow caller. |
| `.gitignore` | GO.-marked ignores for local references, `.scons_cache/` and `dist/`; exceptions for vendored plugin `debug/` source folders and the checked-in `go_suite_icons.gen.h`. | Keep the GO. block at the end. |
| `.gitattributes` | GO.-marked whitespace exception for verbatim vendored plugin sources. | Keep the GO. line; upstream EOL rules remain unchanged. |
| `.pre-commit-config.yaml` | Exclude verbatim vendored plugin sources from upstream formatting hooks and exclude the German `README.md`/`Features.md` from the English codespell hook. | Keep both GO. exclude rules when upstream changes the regexes. |

`LICENSE.txt`, `COPYRIGHT.txt`, `AUTHORS.md` and upstream `README.godot-upstream.md` remain
intact. GO.'s own MIT-notice files are additive.
