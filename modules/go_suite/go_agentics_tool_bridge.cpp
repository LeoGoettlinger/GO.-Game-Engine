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
#include "core/object/class_db.h"
#include "core/object/script_language.h"
#include "core/os/os.h"
#include "core/string/node_path.h"
#include "core/variant/variant.h"
#include "editor/editor_data.h"
#include "editor/editor_interface.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/script/script_editor_plugin.h"
#include "scene/2d/node_2d.h"
#include "scene/main/node.h"
#include "scene/resources/packed_scene.h"

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
	if (EditorInterface::get_singleton() != nullptr) {
		for (const String &scene : EditorInterface::get_singleton()->get_open_scenes()) {
			if (scene == p_path) {
				return true;
			}
		}
	}
	return false;
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
		if (extension != "gd" && extension != "txt" && extension != "md" && extension != "json" && extension != "csv") {
			return "File mutations currently support only gd, txt, md, json and csv; use scene/resource tools for other types";
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
		if (p_call.name == "edit_file") {
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
	return String();
}

String GoAgenticsToolBridge::execute_file_change(const go::GoKiloToolCall &p_call, const String &p_fingerprint) const {
	FileChangePlan plan;
	const std::string error = plan_file_change(p_call, plan);
	if (!error.empty() || p_fingerprint.is_empty() || plan.fingerprint != p_fingerprint) {
		return to_godot(error_json(error.empty() ? "File or project changed after approval; request a fresh action" : error));
	}
	if (p_call.name == "create_folder") {
		const Error result = DirAccess::make_dir_absolute(plan.path);
		if (result != OK) {
			return to_godot(error_json("Could not create folder"));
		}
	} else {
		String backup;
		if (plan.existed) {
			const String folder = "res://.godot/go_agent_backups";
			if (has_link_component(folder) || DirAccess::make_dir_recursive_absolute(folder) != OK) {
				return to_godot(error_json("Cannot create a safe local backup directory"));
			}
			Ref<DirAccess> directory = DirAccess::open(folder);
			if (directory.is_null() || directory->list_dir_begin() != OK) {
				return to_godot(error_json("Cannot inspect local backups"));
			}
			int count = 0;
			while (!directory->get_next().is_empty() && count < 200) {
				++count;
			}
			directory->list_dir_end();
			if (count >= 200) {
				return to_godot(error_json("Backup limit reached; review .godot/go_agent_backups before more file changes"));
			}
			backup = folder.path_join(itos(OS::get_singleton()->get_ticks_usec()) + "-" + plan.path.get_file() + ".bak");
			if (has_link_component(backup) || FileAccess::exists(backup)) {
				return to_godot(error_json("Backup collision; no source file changed"));
			}
			Ref<FileAccess> file = FileAccess::open(backup, FileAccess::WRITE);
			if (file.is_null() || !file->store_buffer(plan.original)) {
				return to_godot(error_json("Could not write backup; no source file changed"));
			}
			file->flush();
			if (file->get_error() != OK) {
				return to_godot(error_json("Could not flush backup; no source file changed"));
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
	if (EditorFileSystem::get_singleton() != nullptr) {
		EditorFileSystem::get_singleton()->scan();
	}
	return to_godot(ok_json("Applied " + p_call.name + " to " + to_std(plan.path) + ". Existing bytes were backed up locally. File changes are not scene Undo/Redo; script parsing/import results are separate."));
}

String GoAgenticsToolBridge::execute(const go::GoKiloToolCall &p_call) const {
	if (GoSuiteRegistry::is_quit_pending() || GoSuiteRegistry::is_editor_input_locked()) {
		return to_godot(error_json("Editor changes are locked by a pending Git/close operation"));
	}
	const std::string tool = p_call.name;
	if (go::GoAgenticsTools::is_file_mutation(tool)) {
		return to_godot(error_json("File mutations require a prepared approval snapshot"));
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
		if (!has_path) {
			return to_godot(error_json("a res:// script path is required"));
		}
		Ref<Resource> resource = ResourceLoader::load(path, "Script", ResourceFormatLoader::CACHE_MODE_IGNORE);
		Ref<Script> script = Object::cast_to<Script>(resource.ptr());
		if (script.is_null()) {
			return to_godot(error_json("script did not parse as a Godot Script resource"));
		}
		return to_godot(ok_json("script is valid: " + to_std(path)));
	}

	EditorInterface *editor = EditorInterface::get_singleton();
	if (editor == nullptr) {
		return to_godot(error_json("the Godot editor is not available for this tool"));
	}

	if (tool == "create_scene") {
		if (!has_path || !path.ends_with(".tscn")) {
			return to_godot(error_json("create_scene requires a res://.tscn path"));
		}
		Node2D *root = memnew(Node2D);
		root->set_name("Node2D");
		Ref<PackedScene> scene;
		scene.instantiate();
		const Error packed = scene->pack(root);
		memdelete(root);
		if (packed != OK || ResourceSaver::save(scene, path) != OK) {
			return to_godot(error_json("could not save scene"));
		}
		editor->open_scene_from_path(path);
		return to_godot(ok_json("created scene " + to_std(path)));
	}

	if (tool == "open_scene") {
		if (!has_path || (!path.ends_with(".tscn") && !path.ends_with(".scn"))) {
			return to_godot(error_json("open_scene requires a res:// scene path"));
		}
		editor->open_scene_from_path(path);
		return to_godot(ok_json("opened scene " + to_std(path)));
	}

	if (tool == "save_current_scene") {
		return to_godot(editor->save_scene() == OK ? ok_json("saved current scene") : error_json("could not save current scene"));
	}

	if (tool == "save_scene_as") {
		if (!has_path) {
			return to_godot(error_json("a res:// scene path is required"));
		}
		editor->save_scene_as(path);
		return to_godot(ok_json("saved scene as " + to_std(path)));
	}

	if (tool == "set_main_scene") {
		if (!has_path) {
			return to_godot(error_json("a res:// scene path is required"));
		}
		ProjectSettings::get_singleton()->set_setting("application/run/main_scene", path);
		ProjectSettings::get_singleton()->save();
		return to_godot(ok_json("main scene set to " + to_std(path)));
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
			if (type != "Node" && type != "Node2D" && type != "Node3D" && type != "Control" && type != "Sprite2D" && type != "Label") {
				return to_godot(error_json("Only basic built-in node types are enabled"));
			}
			Object *instance = ClassDB::instantiate(StringName(to_godot(type)));
			Node *child = Object::cast_to<Node>(instance);
			if (child == nullptr) {
				if (instance != nullptr) {
					memdelete(instance);
				}
				return to_godot(error_json("node type is not a Node"));
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
		if (tool == "attach_script") {
			const std::string script_path = argument_string(arguments, "script_path");
			if (script_path.empty()) {
				return to_godot(error_json("attach_script requires script_path"));
			}
			const go::PathValidationResult script_result = go::validate_res_path(script_path, go::PathOperation::READ);
			if (!script_result.ok || has_link_component(to_godot(script_result.normalized))) {
				return to_godot(error_json(script_result.error));
			}
			Ref<Resource> resource = ResourceLoader::load(to_godot(script_result.normalized), "Script", ResourceFormatLoader::CACHE_MODE_IGNORE);
			Ref<Script> script = Object::cast_to<Script>(resource.ptr());
			if (script.is_null()) {
				return to_godot(error_json("script_path did not load as a Script"));
			}
			node->set_script(Variant(script));
			return to_godot(ok_json("attached script " + script_path));
		}
	}

	if (tool == "run_project" || tool == "run_current_scene" || tool == "stop_project") {
		if (tool == "run_project") {
			editor->play_main_scene();
		} else if (tool == "run_current_scene") {
			editor->play_current_scene();
		} else {
			editor->stop_playing_scene();
		}
		return to_godot(ok_json(tool + " requested"));
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
