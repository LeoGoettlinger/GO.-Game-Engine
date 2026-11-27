/**************************************************************************/
/*  go_agentics_tool_bridge.cpp                                           */
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

#include "go_agentics_tool_bridge.h"

#include "go_agentics_path.h"
#include "go_agentics_tools.h"
#include "go_commit_message.h"
#include "go_git_controller.h"
#include "go_suite_core.h"
#include "go_suite_registry.h"
#include "go_system_git_runner.h"

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/io/resource_uid.h"
#include "core/object/class_db.h"
#include "core/object/script_language.h"
#include "core/os/os.h"
#include "core/string/node_path.h"
#include "core/variant/variant.h"
#include "editor/editor_data.h"
#include "editor/editor_interface.h"
#include "editor/editor_node.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/script/script_editor_plugin.h"
#include "scene/2d/node_2d.h"
#include "scene/main/node.h"
#include "scene/resources/packed_scene.h"

#include "modules/gdscript/gdscript_parser.h"

#include <cmath>
#include <sstream>

namespace {

const int MAX_FILES = 1000;
const int MAX_DEPTH = 8;
const int MAX_TEXT_CHARS = 50000;

std::string ok_json(const std::string &p_result) {
	return std::string("{\"ok\":true,\"result\":") + go::escape_json_string(p_result) + "}";
}

std::string error_json(const std::string &p_error) {
	return std::string("{\"ok\":false,\"error\":") + go::escape_json_string(p_error) + "}";
}

std::string argument_string(const go::JsonValue &p_arguments, const char *p_key, const std::string &p_default = "") {
	const go::JsonValue *value = p_arguments.get(p_key);
	return value == nullptr ? p_default : value->as_string(p_default);
}

String to_godot(const std::string &p_text) {
	return go_suite::to_godot_string(p_text);
}

std::string to_std(const String &p_text) {
	return go_suite::to_std_string(p_text);
}

// Reject links in every component, including the final target. This intentionally
// rejects even in-project links until a race-resistant resolver is available.
bool has_link_component(const String &p_path) {
	Ref<DirAccess> directory = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	if (directory.is_null()) {
		return true;
	}
	String current = "res://";
	for (const String &part : p_path.trim_prefix("res://").split("/", false)) {
		current = current.path_join(part);
		if (directory->is_link(current)) {
			return true;
		}
	}
	return false;
}

void collect_files(const String &p_directory, Vector<String> &r_files, int p_depth = 0, int *p_budget = nullptr) {
	int local_budget = 2000;
	if (p_budget == nullptr) {
		p_budget = &local_budget;
	}
	if (p_depth > MAX_DEPTH || r_files.size() >= MAX_FILES) {
		return;
	}
	Ref<DirAccess> directory = DirAccess::open(p_directory);
	if (directory.is_null()) {
		return;
	}
	directory->list_dir_begin();
	String name = directory->get_next();
	while (!name.is_empty() && r_files.size() < MAX_FILES && *p_budget > 0) {
		--*p_budget;
		if (!directory->current_is_hidden() && name != ".godot" && !directory->is_link(name)) {
			const String full = p_directory.path_join(name);
			if (!go::is_safe_res_path(to_std(full), go::PathOperation::READ) || go::looks_like_secret_file(to_std(full))) {
				name = directory->get_next();
				continue;
			}
			if (directory->current_is_dir()) {
				collect_files(full, r_files, p_depth + 1, p_budget);
			} else {
				r_files.push_back(full);
			}
		}
		name = directory->get_next();
	}
	directory->list_dir_end();
}

bool read_project_text(const String &p_path, String &r_text, std::string &r_error, Vector<uint8_t> *r_raw = nullptr) {
	r_text = String();
	if (has_link_component(p_path) || go::looks_like_secret_file(to_std(p_path))) {
		r_error = "Sensitive paths and symbolic links are not readable by the agent";
		return false;
	}
	const String extension = p_path.get_extension().to_lower();
	const String base = p_path.get_file();
	if (extension != "gd" && extension != "tscn" && extension != "tres" && extension != "godot" && extension != "cfg" && extension != "json" && extension != "md" && extension != "txt" && extension != "gdshader" && extension != "gdshaderinc" && extension != "csv" && extension != "svg" && extension != "ini" && extension != "yaml" && extension != "yml" && extension != "toml" && base != ".gitignore" && base != ".gitattributes") {
		r_error = "Only supported UTF-8 project text files can be read";
		return false;
	}
	if (!FileAccess::exists(p_path)) {
		r_error = "A regular project text file is required";
		return false;
	}
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ);
	constexpr uint64_t MAX_BYTES = 200000;
	if (file.is_null() || file->get_length() > MAX_BYTES) {
		r_error = "File is unavailable or exceeds the 200000-byte read limit";
		return false;
	}
	// Never use an unbounded get_as_text(): the file can grow after the size check.
	Vector<uint8_t> bytes;
	bytes.resize(MAX_BYTES + 1);
	const uint64_t count = file->get_buffer(bytes.ptrw(), MAX_BYTES + 1);
	if (count > MAX_BYTES || (file->get_error() != OK && file->get_error() != ERR_FILE_EOF)) {
		r_error = "Read limit or file I/O error";
		return false;
	}
	for (uint64_t i = 0; i < count; ++i) {
		if (bytes[i] == 0) {
			r_error = "Binary content is not readable by the agent";
			return false;
		}
	}
	if (r_text.append_utf8(reinterpret_cast<const char *>(bytes.ptr()), int(count)) != OK) {
		r_error = "File is not valid UTF-8";
		return false;
	}
	if (go::looks_like_secret_content(to_std(r_text))) {
		r_text = String();
		r_error = "File omitted because it contains possible credentials or private key material";
		return false;
	}
	if (r_raw != nullptr) {
		bytes.resize(count);
		*r_raw = bytes;
	}
	return true;
}

std::string describe_node(const Node *p_node, int p_depth, int &r_budget) {
	if (p_node == nullptr || p_depth > 10 || r_budget-- <= 0) {
		return "";
	}
	std::ostringstream out;
	out << std::string(p_depth * 2, ' ') << to_std(String(p_node->get_name()).substr(0, 128)) << " [" << to_std(p_node->get_class()) << "]\n";
	for (int i = 0; i < p_node->get_child_count(); ++i) {
		if (r_budget <= 0 || out.tellp() >= MAX_TEXT_CHARS) {
			break;
		}
		out << describe_node(p_node->get_child(i), p_depth + 1, r_budget);
	}
	return out.str();
}

Variant json_to_variant(const go::JsonValue *p_value) {
	if (p_value == nullptr) {
		return Variant();
	}
	switch (p_value->type) {
		case go::JsonValue::Type::BOOL:
			return Variant(p_value->bool_value);
		case go::JsonValue::Type::NUMBER:
			return Variant(p_value->number_value);
		case go::JsonValue::Type::STRING:
			return Variant(to_godot(p_value->string_value));
		default:
			return Variant(to_godot(p_value->as_string()));
	}
}

bool editable_subtree(Node *p_node, Node *p_root) {
	std::vector<Node *> pending = { p_node };
	int budget = MAX_FILES;
	while (!pending.empty()) {
		Node *node = pending.back();
		pending.pop_back();
		if (--budget < 0 || !node->get_script().is_null() || (node != p_root && (node->get_owner() != p_root || !node->get_scene_file_path().is_empty()))) {
			return false;
		}
		if (node->get_child_count() > budget) {
			return false;
		}
		for (int i = 0; i < node->get_child_count(); ++i) {
			pending.push_back(node->get_child(i));
		}
	}
	return true;
}

