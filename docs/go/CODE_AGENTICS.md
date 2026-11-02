# Code Agentics

Status: implementation in progress; see [`ROADMAP.md`](ROADMAP.md) for exact completion.
Original-source provenance and missing port work: [`KILO_PORT.md`](KILO_PORT.md).
Tool execution is currently disabled; the protocol and permissions below describe
the target, not completed runtime behavior.

Code Agentics is a native Godot port of Kilo's **anonymous free** chat loop. It is not the
TypeScript extension and does not embed Node.js.

## Hard boundaries

- Gateway: `https://ai-gateway.kilo.ai/api/v1`; bearer key is the public literal `anonymous`.
- Model picker: `kilo-auto/free` plus live models proven free by `isFree == true`, an ID ending
  in `:free`, or zero prompt/completion pricing. Paid models are filtered out.
- No account/login, API key, base URL, custom provider, BYOK, billing, MCP, workspace picker,
  folder picker, terminal or shell tool.
- Workspace is the open project's `res://`, shown as a locked non-editable line.
- All paths are validated centrally; `..`, absolute paths, symlink escapes, writes into `.git/`
  and source reads from `.godot/` are rejected.
- Read tools are automatic; write tools require a confirmation dialog unless the dock's explicit
  Auto-Approve switch is enabled.
- No request is sent before the user presses Send.

Privacy notice: free models may log prompts or use them for provider improvement. Never paste
passwords, API keys, tokens or other secrets into Code Agentics.

## Protocol

1. `GET /models` with `Authorization: Bearer anonymous` and
   `X-KILOCODE-EDITORNAME: GO. Game Engine (Code Agentics)`.
2. `POST /chat/completions` with OpenAI-compatible `messages`, non-streaming JSON, timeout and
   optional `tools` when the chosen model lists `tools` in `supported_parameters`.
3. Models without native tool calling must reply with the documented JSON envelope:
   `{"tool":"<name>","arguments":{...}}` or `{"final":"<answer>"}`. Free text is never executed.
4. At most 20 tool rounds per user request; tool results are truncated before being returned.
5. HTTP 429 shows the rate-limit hint and `Retry-After` when present. Offline retains only
   `kilo-auto/free` and displays an offline badge.

## Tool catalogue

The whitelist covers files, scripts, scenes/nodes, resources/project settings, editor playback
and read-only Git status plus the single write path `auto_commit`. Each tool has a strict JSON
schema, `res://` validation and a bounded result. Full parameter/result details are generated
from `GoAgenticsTools::tool_definitions()` and listed in the dock's About dialog.

Known limits: free models can be unreliable at tool calling; the JSON fallback is weaker.
`validate_script` uses Godot's resource loader and reports parse results honestly; editor-log
access is best-effort.

## Native conversation wire validation (2026-10-28)

`GoKiloMessage` retains assistant `tool_calls` and result `tool_call_id` values.
The HTTP client now uses `build_conversation_request_json`, which serializes the
OpenAI function envelope and rejects incomplete rounds, orphan/duplicate results,
reused call IDs, non-object arguments, unknown roles and more than 20 tool rounds
in the submitted history. A batch may contain up to 20 calls and the history up to
256 messages. This bounds serialization; it is not yet the execution-step counter.

Native tool requests additionally require catalog-confirmed tool support and
function definitions. The old role/content-only client entry point rejects `tool`
messages rather than emitting invalid wire messages. The structured entry point
is ready for the future runner; the dock still sends chat-only requests.
Tests cover multiple results, a second round, malformed histories and limits.
These are local serializer tests, not an end-to-end executing agent test.

## Tool argument contract (2026-10-30)

Tool definitions now declare per-tool fields and required arguments with
`additionalProperties: false`. `validate_arguments` rejects unknown tools/fields,
non-object envelopes, wrong value types, missing fields, oversize/NUL strings,
invalid project paths and escaping node paths before the native bridge touches
Godot APIs. The schema and validator share the same argument specification.
Property values currently accept JSON strings, numbers or booleans only, not
arbitrary resource objects. This does not yet constitute a safe property whitelist.

