/**************************************************************************/
/*  go_agentics_tools.cpp                                                 */
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

#include "go_agentics_tools.h"

#include "go_agentics_path.h"
#include "go_kilo_models.h"
#include "go_suite_core.h"

#include <algorithm>
#include <sstream>

namespace go {

namespace {

const std::vector<std::string> TOOL_NAMES = {
	"list_files",
	"read_file",
	"write_file",
	"edit_file",
	"create_folder",
	"delete_file",
	"search_text",
	"create_script",
	"validate_script",
	"attach_script",
	"create_scene",
	"open_scene",
	"save_current_scene",
	"save_scene_as",
	"set_main_scene",
	"get_current_scene_tree",
	"add_node",
	"remove_node",
	"rename_node",
	"set_node_property",
	"get_node_property",
	"scan_filesystem",
	"get_editor_selection",
	"select_node",
	"run_project",
	"run_current_scene",
	"stop_project",
	"get_editor_log",
	"git_status",
	"auto_commit",
};

struct ToolArgument {
	std::string name;
	bool required = false;
	bool scalar = false;
};

std::vector<ToolArgument> tool_arguments(const std::string &p_name) {
	std::vector<ToolArgument> arguments;
	if (p_name == "list_files" || p_name == "search_text") {
		arguments.push_back({ "path", false, false });
	} else if (p_name == "read_file" || p_name == "write_file" || p_name == "edit_file" || p_name == "create_folder" || p_name == "delete_file" || p_name == "create_script" || p_name == "validate_script" || p_name == "create_scene" || p_name == "open_scene" || p_name == "save_scene_as" || p_name == "set_main_scene") {
		arguments.push_back({ "path", true, false });
	}
	if (p_name == "write_file" || p_name == "create_script") {
		arguments.push_back({ "content", true, false });
	}
	if (p_name == "edit_file") {
		arguments.push_back({ "old_text", true, false });
		arguments.push_back({ "new_text", true, false });
	}
	if (p_name == "search_text") {
		arguments.push_back({ "query", true, false });
	}
	if (p_name == "add_node" || p_name == "remove_node" || p_name == "rename_node" || p_name == "set_node_property" || p_name == "get_node_property" || p_name == "attach_script" || p_name == "select_node") {
		arguments.push_back({ "node_path", false, false });
	}
	if (p_name == "add_node") {
		arguments.push_back({ "type", true, false });
	}
	if (p_name == "add_node" || p_name == "rename_node") {
		arguments.push_back({ "name", true, false });
	}
	if (p_name == "set_node_property" || p_name == "get_node_property") {
		arguments.push_back({ "property", true, false });
	}
	if (p_name == "set_node_property") {
		arguments.push_back({ "value", true, true });
	}
	if (p_name == "attach_script") {
		arguments.push_back({ "script_path", true, false });
	}
	return arguments;
}

bool is_write_tool_name(const std::string &p_name) {
	static const char *write_tools[] = {
		"write_file",
		"edit_file",
		"create_folder",
		"delete_file",
		"create_script",
		"attach_script",
		"create_scene",
		"save_current_scene",
		"save_scene_as",
		"set_main_scene",
		"add_node",
		"remove_node",
		"rename_node",
		"set_node_property",
		"auto_commit",
		// These actions can execute scripts or change editor state even without
		// directly writing a file. They must not bypass future approval gates.
		"validate_script",
		"open_scene",
		"scan_filesystem",
		"select_node",
		"run_project",
		"run_current_scene",
		"stop_project",
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

bool GoAgenticsTools::is_write_tool(const std::string &p_name) {
	return is_write_tool_name(p_name);
}

std::string GoAgenticsTools::tool_definitions_json() {
	std::ostringstream stream;
	stream << "[";
	for (size_t i = 0; i < TOOL_NAMES.size(); ++i) {
		if (i > 0) {
			stream << ",";
		}
		const std::string &name = TOOL_NAMES[i];
		stream << "{\"type\":\"function\",\"function\":{\"name\":" << escape_json_string(name)
			   << ",\"description\":" << escape_json_string("GO. project editor tool: " + name)
			   << ",\"parameters\":{\"type\":\"object\",\"additionalProperties\":false,\"properties\":{";
		const auto arguments = tool_arguments(name);
		std::vector<std::string> required;
		for (size_t j = 0; j < arguments.size(); ++j) {
			const ToolArgument &argument = arguments[j];
			if (j > 0) {
				stream << ",";
			}
			stream << escape_json_string(argument.name) << ":{\"type\":"
				   << (argument.scalar ? "[\"string\",\"number\",\"boolean\"]" : "\"string\"") << "}";
			if (argument.required) {
				required.push_back(escape_json_string(argument.name));
			}
		}
		stream << "},\"required\":[" << join(required, ",") << "]}}}";
	}
	stream << "]";
	return stream.str();
}

std::string GoAgenticsTools::validate_arguments(const std::string &p_name, const std::string &p_arguments_json) {
	if (!is_tool_allowed(p_name) || p_arguments_json.size() > 512000) {
		return "Unknown tool or oversized argument envelope";
	}
	std::string error;
	const JsonValue arguments = parse_json(p_arguments_json, error);
	if (!error.empty() || arguments.type != JsonValue::Type::OBJECT) {
		return "Tool arguments must be a JSON object";
	}
	const auto specifications = tool_arguments(p_name);
	for (const ToolArgument &specification : specifications) {
		if (specification.required && arguments.get(specification.name) == nullptr) {
			return "Missing required argument: " + specification.name;
		}
	}
	for (const auto &entry : arguments.object_value) {
		const auto specification = std::find_if(specifications.begin(), specifications.end(), [&](const ToolArgument &p_argument) { return p_argument.name == entry.first; });
		if (specification == specifications.end()) {
			return "Unexpected argument: " + entry.first;
		}
		const JsonValue &value = entry.second;
		if (value.type != JsonValue::Type::STRING && !(specification->scalar && (value.type == JsonValue::Type::BOOL || value.type == JsonValue::Type::NUMBER))) {
			return "Wrong argument type: " + entry.first;
		}
		if (value.type != JsonValue::Type::STRING) {
			continue;
		}
		const std::string &text = value.string_value;
		const bool content = entry.first == "content" || entry.first == "old_text" || entry.first == "new_text" || entry.first == "value";
		if (text.size() > (content ? 200000 : 4096) || text.find('\0') != std::string::npos) {
			return "Oversized argument or embedded NUL: " + entry.first;
		}
		if (text.empty() && entry.first != "content" && entry.first != "new_text" && entry.first != "value") {
			return "Empty argument: " + entry.first;
		}
		if (entry.first == "path" || entry.first == "script_path") {
			const auto path = validate_res_path(text, is_write_tool(p_name) && entry.first != "script_path" ? PathOperation::WRITE : PathOperation::READ);
			if (!path.ok) {
				return path.error;
			}
		}
		if (entry.first == "node_path" && (text.front() == '/' || text.find("..") != std::string::npos || text.find(':') != std::string::npos)) {
			return "Node path must stay inside the edited scene";
		}
	}
	return "";
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
		const std::string path_validation = validate_tool_path(path_value->as_string(), is_write_tool_name(p_name));
		if (contains(path_validation, "\"ok\":false")) {
			return path_validation;
		}
	}
	result << "{\"ok\":false,\"error\":\"tool execution is not wired to the Godot editor yet\",\"tool\":"
		   << escape_json_string(p_name) << ",\"path_validated\":true}";
	return result.str();
}

} // namespace go