struct FileChangePlan {
	String path;
	String before;
	String after;
	String fingerprint;
	Vector<uint8_t> original;
	bool existed = false;
};

bool file_is_open(const String &p_path) {
	if (ResourceCache::has(p_path)) {
		return true;
	}
	if (ScriptEditor::get_singleton() != nullptr) {
		for (const Ref<Script> &script : ScriptEditor::get_singleton()->get_open_scripts()) {
			if (script.is_valid() && script->get_path() == p_path) {
				return true;
			}
		}
	}
	if (EditorNode::get_singleton() != nullptr && EditorInterface::get_singleton() != nullptr) {
		for (const String &scene : EditorInterface::get_singleton()->get_open_scenes()) {
			if (scene == p_path) {
				return true;
			}
		}
	}
	return false;
}

std::string backup_project_bytes(const String &p_path, const Vector<uint8_t> &p_original, String &r_backup) {
	const String folder = "res://.godot/go_agent_backups";
	if (has_link_component(folder) || DirAccess::make_dir_recursive_absolute(folder) != OK) {
		return "Cannot create a safe local backup directory";
	}
	Ref<DirAccess> directory = DirAccess::open(folder);
	if (directory.is_null() || directory->list_dir_begin() != OK) {
		return "Cannot inspect local backups";
	}
	int count = 0;
	while (!directory->get_next().is_empty() && count < 200) {
		++count;
	}
	directory->list_dir_end();
	if (count >= 200) {
		return "Backup limit reached; review .godot/go_agent_backups before more file changes";
	}
	r_backup = folder.path_join(itos(OS::get_singleton()->get_ticks_usec()) + "-" + p_path.get_file() + ".bak");
	if (has_link_component(r_backup) || FileAccess::exists(r_backup)) {
		return "Backup collision; no source file changed";
	}
	Ref<FileAccess> file = FileAccess::open(r_backup, FileAccess::WRITE);
	if (file.is_null() || !file->store_buffer(p_original)) {
		return "Could not write backup; no source file changed";
	}
	file->flush();
	if (file->get_error() != OK) {
		return "Could not flush backup; no source file changed";
	}
	return "";
}

std::string plan_main_scene(const go::JsonValue &p_args, FileChangePlan &r_plan) {
	const auto checked = go::validate_res_path(argument_string(p_args, "path"), go::PathOperation::READ);
	if (!checked.ok) {
		return checked.error;
	}
	const String scene = to_godot(checked.normalized);
	if (scene.get_extension().to_lower() != "tscn") {
		return "Main scene must be a saved res:// text scene";
	}
	String scene_text;
	std::string error;
	if (!read_project_text(scene, scene_text, error)) {
		return error;
	}
	if (!scene_text.strip_edges().begins_with("[gd_scene ")) {
		return "Main scene is missing its text-scene header";
	}
	r_plan.path = "res://project.godot";
	r_plan.existed = true;
	if (file_is_open(r_plan.path) || !read_project_text(r_plan.path, r_plan.before, error, &r_plan.original)) {
		return error.empty() ? "Close the project configuration text buffer before changing the main scene" : error;
	}
	// Serialize current settings into a private temporary preview, not into the
	// actual project. The user sees ALL changes, including pending settings.
	const String folder = "res://.godot/go_agent_previews";
	if (has_link_component(folder) || DirAccess::make_dir_recursive_absolute(folder) != OK) {
		return "Cannot create a safe settings preview directory";
	}
	const String temporary = folder.path_join(itos(OS::get_singleton()->get_ticks_usec()) + ".godot");
	if (has_link_component(temporary) || FileAccess::exists(temporary)) {
		return "Settings preview collision; no configuration changed";
	}
	ProjectSettings::CustomMap custom;
	custom["application/run/main_scene"] = scene;
	const Error saved = ProjectSettings::get_singleton()->save_custom(temporary, custom);
	const bool read = saved == OK && read_project_text(temporary, r_plan.after, error);
	if (FileAccess::exists(temporary)) {
		DirAccess::remove_absolute(temporary);
	}
	if (!read) {
		return error.empty() ? "Could not serialize settings preview" : error;
	}
	const String old_main = ProjectSettings::get_singleton()->get_setting("application/run/main_scene", String());
	r_plan.fingerprint = (ProjectSettings::get_singleton()->globalize_path("res://") + "\nset_main_scene\n" + scene + "\n" + scene_text.sha256_text() + "\n" + old_main + "\n" + String::hex_encode_buffer(r_plan.original.ptr(), r_plan.original.size()) + "\n" + r_plan.after).sha256_text();
	return "";
}

