# Vendored plugin patches

GO. vendors MIT GDScript addons verbatim from the exact commits pinned in
[`plugins.json`](plugins.json). When an upstream source does not parse under the pinned Godot
4.7.2-stable binary, the smallest possible compatibility edit is recorded here **and** in the
plugin's `.origin.json` `patches` array. Re-running `misc/scripts/go_vendor_plugins.py` restores
the pristine upstream tree; reapply the patches below afterwards.

## ProtonScatter `4.0` (`ca624412c657d469147b4ce464cee290df738a40`)

- `addons/proton_scatter/src/scatter.gd` — `_get()` lacked an explicit return for unknown
  properties. Godot 4.7.2-stable reports “Not all code paths return a value.” Added
  `return null` at the end of the function. No plugin behavior changes for the two handled
  properties.

This is the only source patch among the seven vendored GDScript plugins as of
`4.7.2.go.0.1.0`. All other vendored files remain byte-identical to their pinned tarballs.

## Dialogic 2 alpha-20

- `addons/dialogic/Editor/Common/update_manager.gd`: update check and download
  methods emit failure/unavailable without HTTP requests. This prevents automatic
  network access and mutable self-updates of the pinned offline bundle. Upstream
  licenses are unchanged. Reapply this patch after re-vendoring.

- `addons/dialogic/Modules/Clear/event_clear.gd`: use the event's supplied
  `dialogic` handler instance for the last-event check, consistent with every
  other operation in `_execute`, rather than referencing the autoload global.
  Official 4.7.2 `--check-only --script` does not initialize autoload globals;
  this was the sole parse/compile failure remaining after adding Dialogic. The
  instance-based check also respects non-global handlers. Reapply after vendoring.

## Offline updater policy (Dialogue Manager, Phantom Camera, GUT)

- Dialogue Manager `components/update_button.gd` and
  `components/download_update_panel.gd`: disable automatic checks and mutable
  download/install requests. The download button explains release-based updates.
- Phantom Camera `scripts/panel/updater/update_button.gd` and
  `scripts/panel/updater/download_update_panel.gd`: same policy.
- GUT `update_detector.gd`: `fetch_remote_file` emits completion deferred and
  returns unavailable without HTTP, preserving awaiters and shipped/cached
  version metadata. No error is pushed just because the editor is offline.

Together with the Dialogic patch, these remove all current `.request(...)`
HTTP call sites in the eight vendored GDScript addons. Addons remain unsandboxed;
this static policy check is not a guarantee against arbitrary third-party code.
Reapply these patches after re-vendoring pinned upstream sources.