Opening scenes, parsing scripts, filesystem scanning, selecting nodes and running
or stopping playback are classified as side-effecting for future approval gates,
not as automatically safe reads. Auto-Approve is visibly disabled while execution
is disabled; the UI no longer tells users to enable it to bypass that restriction.
Per-action confirmations, Undo/Redo, race-resistant file access and the runnable
session loop remain incomplete. Schema validation alone does not enable tools.

## Experimental scene-only agent (current implementation)

The preceding blanket execution-disabled notes describe earlier checkpoints.
An explicitly opt-in **Scene tools (experimental)** switch now enables only:

- `get_current_scene_tree` (bounded inspection), `get_editor_selection`;
- `add_node` and `rename_node`, recorded in EditorUndoRedoManager history.

Refresh the free-model catalog first and select a model whose catalog entry
advertises native tools. Otherwise the request stays chat-only with an explanation.
Only these four definitions are sent. Returned calls are also checked against this
runtime subset; no file, script, property, playback or Git tool is enabled by it.
The JSON fallback is not executed. History retains native IDs and result envelopes.

Each write shows its tool name and exact arguments. **Deny** records a denied
result; **Stop agent** cancels the remaining batch. The edited scene and target
instance IDs must still match at confirmation time. **Auto-Approve** is opt-in,
defaults off, and resets on a new conversation. A maximum of 20 individual calls
(including failed or denied calls) is allowed per user request; batches exceeding
the remaining budget are rejected before execution. One call is handled per frame.
Cancellation records errors for unfinished calls, preserving valid history. Already
completed edits are not rolled back automatically; use editor Undo.

Node creation permits only Node, Node2D, Node3D, Control, Sprite2D and Label. Writes
reject scripted ancestors, nodes not owned by the edited scene, invalid names and
sibling-name collisions. Addition retains a do-reference; redo restores ownership.
Both add and rename pin the action to the edited scene's history. Scene edits stay
unsaved until the user saves normally. No source files are loaded or overwritten.

Native CI tests exercise the shared operation-recording functions with a real
core UndoRedo history, including add/rename Undo/Redo and owner restoration.
The editor supplies its scene-specific core history through EditorUndoRedoManager. These do not simulate the whole dock or its modal interaction. UI
acceptance and runtime results of the new tests are pending at this checkpoint.
This is a restricted experimental agent, not the full PROMPT tool suite.

## Local display-only chat archive

Messages displayed by the dock are now saved under
`.godot/editor/go_chat_history.json`. This is project-local editor cache, not a
provider setting or an export resource. It is plaintext: prompts, replies and tool
results may contain private project information. The History tooltip discloses the
storage location, lack of encryption and retention bounds.

History opens a separate read-only view; it never supplies archived messages to
Kilo, restores an approval, replays a tool or resumes a previous agent job. A new
editor session starts a fresh conversation. Clear History asks for confirmation
and removes the archive, temporary file and recovery backup. The live conversation
remains in RAM until New Conversation; later messages are saved again.

Retention limits: 20 conversations, 200 messages per conversation, 16,000 UTF-8
bytes per message, 80 bytes per display role and 512 KiB for the encoded archive.
Truncation preserves UTF-8 character boundaries; oldest messages/sessions are
removed as required. Invalid/oversized/version-incompatible archives are preserved
and writing disabled until explicitly cleared. Symbolic links in storage components,
temporary and backup files are refused (not a guarantee against concurrent external
filesystem replacement). Saves use a temporary file plus a recoverable previous
backup; no cross-platform atomic-rename guarantee is claimed.

Local standalone tests cover round-trip, Unicode, bounds, invalid schema/version,
non-destructive load failure and clear. Native `[GO.][ChatHistory][Editor]` fixtures
cover disk persistence across dock destruction/recreation, interrupted replacement
recovery, malformed-file preservation and confirmed deletion without requests or
tool replay. Native Windows/Linux runtime results and visual acceptance are pending.

## Expanded scene operations