std::string plan_file_change(const go::GoKiloToolCall &p_call, FileChangePlan &r_plan) {
	if (!go::GoAgenticsTools::is_file_mutation(p_call.name) || GoSuiteRegistry::is_git_job_busy() || GoSuiteRegistry::is_quit_pending() || GoSuiteRegistry::is_editor_input_locked()) {
		return "File mutation is unavailable during Git/close operations";
	}
	std::string error = go::GoAgenticsTools::validate_arguments(p_call.name, p_call.arguments);
	if (!error.empty()) {
		return error;
	}
	const auto args = go::parse_json(p_call.arguments, error);
	if (p_call.name == "set_main_scene") {
		return plan_main_scene(args, r_plan);
	}
	const auto checked = go::validate_res_path(argument_string(args, "path"), go::PathOperation::WRITE);
	if (!checked.ok) {
		return checked.error;
	}
	r_plan.path = to_godot(checked.normalized);
	const auto parts = go::split(checked.relative, '/');
	for (const auto &part : parts) {
		if (part.front() == '.' || go::lower(part) == "addons" || go::looks_like_secret_file(part)) {
			return "Hidden, sensitive and addon paths cannot be modified by file tools";
		}
	}
	if (has_link_component(r_plan.path) || !DirAccess::dir_exists_absolute(r_plan.path.get_base_dir())) {
		return "Parent directory must exist and no path component may be a symbolic link";
	}
	if (DirAccess::dir_exists_absolute(r_plan.path)) {
		return "A directory already occupies the target path";
	}
	r_plan.existed = FileAccess::exists(r_plan.path);
	if (p_call.name == "create_folder") {
		if (r_plan.existed) {
			return "A file already occupies the folder path";
		}
	} else {
		const String extension = r_plan.path.get_extension().to_lower();
		if (p_call.name != "create_scene" && p_call.name != "create_resource" && extension != "gd" && extension != "txt" && extension != "md" && extension != "json" && extension != "csv") {
			return "File mutations currently support only gd, txt, md, json and csv; use scene/resource tools for other types";
		}
		if ((p_call.name == "create_scene" && (extension != "tscn" || r_plan.existed)) || (p_call.name == "create_resource" && (extension != "tres" || r_plan.existed))) {
			return "Scene/resource creation requires a new .tscn/.tres target; existing files are never overwritten";
		}
		if (file_is_open(r_plan.path)) {
			return "Close the file/resource in the editor before changing it; unsaved editor work is never overwritten";
		}
		if (r_plan.existed && !read_project_text(r_plan.path, r_plan.before, error, &r_plan.original)) {
			return error;
		}
		if (p_call.name == "create_script" && (r_plan.existed || extension != "gd")) {
			return "create_script requires a new .gd file";
		}
		if ((p_call.name == "edit_file" || p_call.name == "delete_file") && !r_plan.existed) {
			return "The file to edit/delete does not exist";
		}
		if (p_call.name == "create_scene" || p_call.name == "create_resource") {
			const String type = to_godot(argument_string(args, "type"));
			if (p_call.name == "create_scene") {
				if (type != "Node" && type != "Node2D" && type != "Node3D" && type != "CharacterBody2D" && type != "CharacterBody3D" && type != "Control") {
					return "Scene root type is outside the supported built-in allowlist";
				}
				const String name = to_godot(argument_string(args, "name"));
				if (!name.is_valid_ascii_identifier() || name.length() > 128) {
					return "Scene root name must be an ASCII identifier of at most 128 characters";
				}
				r_plan.after = "[gd_scene format=3]\n\n[node name=\"" + name + "\" type=\"" + type + "\"]\n";
			} else {
				if (type != "RectangleShape2D" && type != "CircleShape2D" && type != "CapsuleShape2D" && type != "BoxShape3D" && type != "SphereShape3D" && type != "StandardMaterial3D" && type != "Gradient" && type != "GradientTexture2D" && type != "SpriteFrames") {
					return "Resource type is outside the supported built-in allowlist";
				}
				r_plan.after = "[gd_resource type=\"" + type + "\" format=3]\n\n[resource]\n";
			}
			if (!ClassDB::can_instantiate(type)) {
				return "Requested built-in type is unavailable in this build";
			}
		} else if (p_call.name == "edit_file") {
			const std::string old_text = argument_string(args, "old_text");
			std::string text = to_std(r_plan.before);
			const size_t found = text.find(old_text);
			if (old_text.empty() || found == std::string::npos || text.find(old_text, found + 1) != std::string::npos) {
				return "old_text must match exactly once, including overlapping occurrences";
			}
			text.replace(found, old_text.size(), argument_string(args, "new_text"));
			r_plan.after = to_godot(text);
			if (r_plan.original.size() >= 3 && r_plan.original[0] == 0xef && r_plan.original[1] == 0xbb && r_plan.original[2] == 0xbf) {
				r_plan.after = String::chr(0xfeff) + r_plan.after;
			}
		} else if (p_call.name != "delete_file") {
			r_plan.after = to_godot(argument_string(args, "content"));
		}
		const std::string content = to_std(r_plan.after);
		if (content.size() > 200000 || go::looks_like_secret_content(content)) {
			return "Result exceeds the file limit or contains possible credentials";
		}
		if (extension == "gd" && p_call.name != "delete_file") {
			if (content.find("@tool") != std::string::npos || content.find("static") != std::string::npos || content.find("preload") != std::string::npos) {
				return "Editor tools, static initialization and preloads are not allowed in generated scripts";
			}
			String first;
			for (const String &line : r_plan.after.split("\n")) {
				const String trimmed = line.strip_edges();
				if (!trimmed.is_empty() && !trimmed.begins_with("#")) {
					first = trimmed;
					break;
				}
			}
			const String type = first.trim_prefix("extends ").strip_edges();
			if (!first.begins_with("extends ") || !ClassDB::class_exists(type) || !ClassDB::is_parent_class(type, "Node") || ClassDB::is_parent_class(type, "EditorPlugin")) {
				return "Generated scripts must begin with extends and a built-in non-editor Node class";
			}
		}
	}
	r_plan.fingerprint = (ProjectSettings::get_singleton()->globalize_path("res://") + "\n" + r_plan.path + "\n" + to_godot(p_call.name) + "\n" + (r_plan.existed ? "existing" : "new") + "\n" + String::hex_encode_buffer(r_plan.original.ptr(), r_plan.original.size()) + "\n" + r_plan.after).sha256_text();
	return "";
}

} // namespace

Node *GoAgenticsToolBridge::create_allowed_node(const String &p_type) {
	// Never instantiate arbitrary classes supplied by the model. In particular,
	// editor plugins, custom scripts and OS/process-bearing classes stay excluded.
	if (p_type != "Node" && p_type != "Node2D" && p_type != "Node3D" && p_type != "Control" && p_type != "Sprite2D" && p_type != "Label" && p_type != "CharacterBody2D" && p_type != "CollisionShape2D" && p_type != "CharacterBody3D" && p_type != "CollisionShape3D") {
		return nullptr;
	}
	if (!ClassDB::can_instantiate(p_type)) {
		return nullptr;
	}
	Object *instance = ClassDB::instantiate(p_type);
	Node *node = Object::cast_to<Node>(instance);
	if (node == nullptr && instance != nullptr) {
		memdelete(instance);
	}
	return node;
}

void GoAgenticsToolBridge::record_add_node(UndoRedo *p_undo, Node *p_parent, Node *p_child, Node *p_owner) {
	p_undo->add_do_method(Callable(p_parent, "add_child").bind(p_child, true));
	p_undo->add_do_method(Callable(p_child, "set_owner").bind(p_owner));
	p_undo->add_undo_method(Callable(p_child, "set_owner").bind(Variant()));
	p_undo->add_undo_method(Callable(p_parent, "remove_child").bind(p_child));
	p_undo->add_do_reference(p_child);
}

void GoAgenticsToolBridge::record_rename_node(UndoRedo *p_undo, Node *p_node, const String &p_name) {
	p_undo->add_do_method(Callable(p_node, "set_name").bind(p_name));
	p_undo->add_undo_method(Callable(p_node, "set_name").bind(p_node->get_name()));
}

void GoAgenticsToolBridge::record_remove_node(UndoRedo *p_undo, Node *p_node) {
	Node *parent = p_node->get_parent();
	p_undo->add_do_method(Callable(parent, "remove_child").bind(p_node));
	p_undo->add_undo_method(Callable(parent, "add_child").bind(p_node, true));
	p_undo->add_undo_method(Callable(parent, "move_child").bind(p_node, p_node->get_index()));
	// Removal clears owners whose ancestor lies outside the detached subtree.
	std::vector<Node *> pending = { p_node };
	while (!pending.empty()) {
		Node *node = pending.back();
		pending.pop_back();
		p_undo->add_undo_method(Callable(node, "set_owner").bind(node->get_owner()));
		for (int i = 0; i < node->get_child_count(); ++i) {
			pending.push_back(node->get_child(i));
		}
	}
	p_undo->add_undo_reference(p_node);
}

void GoAgenticsToolBridge::record_move_node(UndoRedo *p_undo, Node *p_node, Node *p_parent, int p_index) {
	Node *old_parent = p_node->get_parent();
	p_undo->add_do_method(Callable(p_node, "reparent").bind(p_parent, false));
	p_undo->add_do_method(Callable(p_parent, "move_child").bind(p_node, p_index));
	p_undo->add_undo_method(Callable(p_node, "reparent").bind(old_parent, false));
	p_undo->add_undo_method(Callable(old_parent, "move_child").bind(p_node, p_node->get_index()));
}

void GoAgenticsToolBridge::record_set_property(UndoRedo *p_undo, Node *p_node, const StringName &p_property, const Variant &p_value) {
	p_undo->add_do_property(p_node, p_property, p_value);
	p_undo->add_undo_property(p_node, p_property, p_node->get(p_property));
}

std::string GoAgenticsToolBridge::validate_scalar_property(Node *p_node, const StringName &p_property, const Variant &p_value, Variant &r_value) {
	if (p_property != "text" && p_property != "tooltip_text" && p_property != "visible" && p_property != "rotation_degrees" && p_property != "z_index") {
		return "Only text, tooltip_text, visible, rotation_degrees and z_index are enabled";
	}
	if (p_property == "text" && !p_node->is_class("Label")) {
		return "Text editing is restricted to plain Label nodes";
	}
	bool valid = false;
	const Variant current = p_node->get(p_property, &valid);
	if (!valid) {
		return "The node has no such property";
	}
	if (current.get_type() == Variant::BOOL && p_value.get_type() == Variant::BOOL) {
		r_value = p_value;
		return "";
	}
	if (current.get_type() == Variant::STRING && p_value.get_type() == Variant::STRING && String(p_value).length() <= 8000) {
		r_value = p_value;
		return "";
	}
	if ((current.get_type() == Variant::FLOAT || current.get_type() == Variant::INT) && (p_value.get_type() == Variant::FLOAT || p_value.get_type() == Variant::INT)) {
		const double value = p_value;
		const double limit = p_property == "z_index" ? 4096 : 360000;
		if (std::isfinite(value) && std::abs(value) <= limit && (current.get_type() != Variant::INT || std::floor(value) == value)) {
			r_value = current.get_type() == Variant::INT ? Variant(int64_t(value)) : Variant(value);
			return "";
		}
	}
	return "Property value has the wrong type or exceeds its bounds";
}

