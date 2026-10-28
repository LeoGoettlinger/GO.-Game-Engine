# GO. base versions and external references

## Godot base

- **Selected base:** `4.7.2-stable`
- **Tag object / commit:** `ed1daf0bf001b61586d9930840f2f1394092c079`
- **Release date:** 2026-08-18 (GitHub API, checked 2026-10-26)
- **Upstream:** <https://github.com/godotengine/godot.git>
- **Verification:** `git ls-remote --tags upstream 'refs/tags/4.*-stable'` ended at `4.7.2-stable`; no newer stable 4.x tag existed at the time of the check.

The Arena checkout initially contained a shallow, grafted 4.8.0-beta development snapshot. In
commit `dd873350` its working tree was reset to the `4.7.2-stable` tag without leaving the
`arena/a15e5657-go-game-engine` branch. `PROMPT.md` was preserved. This gives GO. a stable,
rebaseable base instead of a moving development branch.

Recreate the upstream remote in a fresh clone:

```sh
git remote add upstream https://github.com/godotengine/godot.git
git fetch upstream tag 4.7.2-stable
```

The engine version number remains `4.7.2` so export-template version checks keep working.
GO.'s own release label is `4.7.2.go.0.1.0`; it is shown in the About dialog, README and
release artifact names.

## Kilo Code reference

- **Repository:** <https://github.com/Kilo-Org/kilocode.git>
- **Pinned audited commit:** `54bff5dcbd0ae8c11adad618f32c9255583699d0` (2026-10-26)
- **Clone:** full, unfiltered Git clone and complete working tree, checked out detached
  at the pinned commit in `/home/user/kilocode-reference` (2026-10-28).
  `git rev-parse --is-shallow-repository` returns `false`; neither sparse checkout
  nor partial-clone filtering is configured. Earlier work used a sparse reference;
  this checkout replaces it. No Node dependencies were installed.
- **Reproduction:** `git clone https://github.com/Kilo-Org/kilocode.git ../kilocode-reference`,
  then `git -C ../kilocode-reference checkout --detach 54bff5dcbd0ae8c11adad618f32c9255583699d0`.
  Run `python misc/scripts/go_verify_kilo_reference.py ../kilocode-reference`.
  Source hashes are recorded in `kilo-reference.json`; see `KILO_PORT.md` for the port map.
- **Audited files:** `packages/kilo-gateway/src/api/constants.ts`, `api/url.ts`, `api/models.ts`,
  `headers.ts`, `loader.ts`, `provider.ts`, and the root `LICENSE`.
- **Verified facts:** gateway base `https://ai-gateway.kilo.ai/api/v1`, anonymous key
  `anonymous`, `DEFAULT_FREE_MODEL = DEFAULT_MODEL = kilo-auto/free`, `X-KILOCODE-EDITORNAME`,
  OpenRouter-compatible `data: [...]` model responses with `isFree`, `pricing` and
  `supported_parameters`.
- **License:** MIT; `Copyright (c) 2026 Kilo Code` and `Copyright (c) 2025 opencode`.
  See [`licenses/KILO-MIT.txt`](licenses/KILO-MIT.txt).

Only free, anonymous gateway behaviour is ported. Authentication, billing, custom providers,
MCP, multi-root workspaces and paid-model UI are deliberately excluded.

## Font reference

- **Repository:** <https://github.com/IdreesInc/Monocraft.git>
- **Pinned clone commit:** `e498bf70aeb25b4bdcff1e44d878fb2cb4f7c2a9`
- **Asset:** `dist/Monocraft-ttf/Monocraft.ttf` → `go_assets/fonts/Monocraft.ttf`
- **License:** SIL Open Font License 1.1 → `go_assets/fonts/LICENSE-Monocraft.txt`

No original Mojang/Microsoft font or Minecraft asset is distributed. "Minecraft" is used only
descriptively for the pixel-font style.
