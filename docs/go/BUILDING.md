# Building GO. Game Engine

GO. is built from the Godot 4.7.2-stable source tree. The `short_name = "go"` setting changes
the user-data directory and OS-display name. In this Godot base the SCons binary stem
(`godot`) and the platform export-template filenames are still hard-coded; the GO. release
packaging renames the editor for users but deliberately keeps template filenames
Godot-compatible. GO. still needs its **own export templates** because they are built from
the fork; official Godot templates are not guaranteed to match GO.'s configuration.

## Linux (Ubuntu 22.04 / 24.04)

```sh
sudo apt-get update
sudo apt-get install -y build-essential scons pkg-config libx11-dev libxcursor-dev \
  libxinerama-dev libgl1-mesa-dev libglu1-mesa-dev libasound2-dev libpulse-dev \
  libudev-dev libxi-dev libxrandr-dev libwayland-dev libxkbcommon-dev
python3 -m pip install --user scons==4.9.1
scons platform=linuxbsd target=editor arch=x86_64 no_editor_splash=no production=yes -j$(nproc)
scons platform=linuxbsd target=template_release arch=x86_64 production=yes -j$(nproc)
scons platform=linuxbsd target=template_debug arch=x86_64 production=yes -j$(nproc)
```

## Windows (Visual Studio 2022)

From a *x64 Native Tools Command Prompt for VS 2022*:

```bat
py -m pip install scons==4.9.1
scons platform=windows target=editor arch=x86_64 no_editor_splash=no production=yes
scons platform=windows target=template_release arch=x86_64 production=yes
scons platform=windows target=template_debug arch=x86_64 production=yes
```

C#/.NET is disabled in the GO. default build; do not pass `module_mono_enabled=yes`.
On a clean GitHub-hosted Windows runner, add `accesskit=no d3d12=no` to each SCons command
unless you have installed those SDK dependencies first; the GO. workflow does this via
`matrix.extra_flags`. A developer machine with the SDKs installed can keep both drivers.

## Editor splash

`main/splash_editor.png` is intentionally present in GO. `SConstruct` normally forces
`no_editor_splash=yes` when that file is missing. Build with `no_editor_splash=no`:

- If `NO_EDITOR_SPLASH` is defined or the splash is missing, check that
  `main/splash_editor.png` exists and pass `no_editor_splash=no`.
- Regenerate both splash images with `python3 go_assets/make_splash.py` (Pillow required).
- The generated `main/splash.png` is 800×600; the editor splash is 1152×648 and is displayed
  unscaled/centered.

## Local verification without a full engine build

```sh
python3 go_assets/make_splash.py --check
python3 misc/scripts/go_branding_check.py
python3 misc/scripts/go_run_guardrails.py
python3 misc/scripts/go_check_ui_guardrails.py
python3 misc/scripts/go_validate_plugins.py
python3 misc/scripts/go_mock_gateway_test.py
python3 -m unittest discover -s misc/scripts -p 'go_test_*.py'
```

`go_assets/make_splash.py` needs Pillow. The other checks use only the Python standard
library and a C++17 compiler.

A full editor/template build is performed by `.github/workflows/go-build.yml` on Windows and
Linux. `.github/workflows/go-quick-checks.yml` runs on every PR without compiling C++.

## Upstream sync

```sh
git remote add upstream https://github.com/godotengine/godot.git  # once
git fetch upstream --tags
git rebase upstream/<new-stable-tag>   # or git merge, on the GO. working branch
# Resolve conflicts using docs/go/UPSTREAM_PATCHES.md, then rebuild and rerun guardrails.
```

### Native process tests

GO. CI editor builds use `tests=yes` (templates remain `tests=no`) to exercise
nonblocking process behavior on both host platforms. Run the compiled editor with
`--headless --test --test-case="[GO.][GitProcess]*"` and system Git on PATH.
The tests wait on Git's stdin to test cancellation/timeout without shell scripts
or a remote. Shipping CI editors currently include test support; use `tests=no`
for a local editor without this test harness. Native test results do not replace
manual interaction, approval, Undo/Redo or cancellation-of-descendants acceptance.

The scene-only agent adds a separate native test invocation:
`--headless --test --test-case="[GO.][SceneUndo]*"`. It tests the shared operation
recording methods against an isolated core UndoRedo history, without a live editor
scene or a gateway request. EditorUndoRedoManager itself requires a live editor,
log and debugger and is not instantiated in that standalone test. Run this subset in its own process. Modal approval,
target changes during approval and visible editor Undo remain UI acceptance cases.

The Automatic Commit overlay adds
`--headless --test --test-case="[GO.][AutoOverlay]*"`. Its `[Editor]` fixture creates
a mock scene tree/settings environment, not a full EditorNode. The test checks
controller lifetime, owned root input state and teardown without Git/network calls.
It does not prove native-subwindow input blocking or transaction correctness.

### Automatic Commit offline native transaction tests

With an editor built using `tests=yes`, run
`--headless --test --test-case="[GO.][AutoFlow]*"`. Git and git-lfs must be on PATH.
The fixtures create temporary local repositories/remotes, run real asynchronous
controller phases and force the offline message fallback only in test builds.
No gateway credentials or network repository are required. This suite does not
replace visible editor/native-window input-lock or export/player acceptance.

### Offline export and player validation

After both custom export templates have been built, full CI now imports an isolated
2D player fixture, runs it with the compiled editor, exports it with **both** custom
debug/release templates, and runs each exported executable from a separate directory
without the source project. The PCK is embedded. A success marker is required;
exit code zero alone is insufficient, and engine error output fails the step.
The template run also checks that editor-only `EditorPlugin` is unavailable.

Example (Linux, from the checkout):

```sh
python misc/scripts/go_export_smoke.py --platform linux \
  --editor bin/godot.linuxbsd.editor.x86_64 \
  --debug-template bin/godot.linuxbsd.template_debug.x86_64 \
  --release-template bin/godot.linuxbsd.template_release.x86_64 \
  --output-dir go-validation/export
```

For Windows use `--platform windows` and the corresponding `godot.windows.*.exe`
files. Each subprocess has a 180-second timeout and isolated user/config/cache
folders. No AI call or network repository is involved. Native test logs, export
logs and `result.json` are uploaded even on failure in the platform validation
artifact. Packaging is blocked if export/player validation fails.

The fixture is authored by the test, **not** by Code Agentics. This validates the
engine/template layer only; it does not substitute for the required live-agent
player workflow, screenshots, project-manager creation or packaged-release testing.
The orchestration tests use mocks and must not be reported as real exports.