String GoAgenticsToolBridge::prepare_editor_action(const go::GoKiloToolCall &p_call, String &r_preview, String &r_fingerprint) const {
	r_preview = String();
	r_fingerprint = String();
	if (!go::GoAgenticsTools::is_editor_action(p_call.name)) {
		return TTR("Not an editor execution action.");
	}
	const std::string validation = go::GoAgenticsTools::validate_arguments(p_call.name, p_call.arguments);
	if (!validation.empty()) {
		return to_godot(validation);
	}
	if (GoSuiteRegistry::is_git_job_busy() || GoSuiteRegistry::is_quit_pending() || GoSuiteRegistry::is_editor_input_locked()) {
		return TTR("Editor execution is locked by a Git/close operation.");
	}
	EditorInterface *editor = EditorInterface::get_singleton();
	if (EditorNode::get_singleton() == nullptr || editor == nullptr) {
		return TTR("A running editor is required.");
	}
	const bool saving = p_call.name == "save_current_scene" || p_call.name == "save_scene_as";
	const bool instantiating = p_call.name == "instantiate_scene";
	const bool resource_assignment = p_call.name == "assign_resource";
	const bool attaching = p_call.name == "attach_script" || resource_assignment || instantiating;
	if (!attaching && !saving && p_call.name != "stop_project" && !editor->get_unsaved_scenes().is_empty()) {
		return TTR("Save or discard unsaved scenes yourself before this action; the agent will not autosave them.");
	}
	if (p_call.name != "stop_project" && ScriptEditor::get_singleton() != nullptr && (!ScriptEditor::get_singleton()->get_unsaved_files().is_empty() || !ScriptEditor::get_singleton()->get_unsaved_scripts().is_empty())) {
		return TTR("Save or discard unsaved script/text buffers yourself before this action.");
	}
	if (p_call.name != "stop_project" && editor->is_playing_scene()) {
		return TTR("Stop the running project before opening or starting another scene.");
	}
	Node *root = editor->get_edited_scene_root();
	const String project = ProjectSettings::get_singleton()->globalize_path("res://");
	String path;
	String source;
	Node *attachment_target = nullptr;
	if (attaching) {
		std::string parse_error;
		const auto args = go::parse_json(p_call.arguments, parse_error);
		path = to_godot(argument_string(args, (resource_assignment || instantiating) ? "path" : "script_path"));
		const String node_path = to_godot(argument_string(args, "node_path", "."));
		if (root == nullptr || root->get_scene_inherited_state().is_valid() || node_path.is_empty() || node_path.begins_with("/") || node_path.contains("..") || node_path.contains(":")) {
			return TTR("A local target inside the current non-inherited scene is required.");
		}
		attachment_target = root->get_node_or_null(NodePath(node_path));
		if (attachment_target == nullptr || !attachment_target->get_script().is_null() || (attachment_target != root && (attachment_target->get_owner() != root || !attachment_target->get_scene_file_path().is_empty()))) {
			return TTR("Attach Script requires a locally owned node without an existing script; existing script state is never replaced.");
		}
		if (instantiating) {
			const String name = to_godot(argument_string(args, "name"));
			if (!name.is_valid_ascii_identifier() || name.length() > 128 || attachment_target->has_node(NodePath(name))) {
				return TTR("Instance name must be an unused ASCII identifier of at most 128 characters.");
			}
		}
		if (resource_assignment) {
			const std::string property = argument_string(args, "property");
			if (!((property == "texture" && attachment_target->is_class("Sprite2D")) || (property == "shape" && (attachment_target->is_class("CollisionShape2D") || attachment_target->is_class("CollisionShape3D"))))) {
				return TTR("Resource assignment supports Sprite2D.texture and CollisionShape2D/3D.shape only.");
			}
		}
		if (EditorUndoRedoManager::get_singleton() == nullptr) {
			return TTR("Editor undo history is required for script attachment.");
		}
	} else if (p_call.name == "open_scene" || p_call.name == "save_scene_as") {
		std::string error;
		const auto args = go::parse_json(p_call.arguments, error);
		path = to_godot(argument_string(args, "path"));
	} else if (p_call.name == "run_current_scene" || p_call.name == "save_current_scene") {
		if (root == nullptr) {
			return TTR("No current scene is open.");
		}
		path = root->get_scene_file_path();
	} else if (p_call.name == "run_project") {
		path = ProjectSettings::get_singleton()->get_setting("application/run/main_scene", String());
	}
	if (saving) {
		if (root == nullptr) {
			return TTR("No scene is open to save.");
		}
		for (const String &unsaved : editor->get_unsaved_scenes()) {
			if (unsaved != root->get_scene_file_path()) {
				return TTR("Save or discard other unsaved scenes before requesting this save.");
			}
		}
	}
	Vector<uint8_t> original_bytes;
	if (p_call.name != "stop_project") {
		if (path.begins_with("uid://")) {
			const ResourceUID::ID uid = ResourceUID::get_singleton()->text_to_id(path);
			if (!ResourceUID::get_singleton()->has_id(uid)) {
				return TTR("Main scene UID is not resolved; import/save the project first.");
			}
			path = ResourceUID::get_singleton()->get_id_path(uid);
		}
		const auto checked = go::validate_res_path(to_std(path), saving ? go::PathOperation::WRITE : go::PathOperation::READ);
		if (!checked.ok || path.get_extension().to_lower() != (resource_assignment ? "tres" : ((attaching && !instantiating) ? "gd" : "tscn"))) {
			return TTR("A saved res:// .tscn scene or .gd attachment script is required for this action.");
		}
		path = to_godot(checked.normalized);
		if (has_link_component(path) || !DirAccess::dir_exists_absolute(path.get_base_dir()) || DirAccess::dir_exists_absolute(path)) {
			return TTR("A safe scene file with an existing parent directory is required.");
		}
		if (saving && path != root->get_scene_file_path() && FileAccess::exists(path)) {
			return TTR("Save As requires a new target; another existing scene will not be overwritten.");
		}
		if (!saving || FileAccess::exists(path)) {
			std::string error;
			if (!read_project_text(path, source, error, &original_bytes)) {
				return to_godot(error);
			}
			if (resource_assignment && !source.strip_edges().begins_with("[gd_resource ")) {
				return TTR("Target does not have a text-resource header.");
			}
			if ((!attaching || instantiating) && !source.strip_edges().begins_with("[gd_scene ")) {
				return TTR("Target does not have a text-scene header.");
			}
		}
	}
	String state = project + "\n" + to_godot(p_call.name) + "\n" + to_godot(p_call.arguments) + "\n" + path + "\n" + source.sha256_text() + "\n" + String::hex_encode_buffer(original_bytes.ptr(), original_bytes.size()) + "\n" + (!path.is_empty() && FileAccess::exists(path) ? "existing" : "missing");
	state += "\n" + itos(root != nullptr ? uint64_t(root->get_instance_id()) : 0);
	state += "\n" + itos(attachment_target != nullptr ? uint64_t(attachment_target->get_instance_id()) : 0);
	if (resource_assignment) {
		std::string parse_error;
		const auto args = go::parse_json(p_call.arguments, parse_error);
		Ref<Resource> previous = attachment_target->get(to_godot(argument_string(args, "property")));
		state += "\n" + itos(previous.is_valid() ? uint64_t(previous->get_instance_id()) : 0);
	}
	state += "\n" + editor->get_playing_scene() + "\n" + (editor->is_playing_scene() ? "playing" : "stopped");
	if (root != nullptr && EditorUndoRedoManager::get_singleton() != nullptr) {
		EditorUndoRedoManager *undo = EditorUndoRedoManager::get_singleton();
		state += "\n" + itos(undo->get_history_undo_redo(undo->get_history_id_for_object(root))->get_version());
	}
	r_fingerprint = state.sha256_text();
	r_preview = to_godot(p_call.name) + "\n" + project + "\n" + path + "\n\n";
	if (instantiating) {
		r_preview += TTR("Load and instantiate this scene as a child of the approved local node. Scene loading/initialization may execute scripts with your user permissions; this is NOT a sandbox. Only insertion has Undo/Redo, not arbitrary script effects.");
		return String();
	}
	if (resource_assignment) {
		r_preview += TTR("Load and assign this resource to the approved built-in property. Resource dependencies may execute tool/static scripts; this is NOT a sandbox. Only the property assignment has Undo/Redo, not resource-loading side effects.");
		return String();
	}
	if (attaching) {
		r_preview += TTR("Load and attach this script to the approved local node. Loading may execute static initialization and tool scripts with your user permissions; this is NOT a sandbox. Only the script assignment has Undo/Redo, not arbitrary script side effects. Existing scripts are never replaced.");
		return String();
	}
	if (saving) {
		r_preview += TTR("Save the current scene using the editor. Existing scene bytes are backed up locally. Normal save callbacks and external-resource side effects may occur. This is not an Undo/Redo action. Check the editor for save errors.");
		return String();
	}
	r_preview += p_call.name == "stop_project" ? TTR("Stop the editor's running project. This is not an Undo/Redo action.") : TTR("Opening scenes may execute tool scripts. Running a project executes its scripts with your user permissions. This is NOT a sandbox. Normal editor imports/build/run side effects still apply. Dependencies are not frozen by this approval; review and trust the entire project first. This is not an Undo/Redo action.");
	return String();
}

