# Kilo Code → Code Agentics: source and port map

Audit checkpoint: 2026-10-28. Original repository: https://github.com/Kilo-Org/kilocode
Pinned commit: `54bff5dcbd0ae8c11adad618f32c9255583699d0`.

The full original repository has been cloned outside the Godot tree, without
shallow history, sparse checkout or blob filtering. Source hashes and a local
verification command are in `kilo-reference.json` and `BASE_VERSION.md`.
MIT notices are preserved in `licenses/KILO-MIT.txt` (Kilo and opencode).

## What integration means here

PROMPT §3.1 requires a native port of the original behavior, not an embedded
VS Code extension or a renamed HTTP demo. The original TypeScript runtime depends
on VS Code, its CLI backend and Effect services; those cannot be loaded as Godot
C++ editor plugins. The clone is the source reference, not a shipped Node runtime.
The existing implementation is **partial**, not a completed Kilo integration.

| Original pinned source | Godot destination | Current state / intentional difference |
| --- | --- | --- |
| `packages/kilo-gateway/src/api/constants.ts` | `go_kilo_models.*`, `go_kilo_client.*` | Gateway and `DEFAULT_FREE_MODEL` reproduced; environment URL overrides deliberately excluded. |
| `packages/kilo-gateway/src/api/url.ts` | `go_kilo_client.*` | Fixed anonymous gateway routes; organization/token routing excluded. |
| `packages/kilo-gateway/src/api/models.ts` | `go_kilo_models.*` | Catalog fields and free evidence parsed; GO. further restricts selection to free models. No account/BYOK data imported. |
| `packages/kilo-gateway/src/loader.ts` | `go_kilo_client.*` | Original anonymous fallback becomes GO.'s only authentication mode. |
| `packages/kilo-gateway/src/headers.ts`, `provider.ts` | `go_kilo_client.*` | Editor identity and HTTP transport adapted; no tracking/session identity forwarded. |
| `packages/core/src/session/execution.ts` | `go_agentics_dock.*` | Original session resume/interrupt ownership is a reference; GO. currently has chat cancellation only, not the complete runner. |
| `packages/core/src/permission.ts` | `go_agentics_tool_bridge.*` | Original deny/ask/reply permission concepts inspected. Per-action approvals and durable ownership are not yet ported; execution remains disabled. |
| `packages/kilo-vscode/src/KiloProvider.ts` | `go_agentics_dock.*` | Native dock replaces VS Code/webview/CLI transport. Original telemetry, account and workspace integrations excluded. History and complete tool-result rendering remain open. |

These are behavioral mappings, not claims of verbatim TypeScript inclusion.
No executable shell, arbitrary filesystem workspace, paid provider or MCP path
may be imported while porting the original runner. Native scene edits additionally
need Godot Undo/Redo and protection for unsaved user changes.

## Required next implementation work

1. Represent assistant tool calls and matching `tool_call_id` results, not merely
   role/content pairs; add multi-round protocol fixtures before enabling tools.
2. Port session ownership/cancellation and permission request/reply semantics into
   native asynchronous state transitions, capped at 20 steps.
3. Add strict tool schemas, per-action approval, path confinement and Undo/Redo.
4. Verify real editor interaction, not only pure parser/mock tests.

The upstream agent runner and permission system are **not yet fully ported**.
Automatic Commit is also still disabled pending its separate safety prerequisites.
