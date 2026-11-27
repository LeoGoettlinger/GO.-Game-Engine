/**************************************************************************/
/*  go_agentics_acceptance.cpp                                            */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT

#include "go_agentics_acceptance.h"

#ifdef GO_SUITE_TESTS_ENABLED
#include "go_agentics_tool_bridge.h"
#include "go_agentics_tools.h"
#include "go_system_git_runner.h"

#include "core/config/project_settings.h"
#include "core/io/file_access.h"
#include "core/os/os.h"
#include "editor/editor_interface.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/file_system/editor_file_system.h"
#include "scene/main/scene_tree.h"

bool go_agentics_acceptance_requested() {
	bool requested = false;
	for (const String &argument : OS::get_singleton()->get_cmdline_user_args()) {
		requested |= argument == "--go-agent-acceptance";
	}
	return requested && String(ProjectSettings::get_singleton()->get_setting("application/config/name", String())) == "GO. Agent Acceptance" && FileAccess::exists("res://.go_agent_acceptance");
}

void go_agentics_acceptance_tick(GoAgenticsToolBridge &p_bridge) {
	static int step = 0;
	static int frames = 0;
	static bool finished = false;
	if (finished || ++frames < 60) {
		return;
	}
	auto fail = [&](const String &message) {
		finished = true;
		print_line("GO_AGENT_ACCEPTANCE_FAILED: " + message);
		SceneTree::get_singleton()->quit(1);
	};
	EditorFileSystem *filesystem = EditorFileSystem::get_singleton();
	if (filesystem == nullptr || filesystem->is_scanning() || filesystem->is_importing()) {
		return;
	}
	EditorInterface *editor = EditorInterface::get_singleton();
	if (step == 0 && (FileAccess::exists("res://player.tscn") || FileAccess::exists("res://player.gd") || FileAccess::exists("res://shape.tres") || FileAccess::exists("res://texture.tres"))) {
		fail("Acceptance requires a fresh fixture; existing files will not be overwritten.");
		return;
	}
	if (step >= 2 && editor->get_edited_scene_root() == nullptr) {
		return; // Scene opening/import completion is not assumed synchronous.
	}
	const std::string player = R"GDSCRIPT(extends CharacterBody2D
var smoke_frames: int = 0
func _physics_process(_delta: float) -> void:
    var smoke: bool = "--go-smoke-test" in OS.get_cmdline_user_args()
    velocity.x = 120.0 if smoke else Input.get_axis("ui_left", "ui_right") * 120.0
    move_and_slide()
    if smoke:
        smoke_frames += 1
        if smoke_frames == 3:
            if not ($Sprite2D is Sprite2D) or not ($CollisionShape2D is CollisionShape2D):
                push_error("Generated player lost its child types")
                get_tree().quit(2)
                return
            if "--go-export-test" in OS.get_cmdline_user_args() and ClassDB.class_exists("EditorPlugin"):
                push_error("Editor classes leaked into the export")
                get_tree().quit(3)
                return
            print("GO_PLAYER_SMOKE_OK")
            get_tree().quit()
)GDSCRIPT";
	const std::vector<go::GoKiloToolCall> calls = {
		{ "fixture", "create_scene", R"({"path":"res://player.tscn","type":"CharacterBody2D","name":"Player"})" },
		{ "fixture", "open_scene", R"({"path":"res://player.tscn"})" },
		{ "fixture", "add_node", R"({"node_path":".","type":"Sprite2D","name":"Sprite2D"})" },
		{ "fixture", "add_node", R"({"node_path":".","type":"CollisionShape2D","name":"CollisionShape2D"})" },
		{ "fixture", "create_resource", R"({"path":"res://shape.tres","type":"RectangleShape2D"})" },
		{ "fixture", "assign_resource", R"({"path":"res://shape.tres","node_path":"CollisionShape2D","property":"shape"})" },
		{ "fixture", "create_resource", R"({"path":"res://texture.tres","type":"GradientTexture2D"})" },
		{ "fixture", "assign_resource", R"({"path":"res://texture.tres","node_path":"Sprite2D","property":"texture"})" },
		{ "fixture", "create_script", "{\"path\":\"res://player.gd\",\"content\":" + go::escape_json_string(player) + "}" },
		{ "fixture", "validate_script", R"({"path":"res://player.gd"})" },
		{ "fixture", "attach_script", R"({"node_path":".","script_path":"res://player.gd"})" },
		{ "fixture", "save_current_scene", "{}" },
		{ "fixture", "set_main_scene", R"({"path":"res://player.tscn"})" },
	};
	if (step == int(calls.size())) {
		if (!editor->get_unsaved_scenes().is_empty()) {
			return; // A requested save is not a completed save.
		}
		Node *root = editor->get_edited_scene_root();
		const String saved = FileAccess::get_file_as_string("res://player.tscn");
		if (root == nullptr || !root->is_class("CharacterBody2D") || root->get_script().is_null() || !saved.contains("player.gd") || !saved.contains("CollisionShape2D") || String(ProjectSettings::get_singleton()->get_setting("application/run/main_scene")) != "res://player.tscn") {
			fail("Final editor tree, persisted scene or main-scene setting is incomplete.");
			return;
		}
		finished = true;
		print_line("GO_AGENT_SCENE_OK");
		SceneTree::get_singleton()->quit();
		return;
	}
	const auto &call = calls[step];
	String result;
	if (go::GoAgenticsTools::is_file_mutation(call.name) || go::GoAgenticsTools::is_editor_action(call.name)) {
		String preview;
		String fingerprint;
		const bool file = go::GoAgenticsTools::is_file_mutation(call.name);
		const String error = file ? p_bridge.prepare_file_change(call, preview, fingerprint) : p_bridge.prepare_editor_action(call, preview, fingerprint);
		if (!error.is_empty() || fingerprint.is_empty() || preview.is_empty()) {
			fail(go_suite::to_godot_string(call.name) + ": " + error);
			return;
		}
		// Deterministic offline test approval only. This hook requires a tests=yes
		// editor, an explicit CLI flag, a named fixture and its marker file.
		// It is not a live model or a simulation of a user clicking the dialog.
		result = file ? p_bridge.execute_file_change(call, fingerprint) : p_bridge.execute_editor_action(call, fingerprint);
	} else {
		result = p_bridge.execute(call);
	}
	print_line("GO_AGENT_STEP " + go_suite::to_godot_string(call.name) + ": " + result);
	std::string parse_error;
	const auto parsed = go::parse_json(go_suite::to_std_string(result), parse_error);
	const auto *ok = parsed.get("ok");
	if (!parse_error.empty() || ok == nullptr || !ok->as_bool()) {
		fail("Tool did not succeed: " + go_suite::to_godot_string(call.name));
		return;
	}
	if (step == 3) {
		EditorUndoRedoManager *undo = EditorUndoRedoManager::get_singleton();
		Node *root = editor->get_edited_scene_root();
		if (!undo->undo() || root->has_node(NodePath("CollisionShape2D")) || !undo->redo() || !root->has_node(NodePath("CollisionShape2D"))) {
			fail("Editor node Undo/Redo did not restore the player hierarchy.");
			return;
		}
	}
	++step;
}
#endif // GO_SUITE_TESTS_ENABLED
