// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <string>
#include <vector>

namespace go {

class GoAgenticsTools {
public:
	static std::string tool_definitions_json();
	static bool is_tool_allowed(const std::string &p_name);
	static std::vector<std::string> allowed_tool_names();
	static std::string validate_tool_path(const std::string &p_path, bool p_for_write);
	static std::string execute_tool(const std::string &p_name, const std::string &p_arguments_json);
};

} // namespace go