String GoAgenticsToolBridge::execute_editor_action(const go::GoKiloToolCall &p_call, const String &p_fingerprint) const {
	String preview;
	String fingerprint;
	const String error = prepare_editor_action(p_call, preview, fingerprint);
	if (!error.is_empty() || p_fingerprint.is_empty() || fingerprint != p_fingerprint) {
		return to_godot(error_json(error.is_empty() ? "Project, scene or run state changed after approval; request a fresh action" : to_std(error)));
	}
	EditorInterface *editor = EditorInterface::get_singleton();
	if (p_call.name == "instantiate_scene") {
		std::string parse_error;
		const auto args = go::parse_json(p_call.arguments, parse_error);
		const auto checked = go::validate_res_path(argument_string(args, "path"), go::PathOperation::READ);
		Ref<PackedScene> scene = ResourceLoader::load(to_godot(checked.normalized), "PackedScene", ResourceFormatLoader::CACHE_MODE_IGNORE);
		if (scene.is_null() || !scene->can_instantiate()) {
			return to_godot(error_json("Scene did not load or cannot be instantiated; inspect editor errors"));
		}
		Node *instance = scene->instantiate(PackedScene::GEN_EDIT_STATE_INSTANCE);
		if (instance == nullptr) {
			return to_godot(error_json("Scene instantiation failed"));
		}
		std::vector<Node *> pending = { instance };
		int count = 0;
		while (!pending.empty() && count <= 1000) {
			Node *node = pending.back();
			pending.pop_back();
			++count;
			if (node->get_child_count() > 1000 - count) {
				count = 1001;
				break;
			}
			for (int i = 0; i < node->get_child_count(); ++i) {
				pending.push_back(node->get_child(i));
			}
		}
		const String recheck = prepare_editor_action(p_call, preview, fingerprint);
		if (count > 1000 || !recheck.is_empty() || fingerprint != p_fingerprint) {
			memdelete(instance);
			return to_godot(error_json("Loaded scene exceeds 1000 nodes or editor state changed; insertion aborted. Initialization effects are not rolled back."));
		}
		Node *root = editor->get_edited_scene_root();
		Node *parent = root->get_node_or_null(NodePath(to_godot(argument_string(args, "node_path", "."))));
		instance->set_name(to_godot(argument_string(args, "name")));
		EditorUndoRedoManager *undo = EditorUndoRedoManager::get_singleton();
		undo->create_action(TTR("Code Agentics: Instantiate Scene"), UndoRedo::MERGE_DISABLE, root);
		undo->force_fixed_history();
		record_add_node(undo->get_history_undo_redo(undo->get_history_id_for_object(root)), parent, instance, root);
		undo->commit_action();
		return to_godot(ok_json("Scene insertion committed to editor undo history. Initialization effects are not undone; save the scene separately."));
	}
	if (p_call.name == "attach_script" || p_call.name == "assign_resource") {
		const bool resource_assignment = p_call.name == "assign_resource";
		std::string parse_error;
		const auto args = go::parse_json(p_call.arguments, parse_error);
		const auto checked = go::validate_res_path(argument_string(args, resource_assignment ? "path" : "script_path"), go::PathOperation::READ);
		Ref<Resource> resource = ResourceLoader::load(to_godot(checked.normalized), resource_assignment ? "" : "Script", ResourceFormatLoader::CACHE_MODE_IGNORE);
		Ref<Script> script = resource;
		if (resource.is_null() || (!resource_assignment && (script.is_null() || !script->is_valid()))) {
			return to_godot(error_json("Resource/script did not load or validate; no assignment was made. Check editor diagnostics."));
		}
		// Loading an explicitly approved script can itself run code. Re-resolve
		// everything afterwards instead of retaining possibly freed Node pointers.
		const String recheck = prepare_editor_action(p_call, preview, fingerprint);
		if (!recheck.is_empty() || fingerprint != p_fingerprint) {
			return to_godot(error_json("Script was loaded, but editor/project state changed; attachment was aborted"));
		}
		Node *root = editor->get_edited_scene_root();
		Node *target = root->get_node_or_null(NodePath(to_godot(argument_string(args, "node_path", "."))));
		if (!resource_assignment && !target->is_class(script->get_instance_base_type())) {
			return to_godot(error_json("Script base type does not match the target node"));
		}
		const StringName property = resource_assignment ? StringName(to_godot(argument_string(args, "property"))) : SNAME("script");
		if (resource_assignment) {
			const StringName expected = property == SNAME("texture") ? SNAME("Texture2D") : (target->is_class("CollisionShape2D") ? SNAME("Shape2D") : SNAME("Shape3D"));
			if (!resource->is_class(expected)) {
				return to_godot(error_json("Resource type does not match the target property"));
			}
		}
		EditorUndoRedoManager *undo = EditorUndoRedoManager::get_singleton();
		undo->create_action(TTR("Code Agentics: Assign Resource or Script"), UndoRedo::MERGE_DISABLE, root);
		undo->force_fixed_history();
		record_set_property(undo->get_history_undo_redo(undo->get_history_id_for_object(root)), target, property, resource);
		undo->commit_action();
		return to_godot(ok_json("Resource/script assignment committed to editor undo history. Initialization/runtime effects are not undone; inspect editor errors and save the scene separately."));
	}
	if (p_call.name == "save_current_scene" || p_call.name == "save_scene_as") {
		String target = editor->get_edited_scene_root()->get_scene_file_path();
		if (p_call.name == "save_scene_as") {
			std::string parse_error;
			const auto args = go::parse_json(p_call.arguments, parse_error);
			target = to_godot(go::validate_res_path(argument_string(args, "path"), go::PathOperation::WRITE).normalized);
		}
		String backup;
		if (FileAccess::exists(target)) {
			String text;
			Vector<uint8_t> original;
			std::string read_error;
			if (!read_project_text(target, text, read_error, &original)) {
				return to_godot(error_json(read_error));
			}
			const std::string backup_error = backup_project_bytes(target, original, backup);
			if (!backup_error.empty()) {
				return to_godot(error_json(backup_error));
			}
		}
		const String recheck = prepare_editor_action(p_call, preview, fingerprint);
		if (!recheck.is_empty() || fingerprint != p_fingerprint) {
			return to_godot(error_json("Scene or target changed before saving; request a fresh action"));
		}
		editor->save_scene_as(target, false);
		return to_godot(ok_json("Scene save requested for " + to_std(target) + ". Backup: " + to_std(backup) + ". The editor API does not return the final save error; inspect the editor before treating this as saved."));
	}
	if (p_call.name == "open_scene") {
		std::string parse_error;
		const auto args = go::parse_json(p_call.arguments, parse_error);
		editor->open_scene_from_path(to_godot(argument_string(args, "path")));
	} else if (p_call.name == "run_project") {
		editor->play_main_scene();
	} else if (p_call.name == "run_current_scene") {
		editor->play_current_scene();
	} else {
		editor->stop_playing_scene();
	}
	return to_godot(ok_json(p_call.name + " requested. Completion, imports and script/runtime errors must be checked in the editor; this is not a successful-run assertion."));
}

