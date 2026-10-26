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
