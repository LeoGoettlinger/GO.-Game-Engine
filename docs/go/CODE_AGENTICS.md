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
