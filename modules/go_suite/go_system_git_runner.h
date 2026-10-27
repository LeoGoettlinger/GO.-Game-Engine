// GO. Game Engine — Copyright (c) 2026 SampleCraft / GO. contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "go_system_git.h"

#include "core/string/ustring.h"

#include <string>
#include <vector>

namespace go_suite {

std::string to_std_string(const String &p_string);
String to_godot_string(const std::string &p_string);

class GoSystemGitRunner {
	std::string project_path;

public:
	GoSystemGitRunner() = default;
	explicit GoSystemGitRunner(const String &p_project_path);

	void set_project_path(const String &p_project_path);
	const std::string &get_project_path() const { return project_path; }
	bool run(const go::GitCommand &p_command, std::string &r_output, int &r_exit_code) const;
	bool run_git(const std::vector<std::string> &p_arguments, std::string &r_output, int &r_exit_code) const;
};

} // namespace go_suite
