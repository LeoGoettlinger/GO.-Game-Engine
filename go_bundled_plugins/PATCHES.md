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