String GoAgenticsToolBridge::prepare_file_change(const go::GoKiloToolCall &p_call, String &r_preview, String &r_fingerprint) const {
	r_preview = String();
	r_fingerprint = String();
	FileChangePlan plan;
	const std::string error = plan_file_change(p_call, plan);
	if (!error.empty()) {
		return to_godot(error);
	}
	r_fingerprint = plan.fingerprint;
	r_preview = to_godot(p_call.name) + " " + plan.path + "\n\n--- BEFORE ---\n" + plan.before + "\n\n--- AFTER ---\n" + (p_call.name == "delete_file" ? TTR("File will be deleted; a byte-for-byte backup is retained in .godot/go_agent_backups.") : plan.after);
	if (p_call.name == "set_main_scene") {
		r_preview = TTR("This saves the current in-memory project settings, including all other differences shown below. The original configuration is backed up before replacement.") + "\n\n" + r_preview;
	}
	return String();
}

String GoAgenticsToolBridge::execute_file_change(const go::GoKiloToolCall &p_call, const String &p_fingerprint) const {
	FileChangePlan plan;
	const std::string error = plan_file_change(p_call, plan);
	if (!error.empty() || p_fingerprint.is_empty() || plan.fingerprint != p_fingerprint) {
		return to_godot(error_json(error.empty() ? "File or project changed after approval; request a fresh action" : error));
	}
	String backup;
	if (p_call.name == "create_folder") {
		const Error result = DirAccess::make_dir_absolute(plan.path);
		if (result != OK) {
			return to_godot(error_json("Could not create folder"));
		}
	} else {
		if (plan.existed) {
			const std::string backup_error = backup_project_bytes(plan.path, plan.original, backup);
			if (!backup_error.empty()) {
				return to_godot(error_json(backup_error));
			}
		}
		String temporary;
		if (p_call.name != "delete_file") {
			temporary = plan.path + ".go-agent-" + itos(OS::get_singleton()->get_ticks_usec()) + ".tmp";
			if (has_link_component(temporary) || FileAccess::exists(temporary)) {
				return to_godot(error_json("Temporary file collision; no source file changed"));
			}
			Ref<FileAccess> file = FileAccess::open(temporary, FileAccess::WRITE);
			if (file.is_null() || !file->store_string(plan.after)) {
				return to_godot(error_json("Could not write replacement; original file unchanged"));
			}
			file->flush();
			if (file->get_error() != OK) {
				return to_godot(error_json("Could not flush replacement; original file unchanged"));
			}
		}
		FileChangePlan current;
		const std::string changed = plan_file_change(p_call, current);
		if (!changed.empty() || current.fingerprint != p_fingerprint) {
			if (!temporary.is_empty()) {
				DirAccess::remove_absolute(temporary);
			}
			return to_godot(error_json("File/editor state changed before replacement; source not modified"));
		}
		const Error result = p_call.name == "delete_file" ? DirAccess::remove_absolute(plan.path) : DirAccess::rename_absolute(temporary, plan.path);
		if (result != OK) {
			return to_godot(error_json("File operation failed; inspect the original and recovery files. Backup: " + to_std(backup) + "; replacement: " + to_std(temporary)));
		}
	}
	if (p_call.name == "set_main_scene") {
		std::string parse_error;
		const auto args = go::parse_json(p_call.arguments, parse_error);
		const auto checked = go::validate_res_path(argument_string(args, "path"), go::PathOperation::READ);
		ProjectSettings::get_singleton()->set_setting("application/run/main_scene", to_godot(checked.normalized));
	}
	if (EditorFileSystem::get_singleton() != nullptr) {
		EditorFileSystem::get_singleton()->scan();
	}
	return to_godot(ok_json("Applied " + p_call.name + " to " + to_std(plan.path) + (backup.is_empty() ? ". No previous file required a backup." : ". Backup: " + to_std(backup)) + " File changes are not scene Undo/Redo; script parsing/import results are separate."));
}

