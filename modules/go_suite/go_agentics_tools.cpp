// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT

#include "go_agentics_tools.h"

#include "go_agentics_path.h"
#include "go_kilo_models.h"
#include "go_suite_core.h"

#include <algorithm>
#include <sstream>

namespace go {

namespace {

const std::vector<std::string> TOOL_NAMES = {
	"list_files", "read_file", "write_file", "edit_file", "create_folder", "delete_file",
	"search_text", "create_script", "validate_script", "attach_script", "create_scene",
	"open_scene", "save_current_scene", "save_scene_as", "set_main_scene", "get_current_scene_tree",
	"add_node", "remove_node", "rename_node", "set_node_property", "get_node_property",
	"scan_filesystem", "get_editor_selection", "select_node", "run_project", "run_current_scene",
	"stop_project", "get_editor_log", "git_status", "auto_commit",
};

bool is_write_tool(const std::string &p_name) {
	static const char *write_tools[] = {
		"write_file", "edit_file", "create_folder", "delete_file", "create_script",
		"attach_script", "create_scene", "save_current_scene", "save_scene_as",
		"set_main_scene", "add_node", "remove_node", "rename_node", "set_node_property",
		"auto_commit",
	};
	for (const char *tool : write_tools) {
		if (p_name == tool) {
			return true;
		}
	}
	return false;
}

} // namespace

std::vector<std::string> GoAgenticsTools::allowed_tool_names() {
	return TOOL_NAMES;
}

bool GoAgenticsTools::is_tool_allowed(const std::string &p_name) {
	return std::find(TOOL_NAMES.begin(), TOOL_NAMES.end(), p_name) != TOOL_NAMES.end();
}

std::string GoAgenticsTools::tool_definitions_json() {
	std::ostringstream stream;
	stream << "[";
	for (size_t i = 0; i < TOOL_NAMES.size(); ++i) {
		if (i > 0) {
			stream << ",";
		}
		const std::string description = std::string("Whitelisted GO. Godot editor tool: ") + TOOL_NAMES[i];
		stream << "{\"type\":\"function\",\"function\":{"
			   << "\"name\":" << escape_json_string(TOOL_NAMES[i]) << ","
			   << "\"description\":" << escape_json_string(description) << ","
			   << "\"parameters\":{\"type\":\"object\",\"properties\":{\"path\":{\"type\":\"string\",\"description\":\"res:// project path\"}},\"required\":[]}"
			   << "}}";
	}
	stream << "]";
	return stream.str();
}

std::string GoAgenticsTools::validate_tool_path(const std::string &p_path, bool p_for_write) {
	const PathValidationResult result = validate_res_path(p_path, p_for_write ? PathOperation::WRITE : PathOperation::READ);
	if (!result.ok) {
		return std::string("{\"ok\":false,\"error\":") + escape_json_string(result.error) + "}";
	}
	return std::string("{\"ok\":true,\"path\":") + escape_json_string(result.normalized) + "}";
}

std::string GoAgenticsTools::execute_tool(const std::string &p_name, const std::string &p_arguments_json) {
	std::ostringstream result;
	if (!is_tool_allowed(p_name)) {
		return "{\"ok\":false,\"error\":\"tool is not whitelisted\"}";
	}
	std::string parse_error;
	const JsonValue arguments = parse_json(p_arguments_json, parse_error);
	if (!parse_error.empty()) {
		return std::string("{\"ok\":false,\"error\":") + escape_json_string(parse_error) + "}";
	}
	const JsonValue *path_value = arguments.get("path");
	if (path_value != nullptr) {
		const std::string path_validation = validate_tool_path(path_value->as_string(), is_write_tool(p_name));
		if (contains(path_validation, "\"ok\":false")) {
			return path_validation;
		}
	}
	result << "{\"ok\":false,\"error\":\"tool execution is not wired to the Godot editor yet\",\"tool\":"
		   << escape_json_string(p_name) << ",\"path_validated\":true}";
	return result.str();
}

} // namespace go
