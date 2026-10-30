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
#include "core/string/node_path.h"
#include "core/variant/variant.h"
#include "editor/editor_data.h"
#include "editor/editor_interface.h"
#include "editor/file_system/editor_file_system.h"
#include "scene/2d/node_2d.h"
#include "scene/main/node.h"
#include "scene/resources/packed_scene.h"

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

std::string clip(const String &p_text) {
	const String clipped = p_text.substr(0, MAX_TEXT_CHARS);
	return to_std(clipped);
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

void collect_files(const String &p_directory, Vector<String> &r_files, int p_depth = 0) {
	if (p_depth > MAX_DEPTH || r_files.size() >= MAX_FILES) {
		return;
	}
	Ref<DirAccess> directory = DirAccess::open(p_directory);
	if (directory.is_null()) {
		return;
	}
	directory->list_dir_begin();
	String name = directory->get_next();
	while (!name.is_empty() && r_files.size() < MAX_FILES) {
		if (!directory->current_is_hidden() && name != ".godot" && !directory->is_link(name)) {
			const String full = p_directory.path_join(name);
			if (directory->current_is_dir()) {
				collect_files(full, r_files, p_depth + 1);
			} else {
				r_files.push_back(full);
			}
		}
		name = directory->get_next();
	}
	directory->list_dir_end();
}

std::string describe_node(const Node *p_node, int p_depth) {
	if (p_node == nullptr || p_depth > 10) {
		return "";
	}
	std::ostringstream out;
	out << std::string(p_depth * 2, ' ') << to_std(p_node->get_name()) << " [" << to_std(p_node->get_class()) << "]\n";
	for (int i = 0; i < p_node->get_child_count(); ++i) {
		out << describe_node(p_node->get_child(i), p_depth + 1);
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

} // namespace

String GoAgenticsToolBridge::execute(const go::GoKiloToolCall &p_call) const {
	const std::string tool = p_call.name;
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

	if (tool == "list_files") {
		Vector<String> files;
		collect_files(path, files);
		std::ostringstream out;
		for (const String &file : files) {
			out << to_std(file) << "\n";
		}
		return to_godot(ok_json(out.str()));
	}

	if (tool == "search_text" && path == "res://") {
		const std::string query = argument_string(arguments, "query");
		if (query.empty()) {
			return to_godot(error_json("search_text requires a query"));
		}
		Vector<String> files;
		collect_files(path, files);
		std::ostringstream out;
		int matches = 0;
		for (const String &candidate : files) {
			if (matches >= 50) {
				break;
			}
			const Ref<FileAccess> candidate_file = FileAccess::open(candidate, FileAccess::READ);
			if (candidate_file.is_null()) {
				continue;
			}
			const PackedStringArray lines = candidate_file->get_as_text().split("\n");
			for (int i = 0; i < lines.size() && matches < 50; ++i) {
				if (to_std(lines[i]).find(query) != std::string::npos) {
					out << to_std(candidate) << ":" << (i + 1) << ": " << to_std(lines[i]) << "\n";
					matches++;
				}
			}
		}
		return to_godot(ok_json(out.str()));
	}

	if (tool == "read_file" || tool == "search_text") {
		if (!has_path || path == "res://" || !FileAccess::exists(path)) {
			return to_godot(error_json("a valid res:// file path is required"));
		}
		const Ref<FileAccess> file = FileAccess::open(path, FileAccess::READ);
		if (file.is_null()) {
			return to_godot(error_json("could not open file"));
		}
		const String text = file->get_as_text();
		if (tool == "read_file") {
			return to_godot(ok_json(clip(text)));
		}
		const std::string query = argument_string(arguments, "query");
		if (query.empty()) {
			return to_godot(error_json("search_text requires a query"));
		}
		const PackedStringArray lines = text.split("\n");
		std::ostringstream out;
		int matches = 0;
		for (int i = 0; i < lines.size() && matches < 50; ++i) {
			if (to_std(lines[i]).find(query) != std::string::npos) {
				out << (i + 1) << ": " << to_std(lines[i]) << "\n";
				matches++;
			}
		}
		return to_godot(ok_json(out.str()));
	}

	if (tool == "write_file" || tool == "create_script") {
		if (!has_path) {
			return to_godot(error_json("a res:// path is required"));
		}
		if (tool == "create_script" && !path.ends_with(".gd")) {
			return to_godot(error_json("create_script requires a .gd path"));
		}
		const std::string content = argument_string(arguments, "content");
		if (content.size() > 200000) {
			return to_godot(error_json("content exceeds the write limit"));
		}
		const Ref<FileAccess> file = FileAccess::open(path, FileAccess::WRITE);
		if (file.is_null()) {
			return to_godot(error_json("could not open file for writing"));
		}
		file->store_string(to_godot(content.substr(0, 200000)));
		return to_godot(ok_json("wrote " + to_std(path)));
	}

	if (tool == "edit_file") {
		if (!has_path || !FileAccess::exists(path)) {
			return to_godot(error_json("a valid res:// file path is required"));
		}
		const std::string old_text = argument_string(arguments, "old_text");
		const std::string new_text = argument_string(arguments, "new_text");
		if (old_text.empty()) {
			return to_godot(error_json("edit_file requires old_text"));
		}
		const Ref<FileAccess> file = FileAccess::open(path, FileAccess::READ);
		if (file.is_null()) {
			return to_godot(error_json("could not open file"));
		}
		std::string text = to_std(file->get_as_text());
		const size_t found = text.find(old_text);
		if (found == std::string::npos || text.find(old_text, found + old_text.size()) != std::string::npos) {
			return to_godot(error_json("old_text must match exactly once"));
		}
		text.replace(found, old_text.size(), new_text);
		if (text.size() > 200000) {
			return to_godot(error_json("edited file exceeds the write limit"));
		}
		const Ref<FileAccess> output = FileAccess::open(path, FileAccess::WRITE);
		if (output.is_null()) {
			return to_godot(error_json("could not open file for writing"));
		}
		output->store_string(to_godot(text.substr(0, 200000)));
		return to_godot(ok_json("edited " + to_std(path)));
	}

	if (tool == "create_folder") {
		if (!has_path) {
			return to_godot(error_json("a res:// folder path is required"));
		}
		Ref<DirAccess> directory = DirAccess::create(DirAccess::ACCESS_RESOURCES);
		const String relative_path = to_godot(path_result.relative);
		if (directory.is_null() || directory->make_dir_recursive(relative_path) != OK) {
			return to_godot(error_json("could not create folder"));
		}
		return to_godot(ok_json("created folder " + to_std(path)));
	}

	if (tool == "delete_file") {
		if (!has_path) {
			return to_godot(error_json("a res:// path is required"));
		}
		Ref<DirAccess> directory = DirAccess::create(DirAccess::ACCESS_RESOURCES);
		const String relative_path = to_godot(path_result.relative);
		if (directory.is_null() || directory->remove(relative_path) != OK) {
			return to_godot(error_json("could not delete file"));
		}
		return to_godot(ok_json("deleted " + to_std(path)));
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

	if (tool == "get_current_scene_tree" || tool == "get_editor_selection" || tool == "select_node" || tool == "add_node" || tool == "remove_node" || tool == "rename_node" || tool == "set_node_property" || tool == "get_node_property" || tool == "attach_script") {
		Node *root = editor->get_edited_scene_root();
		if (tool == "get_current_scene_tree") {
			return root == nullptr ? to_godot(error_json("no scene is open")) : to_godot(ok_json(describe_node(root, 0)));
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
		if (tool == "add_node") {
			const std::string type = argument_string(arguments, "type", "Node");
			const std::string name = argument_string(arguments, "name", "Node");
			Object *instance = ClassDB::instantiate(StringName(to_godot(type)));
			Node *child = Object::cast_to<Node>(instance);
			if (child == nullptr) {
				if (instance != nullptr) {
					memdelete(instance);
				}
				return to_godot(error_json("node type is not a Node"));
			}
			child->set_name(to_godot(name));
			node->add_child(child);
			child->set_owner(root);
			return to_godot(ok_json("added " + type + " at " + to_std(String(child->get_path()))));
		}
		if (tool == "remove_node") {
			if (node == root) {
				return to_godot(error_json("the scene root cannot be removed"));
			}
			Node *parent = node->get_parent();
			parent->remove_child(node);
			memdelete(node);
			return to_godot(ok_json("removed node"));
		}
		if (tool == "rename_node") {
			const std::string name = argument_string(arguments, "name");
			if (name.empty()) {
				return to_godot(error_json("rename_node requires a name"));
			}
			node->set_name(to_godot(name));
			return to_godot(ok_json("renamed node to " + name));
		}
		if (tool == "set_node_property") {
			const std::string property = argument_string(arguments, "property");
			if (property.empty()) {
				return to_godot(error_json("set_node_property requires a property name"));
			}
			bool valid = false;
			node->set(StringName(to_godot(property)), json_to_variant(arguments.get("value")), &valid);
			return to_godot(valid ? ok_json("set " + property) : error_json("property could not be set: " + property));
		}
		if (tool == "get_node_property") {
			const std::string property = argument_string(arguments, "property");
			if (property.empty()) {
				return to_godot(error_json("get_node_property requires a property name"));
			}
			bool valid = false;
			const Variant value = node->get(StringName(to_godot(property)), &valid);
			return to_godot(valid ? ok_json(to_std(value.stringify())) : error_json("property could not be read: " + property));
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