String GoAgenticsToolBridge::execute(const go::GoKiloToolCall &p_call) const {
	if (GoSuiteRegistry::is_quit_pending() || GoSuiteRegistry::is_editor_input_locked()) {
		return to_godot(error_json("Editor changes are locked by a pending Git/close operation"));
	}
	const std::string tool = p_call.name;
	if (go::GoAgenticsTools::is_file_mutation(tool) || go::GoAgenticsTools::is_editor_action(tool)) {
		return to_godot(error_json("File mutations and editor execution require a prepared approval snapshot"));
	}
	const std::string argument_error = go::GoAgenticsTools::validate_arguments(tool, p_call.arguments);
	if (!argument_error.empty()) {
		return to_godot(error_json(argument_error));
	}
	if (!go::GoAgenticsTools::is_tool_allowed(tool)) {
		return to_godot(error_json("tool is not whitelisted"));
	}

	std::string parse_error;
	const go::JsonValue arguments = go::parse_json(p_call.arguments.empty() ? "{}" : p_call.arguments, parse_error);
	if (!parse_error.empty() || arguments.type != go::JsonValue::Type::OBJECT) {
		return to_godot(error_json("invalid tool arguments: " + parse_error));
	}

	const std::string raw_path = argument_string(arguments, "path");
	const bool write_tool = go::GoAgenticsTools::is_write_tool(tool);
	const bool delete_tool = tool == "delete_file";
	go::PathOperation operation = go::PathOperation::READ;
	if (delete_tool) {
		operation = go::PathOperation::DELETE;
	} else if (write_tool) {
		operation = go::PathOperation::WRITE;
	}
	go::PathValidationResult path_result;
	String path;
	bool has_path = !raw_path.empty();
	if (has_path) {
		path_result = go::validate_res_path(raw_path, operation);
		if (!path_result.ok) {
			return to_godot(error_json(path_result.error));
		}
		path = to_godot(path_result.normalized);
		if (has_link_component(path)) {
			return to_godot(error_json("symbolic links are not allowed for agent tools"));
		}
	} else if (tool == "list_files" || tool == "search_text" || tool == "scan_filesystem") {
		path = "res://";
		has_path = true;
	}

	if (tool == "list_files" || tool == "search_text") {
		Vector<String> files;
		if (DirAccess::dir_exists_absolute(path)) {
			collect_files(path, files);
		} else if (tool == "search_text" && FileAccess::exists(path)) {
			files.push_back(path);
		} else {
			return to_godot(error_json("A readable project directory or search file is required"));
		}
		files.sort();
		std::string output;
		if (tool == "list_files") {
			for (const String &file : files) {
				const std::string line = to_std(file) + "\n";
				if (output.size() + line.size() > 48000) {
					break;
				}
				output += line;
			}
			return to_godot(ok_json(output + "[Bounded listing: up to 1000 files, depth 8, 2000 directory entries; hidden/sensitive/link paths omitted.]"));
		}
		const String query = to_godot(argument_string(arguments, "query"));
		int examined = 0;
		int matches = 0;
		for (const String &file : files) {
			if (++examined > 64 || matches >= 50 || output.size() >= 48000) {
				break;
			}
			String text;
			std::string error;
			if (!read_project_text(file, text, error)) {
				continue;
			}
			const PackedStringArray lines = text.split("\n");
			for (int i = 0; i < lines.size() && matches < 50; ++i) {
				if (lines[i].contains(query)) {
					const std::string line = to_std(file) + ":" + std::to_string(i + 1) + ": " + to_std(lines[i].substr(0, 500)) + "\n";
					if (output.size() + line.size() > 48000) {
						break;
					}
					output += line;
					++matches;
				}
			}
		}
		return to_godot(ok_json(output + "[Bounded search: first 64 candidate files, 50 matches; unreadable, oversized, binary and sensitive files omitted. Not an exhaustive search.]"));
	}

	if (tool == "read_file") {
		String text;
		std::string error;
		if (!read_project_text(path, text, error)) {
			return to_godot(error_json(error));
		}
		const auto *start_value = arguments.get("start_line");
		const auto *limit_value = arguments.get("line_limit");
		const int start = start_value != nullptr ? int(start_value->as_number()) : 1;
		const int limit = limit_value != nullptr ? int(limit_value->as_number()) : 200;
		if (start < 1 || limit < 1 || limit > 400) {
			return to_godot(error_json("start_line must be positive and line_limit must be between 1 and 400"));
		}
		const PackedStringArray lines = text.split("\n");
		std::string output;
		for (int i = start - 1; i < lines.size() && i < start - 1 + limit; ++i) {
			const std::string line = to_std(lines[i]) + "\n";
			if (output.size() + line.size() > 48000) {
				output += "[Output byte limit reached; remaining lines omitted.]";
				break;
			}
			output += line;
		}
		return to_godot(ok_json(output));
	}

	if (tool == "validate_script") {
		if (!has_path || path.get_extension().to_lower() != "gd") {
			return to_godot(error_json("validate_script requires a res:// .gd file"));
		}
		String source;
		std::string error;
		if (!read_project_text(path, source, error)) {
			return to_godot(error_json(error));
		}
		// Parse the saved source only. Do NOT run the analyzer/compiler or use
		// ResourceLoader: validation must not initialize scripts/dependencies.
		GDScriptParser parser;
		const Error result = parser.parse(source, path, false);
		if (result != OK) {
			std::string diagnostics = "GDScript syntax errors in " + to_std(path) + ":\n";
			int count = 0;
			for (const GDScriptParser::ParserError &entry : parser.get_errors()) {
				if (++count > 20) {
					diagnostics += "Additional diagnostics omitted.\n";
					break;
				}
				diagnostics += std::to_string(entry.start_line) + ":" + std::to_string(entry.start_column) + " " + to_std(entry.message.substr(0, 500)) + "\n";
			}
			return to_godot(error_json(diagnostics));
		}
		return to_godot(ok_json("Saved source passed GDScript syntax parsing only. Semantic/type analysis, dependencies, unsaved editor buffers and runtime behavior were NOT validated."));
	}

	if (tool == "get_project_setting") {
		const std::string name = argument_string(arguments, "name");
		if (name != "application/run/main_scene" && name != "application/config/name" && name != "application/boot_splash/image" && name != "rendering/renderer/rendering_method") {
			return to_godot(error_json("Project setting is outside the readable allowlist"));
		}
		ProjectSettings *settings = ProjectSettings::get_singleton();
		if (!settings->has_setting(to_godot(name))) {
			return to_godot(error_json("Project setting is not configured"));
		}
		const Variant value = settings->get_setting(to_godot(name));
		if (value.get_type() != Variant::STRING && value.get_type() != Variant::STRING_NAME) {
			return to_godot(error_json("Project setting is not a string"));
		}
		const std::string text = to_std(String(value));
		if (text.size() > 4096 || go::looks_like_secret_content(text)) {
			return to_godot(error_json("Project setting omitted because it is oversized or potentially sensitive"));
		}
		return to_godot(ok_json(name + "=" + text));
	}

	EditorInterface *editor = EditorInterface::get_singleton();
	if (editor == nullptr) {
		return to_godot(error_json("the Godot editor is not available for this tool"));
	}

	if (tool == "scan_filesystem") {
		if (EditorFileSystem *filesystem = editor->get_resource_filesystem()) {
			filesystem->scan();
			return to_godot(ok_json("editor filesystem scan requested"));
		}
		return to_godot(error_json("editor filesystem is not available"));
	}

	if (tool == "get_current_scene_tree" || tool == "get_editor_selection" || tool == "select_node" || tool == "add_node" || tool == "move_node" || tool == "remove_node" || tool == "rename_node" || tool == "set_node_property" || tool == "get_node_property" || tool == "attach_script") {
		Node *root = editor->get_edited_scene_root();
		if (tool == "get_current_scene_tree") {
			int budget = MAX_FILES;
			return root == nullptr ? to_godot(error_json("no scene is open")) : to_godot(ok_json(describe_node(root, 0, budget).substr(0, MAX_TEXT_CHARS)));
		}
		if (tool == "get_editor_selection") {
			std::ostringstream out;
			TypedArray<Node> selected = editor->get_selection()->get_selected_nodes();
			for (int i = 0; i < selected.size(); ++i) {
				Node *node = Object::cast_to<Node>(selected[i]);
				if (node != nullptr) {
					out << to_std(String(node->get_path())) << " [" << to_std(node->get_class()) << "]\n";
				}
			}
			return to_godot(ok_json(out.str()));
		}
		if (root == nullptr) {
			return to_godot(error_json("no scene is open"));
		}
		const std::string node_path = argument_string(arguments, "node_path", ".");
		if (node_path.empty() || node_path[0] == '/' || node_path.find("..") != std::string::npos || node_path.find(':') != std::string::npos) {
			return to_godot(error_json("node path must stay inside the edited scene"));
		}
		Node *node = root->get_node_or_null(NodePath(to_godot(node_path)));
		if (node == nullptr) {
			return to_godot(error_json("node not found: " + node_path));
		}
		if (tool == "select_node") {
			editor->get_selection()->clear();
			editor->get_selection()->add_node(node);
			editor->get_selection()->update();
			return to_godot(ok_json("selected " + to_std(String(node->get_path()))));
		}
		if (tool == "remove_node" || tool == "move_node" || tool == "set_node_property" || tool == "get_node_property") {
			if (root->get_scene_inherited_state().is_valid() || !editable_subtree(node, root)) {
				return to_godot(error_json("Only bounded, locally owned, non-scripted, non-instanced subtrees may be changed"));
			}
			for (Node *ancestor = node; ancestor != nullptr; ancestor = ancestor == root ? nullptr : ancestor->get_parent()) {
				if (!ancestor->get_script().is_null()) {
					return to_godot(error_json("Scripted ancestors are not supported by scene tools"));
				}
			}
			if (tool != "get_node_property" && EditorUndoRedoManager::get_singleton() == nullptr) {
				return to_godot(error_json("Editor undo history is unavailable"));
			}
		}
		if (tool == "add_node" || tool == "rename_node") {
			if (node != root && node->get_owner() != root) {
				return to_godot(error_json("Only nodes owned by the edited scene may be changed"));
			}
			for (Node *ancestor = node; ancestor != nullptr; ancestor = ancestor == root ? nullptr : ancestor->get_parent()) {
				if (!ancestor->get_script().is_null()) {
					return to_godot(error_json("Scene tools do not modify scripted nodes or their descendants"));
				}
			}
			const String name = to_godot(argument_string(arguments, "name"));
			if (name.is_empty() || name != name.validate_node_name() || name.length() > 128 || name.begins_with("@")) {
				return to_godot(error_json("A valid unique node name is required"));
			}
			Node *parent = tool == "add_node" ? node : node->get_parent();
			if (parent != nullptr && parent->has_node(NodePath(name)) && parent->get_node(NodePath(name)) != node) {
				return to_godot(error_json("A sibling already has that name"));
			}
			if (EditorUndoRedoManager::get_singleton() == nullptr) {
				return to_godot(error_json("Editor undo history is unavailable"));
			}
		}
		if (tool == "add_node") {
			const std::string type = argument_string(arguments, "type", "Node");
			const std::string name = argument_string(arguments, "name", "Node");
			Node *child = create_allowed_node(to_godot(type));
			if (child == nullptr) {
				return to_godot(error_json("Node type is outside the built-in allowlist or unavailable in this build"));
			}
			child->set_name(to_godot(name));
			EditorUndoRedoManager *undo = EditorUndoRedoManager::get_singleton();
			undo->create_action(TTR("Code Agentics: Add Node"), UndoRedo::MERGE_DISABLE, root);
			undo->force_fixed_history();
			record_add_node(undo->get_history_undo_redo(undo->get_history_id_for_object(root)), node, child, root);
			undo->commit_action();
			return to_godot(ok_json("added " + type + " at " + to_std(String(child->get_path()))));
		}
		if (tool == "remove_node") {
			if (node == root) {
				return to_godot(error_json("the scene root cannot be removed"));
			}
			EditorUndoRedoManager *undo = EditorUndoRedoManager::get_singleton();
			undo->create_action(TTR("Code Agentics: Remove Node"), UndoRedo::MERGE_DISABLE, root);
			undo->force_fixed_history();
			record_remove_node(undo->get_history_undo_redo(undo->get_history_id_for_object(root)), node);
			undo->commit_action();
			return to_godot(ok_json("removed node (undo available)"));
		}
		if (tool == "move_node") {
			Node *parent = root->get_node_or_null(NodePath(to_godot(argument_string(arguments, "parent_path"))));
			if (node == root || parent == nullptr || parent == node || node->is_ancestor_of(parent) || (parent != root && parent->get_owner() != root)) {
				return to_godot(error_json("Invalid reparent destination or scene root move"));
			}
			for (Node *ancestor = parent; ancestor != nullptr; ancestor = ancestor == root ? nullptr : ancestor->get_parent()) {
				if (!ancestor->get_script().is_null() || (ancestor != root && !ancestor->get_scene_file_path().is_empty())) {
					return to_godot(error_json("Cannot move into scripted or instanced nodes"));
				}
			}
			Node *sibling = parent->get_node_or_null(NodePath(String(node->get_name())));
			if (sibling != nullptr && sibling != node) {
				return to_godot(error_json("Destination already contains a node with that name"));
			}
			const int last_index = parent->get_child_count() - (parent == node->get_parent() ? 1 : 0);
			const auto *index_value = arguments.get("index");
			const int index = index_value == nullptr ? last_index : int(index_value->as_number());
			if (index < 0 || index > last_index) {
				return to_godot(error_json("Child index is out of range"));
			}
			EditorUndoRedoManager *undo = EditorUndoRedoManager::get_singleton();
			undo->create_action(TTR("Code Agentics: Move Node"), UndoRedo::MERGE_DISABLE, root);
			undo->force_fixed_history();
			record_move_node(undo->get_history_undo_redo(undo->get_history_id_for_object(root)), node, parent, index);
			undo->commit_action();
			return to_godot(ok_json("moved node; local transform preserved (undo available)"));
		}

		if (tool == "rename_node") {
			const std::string name = argument_string(arguments, "name");
			if (name.empty()) {
				return to_godot(error_json("rename_node requires a name"));
			}
			EditorUndoRedoManager *undo = EditorUndoRedoManager::get_singleton();
			undo->create_action(TTR("Code Agentics: Rename Node"), UndoRedo::MERGE_DISABLE, root);
			undo->force_fixed_history();
			record_rename_node(undo->get_history_undo_redo(undo->get_history_id_for_object(root)), node, to_godot(name));
			undo->commit_action();
			return to_godot(ok_json("renamed node to " + name));
		}
		if (tool == "set_node_property" || tool == "get_node_property") {
			const StringName property(to_godot(argument_string(arguments, "property")));
			bool valid = false;
			const Variant current = node->get(property, &valid);
			Variant value;
			const std::string error = validate_scalar_property(node, property, tool == "set_node_property" ? json_to_variant(arguments.get("value")) : current, value);
			if (!valid || !error.empty()) {
				return to_godot(error_json(error.empty() ? "Unknown property" : error));
			}
			if (tool == "get_node_property") {
				return to_godot(ok_json(to_std(current.stringify())));
			}
			EditorUndoRedoManager *undo = EditorUndoRedoManager::get_singleton();
			undo->create_action(TTR("Code Agentics: Set Node Property"), UndoRedo::MERGE_DISABLE, root);
			undo->force_fixed_history();
			record_set_property(undo->get_history_undo_redo(undo->get_history_id_for_object(root)), node, property, value);
			undo->commit_action();
			return to_godot(ok_json("property changed (undo available)"));
		}
	}

	if (tool == "git_status") {
		go_suite::GoGitController controller;
		controller.set_project(ProjectSettings::get_singleton()->globalize_path("res://"));
		std::string error;
		const go_suite::GoGitSnapshot snapshot = controller.snapshot(error);
		if (!snapshot.is_repo) {
			return to_godot(error_json("this project is not a Git repository"));
		}
		std::ostringstream out;
		out << "branch=" << snapshot.branch << " ahead=" << snapshot.status.ahead << " behind=" << snapshot.status.behind
			<< " changes=" << snapshot.status.entries.size() << " lfs_files=" << snapshot.lfs.files.size();
		return to_godot(ok_json(out.str()));
	}

	if (tool == "auto_commit") {
		return to_godot(GoSuiteRegistry::request_auto_commit() ? ok_json("Automatic Commit was requested in the Git Suite") : error_json("Automatic Commit is not ready or is already running"));
	}

	if (tool == "get_editor_log") {
		return to_godot(error_json("editor log access is best-effort and not available in this build"));
	}

	return to_godot(error_json("tool is whitelisted but not implemented in this build: " + tool));
}