The opt-in scene-only native loop now offers eight tools: `get_current_scene_tree`,
`get_editor_selection`, `add_node`, `rename_node`, `remove_node`, `move_node`,
`get_node_property`, and `set_node_property`. Other draft bridge tools remain
unexposed. Writes still require confirmation unless Auto-Approve was explicitly
selected; the shared 20-action budget also counts failures and denials.

`move_node` accepts scene-relative `node_path`, required `parent_path`, and an
optional nonnegative integer `index`. Reparenting preserves the **local** transform,
rejects root moves/cycles/name collisions, and supports undo to the old parent and
sibling index. `remove_node` retains the detached subtree through an undo reference;
undo restores ownership recursively. Neither action calls `memdelete` on a live
user node at execution time.

New mutation paths reject scripted ancestors, inherited scene roots, foreign-owned
or instanced subtrees and subtrees over 1,000 nodes. Destination nodes are checked
again at execution. Property support is deliberately restricted to plain `Label`
`text`, `tooltip_text`, `visible`, scalar `rotation_degrees` and integer `z_index`.
No arbitrary property setter, script assignment, resource property, BBCode label
text or implicit type coercion is enabled. Text is bounded to 8,000 characters,
rotation to finite ±360,000 degrees and Z-index to integer ±4,096.

Approval captures scene/target/destination identities and the scene undo version.
Changing the scene or replacing the destination while approval is pending rejects
the old action rather than applying it to a newly resolved target. Native tests
exercise shared production undo operations for remove/reparent/property changes,
subtree ownership, sibling position, reference lifetime and invalid values. Their
new runtime and visual results remain pending; syntax checks are not that evidence.

## Bounded project text tools

The opt-in mode is now labeled **Project tools (experimental)** and offers the eight
scene tools plus three read-only file tools. Native model capability verification,
strict schemas, no automatic requests on startup and the 20-action budget remain.
Tool results/file content are explicitly untrusted data in the model system prompt.
File writes, script loading/execution and other draft bridge tools remain unexposed.

| Tool | Arguments | Result and bounds |
| --- | --- | --- |
| `list_files` | optional `path` (`res://` by default) | Sorted collected paths; max 1,000 files, depth 8, 2,000 visited directory entries, about 48 KB output. Hidden/sensitive/link paths omitted. |
| `read_file` | required `path`, optional integer `start_line` (default 1), `line_limit` (default 200, range 1–400) | Selected text lines from a supported UTF-8 file; max 200,000 input bytes and about 48 KB output. No unbounded whole-file read. |
| `search_text` | required `query`, optional `path` (file or directory, default `res://`) | At most 64 candidate files and 50 matching lines; source paths/line numbers included, individual lines clipped to 500 characters. Always reports that the search is bounded, not exhaustive. |

Supported extensions: gd, tscn, tres, godot, cfg, json, md, txt, gdshader,
gdshaderinc, csv, svg, ini, yaml, yml and toml; `.gitignore` and `.gitattributes`
are also readable. Binary/NUL data, invalid UTF-8, unreadable/nonregular files and
oversized files are rejected or omitted from search. Loading text never calls
ResourceLoader, evaluates code, or follows scene/script dependencies.

Whole-file screening uses the same conservative sensitive-name/content markers
as commit-message privacy; it happens before line selection, so selecting the body
of a recognized private key cannot bypass its header detection. These are heuristics,
with false positives (such as names containing `key`) and no guarantee of recognizing
arbitrary embedded secrets. Review the project before granting model tool access.

The central path guard now blocks case variants of `.git`/`.godot`, Windows device
names (including superscript port variants), trailing dots/spaces, short-name aliases
and non-ASCII hidden components that could alias protected names on Windows. These
conservative restrictions also apply on Linux for portable policy. Runtime adapters
still reject every symlink component; this is not a race-proof sandbox against a
concurrent external process replacing filesystem objects.

Native `[GO.][ReadTools]` fixtures cover line ranges, searches, lists, size/type/
secret/path rejection and an actual outside-project symlink on Linux. Windows tests
cover regular-file behavior and pure cross-platform path-policy regressions; native
symlink creation is not required there. New runtime results remain pending.
